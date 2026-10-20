#!/usr/bin/env python3
"""Ecrans fixes : titre (badge), fond de menus, fond du versus, vignettes de selection.

  res/bg/title.png     8 bits (PAL0 = rayons qui tournent par cycle de palette, PAL3 = badge)
  res/bg/menu_bg.png   PAL3 seulement
  res/bg/vs_bg.png     PAL0 + PAL3
  res/ui/mugshots.png  27 vignettes 24x24 (palette PAL0 de l'ecran de selection)
  src/gen/screens_pal.h  palettes PAL0 de la selection
"""
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_bg import Canvas, export, P3, W as SW  # noqa: E402
from build_ui import PAL3, K, WH, LG, DG, RD, DR, GR, DGR, YE, OR, BL, DBL, SK, SKS, NV  # noqa: E402
from render import Layer, render_fighter, remap, to_vdp  # noqa: E402
import pairs  # noqa: E402
import build_portraits as BP  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

# fighter roles -> PAL3 (pour dessiner des combattants dans un decor)
MAP_WHITE = {1: K, 2: SK, 3: SK, 4: SKS, 5: DG, 6: K, 7: WH, 8: WH, 9: LG, 10: DG, 11: K, 12: SKS, 13: RD,
             14: WH, 15: YE, 17: WH, 18: WH, 19: LG, 20: DG, 21: DG}
MAP_BLUE = dict(MAP_WHITE)
MAP_BLUE.update({7: BL, 8: BL, 9: DBL, 10: DBL, 17: BL, 18: BL, 19: DBL, 20: DBL, 21: DBL, 2: SK, 3: SKS, 4: SKS})


def curved_text(cv, text, cx, cy, r, a0, a1, size, color, outline=None, inward=False):
    fnt = ImageFont.truetype(FONT, size)
    n = len(text)
    for i, ch in enumerate(text):
        t = (i + 0.5) / n
        a = a0 + (a1 - a0) * t
        im = Image.new("L", (size * 2, size * 2), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((size // 2, size // 4), ch, font=fnt, fill=255)
        rot = -(a + 90) if not inward else -(a - 90)
        im = im.rotate(rot, resample=Image.NEAREST, expand=False)
        m = np.array(im) > 127
        px = cx + r * math.cos(math.radians(a))
        py = cy + r * math.sin(math.radians(a))
        ys, xs = np.where(m)
        if len(xs) == 0:
            continue
        ox = int(round(px - (xs.min() + xs.max()) / 2))
        oy = int(round(py - (ys.min() + ys.max()) / 2))
        for y, x in zip(ys, xs):
            if outline is not None:
                for dx, dy in ((1, 1), (1, 0), (0, 1)):
                    if 0 <= y + oy + dy < cv.h and 0 <= x + ox + dx < cv.w and cv.a[y + oy + dy, x + ox + dx] != P3(color):
                        cv.a[y + oy + dy, x + ox + dx] = P3(outline)
        for y, x in zip(ys, xs):
            if 0 <= y + oy < cv.h and 0 <= x + ox < cv.w:
                cv.a[y + oy, x + ox] = P3(color)


def logo_text(cv, text, cx, y, size, grad=(WH, YE, YE, OR, RD)):
    fnt = ImageFont.truetype(FONT, size)
    im = Image.new("L", (size * len(text) + 40, size * 2), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    d.text((20, size // 4), text, font=fnt, fill=255)
    m = np.array(im) > 127
    ys, xs = np.where(m)
    m = m[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    h, w = m.shape
    x0 = cx - w // 2
    # contour 2 px + ombre
    for dy in range(-2, 5):
        for dx in range(-2, 5):
            sh = dx > 2 or dy > 2
            for yy, xx in zip(*np.where(m)):
                Y, X = y + yy + dy, x0 + xx + dx
                if 0 <= Y < cv.h and 0 <= X < cv.w and cv.a[Y, X] not in [P3(c) for c in grad]:
                    cv.a[Y, X] = P3(DR if sh else K)
    for yy, xx in zip(*np.where(m)):
        g = grad[min(len(grad) - 1, int(yy / h * len(grad)))]
        cv.a[y + yy, x0 + xx] = P3(g)


def paste_layer(cv, idx, mapping, ox, oy, clip=None):
    h, w = idx.shape
    for y in range(h):
        for x in range(w):
            v = idx[y, x]
            if v == 0:
                continue
            X, Y = ox + x, oy + y
            if clip is not None and not clip(X, Y):
                continue
            if 0 <= X < cv.w and 0 <= Y < cv.h:
                cv.a[Y, X] = P3(mapping.get(int(v), K))


# ---------------------------------------------------------------- titre
def title():
    pal0 = [(18, 18, 54), (36, 28, 72), (54, 36, 109), (36, 28, 72), (18, 18, 54), (0, 0, 0), (0, 0, 0),
            (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
    cv = Canvas(320, 224, fill=1)
    cx, cy, R = 160, 96, 88
    yy, xx = np.mgrid[0:224, 0:320]
    ang = (np.degrees(np.arctan2(yy - cy, xx - cx)) + 360) % 360
    # rayons : 24 secteurs, index 1..4 en sequence (le cycle de palette les fait tourner)
    cv.a[:, :] = 1 + ((ang // 15).astype(int) % 4)
    r = np.hypot(yy - cy, xx - cx)
    # badge (PAL3)
    cv.a[r <= R] = P3(K)
    cv.a[r <= R - 2] = P3(WH)
    cv.a[r <= R - 4] = P3(K)
    inner = R - 24
    cv.a[r <= inner + 2] = P3(YE)
    cv.a[r <= inner] = P3(GR)
    # losange jaune + cercle bleu (drapeau stylise)
    dia = (np.abs(xx - cx) / (inner * 0.95) + np.abs(yy - cy) / (inner * 0.62)) <= 1
    cv.a[dia & (r <= inner)] = P3(YE)
    cv.a[(r <= inner * 0.42)] = P3(BL)
    band = (np.abs(np.hypot(yy - cy + 40, xx - cx) - 52) <= 2.2) & (r <= inner * 0.42)
    cv.a[band] = P3(WH)
    # textes courbes
    curved_text(cv, "BJJ TATAMI", cx, cy, R - 13, 200, 340, 17, WH, outline=DG)
    curved_text(cv, "OSS * HONRA * RESPEITO", cx, cy, R - 13, 160, 20, 10, YE, inward=True)
    for a in (180, 0):
        sx = cx + (R - 13) * math.cos(math.radians(a))
        sy = cy + (R - 13) * math.sin(math.radians(a))
        cv.disc(sx, sy, 3.2, P3(YE), wrap=False)
    # deux combattants : projection de hanche (image de la sequence TD_JUDO)
    s = pairs.td_judo()
    fr = s.frames()
    A, B, order, _, _ = fr[7]
    from preview import layers_from_order
    clip = lambda X, Y: (X - cx) ** 2 + (Y - cy) ** 2 <= (inner - 1) ** 2
    for who, gs in layers_from_order(order):
        L = Layer(176, 112)
        render_fighter(L, A if who == "A" else B, groups=gs)
        L.outline()
        paste_layer(cv, L.idx, MAP_WHITE if who == "A" else MAP_BLUE, cx - 88 - 4, cy - 66, clip)
    # sol du badge
    cv.a[(yy > cy + 42) & (r <= inner)] = P3(DGR)
    cv.a[(yy == cy + 42) & (r <= inner)] = P3(K)
    # bande du menu
    cv.a[190:, :] = P3(NV)
    cv.a[190, :] = P3(YE)
    cv.a[191, :] = P3(OR)
    return cv, pal0


# ---------------------------------------------------------------- fond de menus
def menu_bg():
    pal0 = [(18, 18, 54)] + [(0, 0, 0)] * 15
    cv = Canvas(320, 224, fill=P3(NV))
    yy, xx = np.mgrid[0:224, 0:320]
    # trame diagonale discrete
    cv.a[((xx + yy) % 8 == 0)] = P3(DBL)
    # cadre dore
    cv.a[2:4, 4:316] = P3(YE)
    cv.a[220:222, 4:316] = P3(YE)
    cv.a[2:222, 4:6] = P3(YE)
    cv.a[2:222, 314:316] = P3(YE)
    cv.a[4, 6:314] = P3(OR)
    cv.a[219, 6:314] = P3(OR)
    return cv, pal0


# ---------------------------------------------------------------- versus
def vs_bg():
    pal0 = [(18, 18, 54), (128, 0, 0), (182, 36, 36), (0, 36, 128), (36, 72, 182), (255, 255, 182)] + [(0, 0, 0)] * 10
    cv = Canvas(320, 224, fill=1)
    yy, xx = np.mgrid[0:224, 0:320]
    left = xx < 160 + (yy - 112) * 0.35
    cv.a[left] = 2
    cv.a[~left] = 4
    cv.a[left & ((xx // 4 + yy // 4) % 2 == 0)] = 1
    cv.a[~left & ((xx // 4 + yy // 4) % 2 == 0)] = 3
    edge = np.abs(xx - (160 + (yy - 112) * 0.35)) < 2
    cv.a[edge] = 5
    logo_text(cv, "VS", 160, 84, 44, grad=(WH, YE, YE, OR, RD))
    return cv, pal0


# ---------------------------------------------------------------- vignettes
MUG_PAL = [(18, 18, 54), (16, 16, 40), (252, 200, 160), (220, 150, 110), (214, 150, 100), (160, 100, 64),
           (150, 96, 60), (90, 56, 30), (40, 30, 30), (130, 84, 44), (230, 190, 90), (252, 252, 252),
           (60, 120, 240), (70, 70, 84), (182, 182, 210), (255, 218, 36)]
MUG_PAL = [to_vdp(c) for c in MUG_PAL]


def mugshots():
    fighters = BP.parse_roster()
    tiles = np.zeros((24, 24 * 27), np.uint8)
    for gid, f in enumerate(fighters):
        a = BP.portrait(f, gid)
        rgb = np.array(Image.fromarray(a, "P").convert("RGB")) if False else None
        pal = BP.fighter_rgb_pal(f)
        src = np.array(pal, float)[a]
        crop = src[6:6 + 60, 2:2 + 60]
        mask = (a[6:66, 2:62] > 0).astype(float)
        im = Image.fromarray(crop.astype(np.uint8)).resize((24, 24), Image.BOX)
        mk = np.array(Image.fromarray((mask * 255).astype(np.uint8)).resize((24, 24), Image.BOX)) > 100
        arr = np.array(im).astype(float)
        P = np.array(MUG_PAL[1:], float)
        d = ((arr[:, :, None, :] - P[None, None, :, :]) ** 2).sum(-1)
        idx = d.argmin(-1) + 1
        idx[~mk] = 0
        tiles[:, gid * 24:(gid + 1) * 24] = idx
    # cadre : contour 1 px
    out = tiles.copy()
    # re-arrange en tuiles 8x8 : 3x3 tuiles par vignette, ordre ligne par ligne
    seq = []
    for gid in range(27):
        for ty in range(3):
            for tx in range(3):
                seq.append(out[ty * 8:(ty + 1) * 8, gid * 24 + tx * 8:gid * 24 + (tx + 1) * 8])
    sheet = np.zeros((((len(seq) + 15) // 16) * 8, 128), np.uint8)
    for i, t in enumerate(seq):
        sheet[(i // 16) * 8:(i // 16 + 1) * 8, (i % 16) * 8:(i % 16 + 1) * 8] = t
    im = Image.fromarray(sheet, "P")
    flat = []
    for c in MUG_PAL:
        flat += list(c)
    im.putpalette(flat + [0] * (768 - len(flat)))
    return im


def vdp(c):
    return ((c[2] >> 5) << 9) | ((c[1] >> 5) << 5) | ((c[0] >> 5) << 1)


def main():
    os.makedirs(os.path.join(ROOT, "res", "bg"), exist_ok=True)
    for name, fn in (("title", title), ("menu_bg", menu_bg), ("vs_bg", vs_bg)):
        cv, pal0 = fn()
        export(cv, pal0, os.path.join(ROOT, "res", "bg", name + ".png"), name)
    mugshots().save(os.path.join(ROOT, "res", "ui", "mugshots.png"))
    open(os.path.join(ROOT, "src", "gen", "screens_pal.h"), "w").write(
        "/* genere par tools/art/build_screens.py */\n"
        "static const u16 mugPal[16] = { %s };\n" % ", ".join("0x%03X" % vdp(c) for c in MUG_PAL) +
        "static const u16 titleRays[4] = { 0x%03X, 0x%03X, 0x%03X, 0x%03X };\n" % (
            vdp(to_vdp((36, 28, 72))), vdp(to_vdp((54, 36, 109))), vdp(to_vdp((36, 28, 72))), vdp(to_vdp((18, 18, 54)))))
    print("ok")


if __name__ == "__main__":
    main()
