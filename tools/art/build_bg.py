#!/usr/bin/env python3
"""Decors (512 x 224, deux palettes : PAL0 propre au decor + PAL3 partagee).

Chaque tuile 8x8 doit n'utiliser que les couleurs d'UNE des deux palettes ; le script
produit un PNG 8 bits ou l'index = palette * 16 + couleur (format accepte par rescomp).

Index speciaux de PAL0 (animations par palette, voir src/stage.c) :
  11, 12 : bras leves du public (groupes 1 et 2) - invisibles hors acclamations
  13     : flashs d'appareils photo
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_ui import PAL3  # noqa: E402
from render import to_vdp  # noqa: E402
from font import glyph  # noqa: E402
from logo import banner  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
W, H = 512, 224
K3, WH3, LG3, DG3, RD3, DR3, GR3, DGR3, YE3, OR3, BL3, DBL3, SK3, SKS3, NV3 = range(1, 16)


def P3(i):
    return 48 + i


class Canvas:
    """Image en index 'globaux' : 0-15 = PAL0, 48-63 = PAL3."""

    def __init__(self, w=W, h=H, fill=0):
        self.a = np.full((h, w), fill, np.int32)
        self.w, self.h = w, h

    def rect(self, x0, y0, x1, y1, c):
        self.a[max(0, y0):min(self.h, y1), max(0, x0):min(self.w, x1)] = c

    def px(self, x, y, c):
        x %= self.w
        if 0 <= y < self.h:
            self.a[y, x] = c

    def disc(self, cx, cy, r, c, wrap=True):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if (x - cx + 0.5) ** 2 + (y - cy + 0.5) ** 2 <= r * r:
                    if wrap:
                        self.px(x, y, c)
                    elif 0 <= x < self.w and 0 <= y < self.h:
                        self.a[y, x] = c

    def dither(self, x0, y0, x1, y1, c, phase=0):
        for y in range(max(0, y0), min(self.h, y1)):
            for x in range(x0, x1):
                if (x + y + phase) % 2 == 0:
                    self.px(x, y, c)

    def text(self, s, x, y, c, shadow=None, scale=1):
        for i, ch in enumerate(s):
            g = glyph(ch)
            for gy in range(7):
                for gx in range(6):
                    if g[gy][gx]:
                        for sy in range(scale):
                            for sx in range(scale):
                                if shadow is not None:
                                    self.px(x + (i * 7 + gx) * scale + sx + 1, y + gy * scale + sy + 1, shadow)
                for gx in range(6):
                    pass
            for gy in range(7):
                for gx in range(6):
                    if g[gy][gx]:
                        for sy in range(scale):
                            for sx in range(scale):
                                self.px(x + (i * 7 + gx) * scale + sx, y + gy * scale + sy, c)


def export(cv, pal0, path, name):
    """verifie les palettes par tuile et ecrit le PNG 8 bits."""
    a = cv.a
    bad = 0
    for ty in range(cv.h // 8):
        for tx in range(cv.w // 8):
            t = a[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8]
            pals = set((t // 16).ravel().tolist())
            nz = t[t % 16 != 0]
            pals_nz = set((nz // 16).ravel().tolist())
            if len(pals_nz) > 1:
                bad += 1
                # repli : tout passe dans la palette majoritaire (couleur la plus proche)
                maj = max(pals_nz, key=lambda p: int((nz // 16 == p).sum()))
                for y in range(8):
                    for x in range(8):
                        v = t[y, x]
                        if v % 16 and v // 16 != maj:
                            t[y, x] = nearest(v, maj, pal0)
            elif len(pals_nz) == 1:
                p = pals_nz.pop()
                t[t % 16 == 0] = p * 16      # le transparent prend le numero de palette de la tuile
    full = [(0, 0, 0)] * 64
    for i, c in enumerate(pal0):
        full[i] = to_vdp(c)
    for i, c in enumerate(PAL3):
        full[48 + i] = c
    im = Image.fromarray(a.astype(np.uint8), "P")
    flat = []
    for c in full:
        flat += list(c)
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.save(path)
    # tuiles uniques (avec retournements)
    seen = set()
    for ty in range(cv.h // 8):
        for tx in range(cv.w // 8):
            t = a[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8] % 16
            p = int(a[ty * 8, tx * 8] // 16)
            vs = [t, t[:, ::-1], t[::-1, :], t[::-1, ::-1]]
            seen.add(min(v.tobytes() for v in vs))
    print("%-10s tuiles uniques : %d  (tuiles a 2 palettes corrigees : %d)" % (name, len(seen), bad))
    return len(seen)


def nearest(v, pal_idx, pal0):
    src = (pal0 + [(0, 0, 0)] * 16)[v % 16] if v // 16 == 0 else PAL3[v % 16]
    cand = pal0 if pal_idx == 0 else PAL3
    best, bd = 1, 1e9
    for i in range(1, 16):
        c = cand[i] if i < len(cand) else (0, 0, 0)
        d = sum((a - b) ** 2 for a, b in zip(src, c))
        if d < bd:
            best, bd = i, d
    return pal_idx * 16 + best


# ---------------------------------------------------------------- public
def crowd(cv, y0, y1, pal, seed, period=128, tiers=5, shirts=(5, 6, 7, 8, 9), skins=(3, 4), hair=10,
          bg=1, bg2=2):
    """Gradins + public en rangees (periodique de `period` px pour limiter les tuiles)."""
    rng = random.Random(seed)
    cv.rect(0, y0, W, y1, bg)
    th = (y1 - y0) / tiers
    for t in range(tiers):
        ty = y0 + int(t * th)
        # marche des gradins
        cv.rect(0, ty + int(th) - 2, W, ty + int(th) - 1, bg2)
    pattern = []
    for t in range(tiers):
        r = 2.2 + t * 0.45
        ty = y0 + int((t + 1) * th) - 2
        x = rng.uniform(0, 4)
        row = []
        while x < period:
            row.append((x, rng.choice(shirts), rng.choice(skins), rng.random() < 0.35, rng.randint(0, 1),
                        rng.random() < 0.15, rng.uniform(-1, 1)))
            x += r * 2.6 + rng.uniform(0, 3)
        pattern.append((r, ty, row))
    for rep in range(W // period):
        ox = rep * period
        for t, (r, ty, row) in enumerate(pattern):
            for x, shirt, skin, hairy, grp, flash, jit in row:
                cx = ox + x
                hy = ty - r * 2.4 + jit
                # bras leves (palette 11/12, invisibles au repos)
                arm = 11 + grp
                for k in range(int(r * 2.2)):
                    cv.px(int(cx - r * 1.3), int(hy - k + r * 0.5), arm)
                    cv.px(int(cx + r * 1.3), int(hy - k + r * 0.5), arm)
                cv.disc(int(cx - r * 1.3), hy - r * 2.0, max(0.8, r * 0.45), arm)
                cv.disc(int(cx + r * 1.3), hy - r * 2.0, max(0.8, r * 0.45), arm)
                # buste
                for yy in range(int(hy + r * 0.6), ty + 2):
                    half = r * 1.35 + (yy - hy) * 0.15
                    for xx in range(int(cx - half), int(cx + half) + 1):
                        cv.px(xx, yy, shirt)
                # tete
                cv.disc(cx, hy, r, skin)
                if hairy:
                    for xx in range(int(cx - r), int(cx + r) + 1):
                        cv.px(xx, int(hy - r + 0.5), hair)
                if flash:
                    cv.px(int(cx + r * 0.8), int(hy + r * 0.8), 13)
    return pattern


# ---------------------------------------------------------------- tapis en perspective
FLOOR_TOP = 144


def mat(cv, main=(BL3, DBL3), border=(YE3, OR3), y0=FLOOR_TOP, stripe=8, seam_period=64, center_light=True):
    cv.rect(0, y0, W, H, P3(main[0]))
    # bande de securite au fond (pres du mur), puis zone de combat
    cv.rect(0, y0, W, y0 + stripe, P3(border[0]))
    cv.rect(0, y0 + stripe - 1, W, y0 + stripe, P3(border[1]))
    # degrade vers le fond : trame sombre
    for y in range(y0 + stripe, y0 + stripe + 18):
        dens = 1 - (y - y0 - stripe) / 18
        for x in range(W):
            if (x + y) % 2 == 0 and dens > 0.45:
                cv.px(x, y, P3(main[1]))
            elif (x * 3 + y) % 4 == 0 and dens > 0.15:
                cv.px(x, y, P3(main[1]))
    # joints horizontaux (espacement croissant vers l'avant)
    yy = y0 + stripe + 6
    gap = 6
    while yy < H:
        cv.rect(0, yy, W, yy + 1, P3(main[1]))
        gap = int(gap * 1.45) + 1
        yy += gap
    # joints verticaux
    for x in range(0, W, seam_period):
        cv.rect(x, y0 + stripe, x + 1, H, P3(main[1]))
    if center_light:
        # halo de projecteur au centre du tapis (trame claire), periodique pour le parallaxe
        for y in range(y0 + stripe + 20, H):
            for x in range(W):
                d = ((x % 256) - 128) ** 2 / (110 ** 2) + ((y - 196) / 26.0) ** 2
                if d < 1 and (x + y) % 2 == 0 and cv.a[y, x] == P3(main[0]):
                    cv.px(x, y, P3(WH3) if d < 0.25 and (x // 2 + y) % 2 == 0 else P3(main[0]))


def club_banner(cv, x, y, w=56, h=64):
    """etendard du club (Ronin Fight Team), tuiles PAL3 alignees."""
    assert x % 8 == 0 and y % 8 == 0 and w % 8 == 0 and h % 8 == 0
    b = banner(w, h)
    for yy in range(h):
        for xx in range(w):
            cv.px(x + xx, y + yy, P3(int(b[yy, xx])))


def ad_boards(cv, y0, y1, items):
    """Panneaux publicitaires (PAL3) le long de la barriere."""
    cv.rect(0, y0, W, y1, P3(K3))
    x = 0
    i = 0
    while x < W:
        bg, fg, txt, w = items[i % len(items)]
        cv.rect(x + 1, y0 + 2, x + w - 1, y1 - 2, P3(bg))
        cv.rect(x + 1, y0 + 2, x + w - 1, y0 + 3, P3(WH3) if bg != WH3 else P3(LG3))
        tw = len(txt) * 7
        cv.text(txt, x + (w - tw) // 2, y0 + (y1 - y0 - 7) // 2, P3(fg), shadow=P3(K3))
        x += w
        i += 1
    cv.rect(0, y1 - 2, W, y1, P3(DG3))


# ---------------------------------------------------------------- arene
def stage_arena():
    pal0 = [(18, 18, 36), (36, 36, 72), (54, 54, 109), (218, 145, 109), (145, 90, 54), (182, 36, 54),
            (36, 109, 182), (218, 182, 36), (218, 218, 218), (36, 145, 72), (18, 18, 18), (36, 36, 72),
            (36, 36, 72), (36, 36, 72), (72, 72, 128), (255, 255, 182)]
    cv = Canvas(fill=P3(NV3))
    # plafond (PAL3) : poutres et projecteurs
    cv.rect(0, 0, W, 40, P3(NV3))
    for y in (6, 22):
        cv.rect(0, y, W, y + 2, P3(DG3))
    for x in range(0, W, 16):
        for k in range(16):
            cv.px(x + k, 8 + k, P3(DG3))
            cv.px(x + 15 - k, 8 + k, P3(DG3))
    for x in range(24, W, 64):
        cv.rect(x - 5, 24, x + 6, 31, P3(K3))
        cv.rect(x - 4, 25, x + 5, 30, P3(LG3))
        cv.rect(x - 3, 26, x + 4, 29, P3(WH3))
        cv.dither(x - 8, 32, x + 9, 40, P3(LG3))
    # gradins + public (PAL0)
    crowd(cv, 40, 120, pal0, seed=7)
    # faisceaux de lumiere sur le public (trame)
    for x in range(24, W, 64):
        for y in range(40, 120):
            spread = (y - 40) * 0.25 + 6
            for xx in range(int(x - spread), int(x + spread)):
                if (xx + y) % 2 == 0 and cv.a[y, xx % W] == 1:
                    cv.px(xx, y, 14)
    # etendards du club accroches au-dessus des gradins
    for x in (96, 352):
        club_banner(cv, x, 56, 56, 64)
    # barriere publicitaire (PAL3)
    ad_boards(cv, 120, 144, [(BL3, WH3, "BJJ TATAMI", 96), (YE3, K3, "OSS!", 48), (RD3, WH3, "JIU-JITSU", 80),
                             (WH3, BL3, "TATAMI", 64), (GR3, YE3, "BRASIL", 64), (K3, YE3, "MUNDIAL", 64),
                             (K3, RD3, "RONIN F.T.", 96)])
    mat(cv, center_light=False)
    return cv, pal0


# ---------------------------------------------------------------- academie
def stage_academy():
    pal0 = [(18, 18, 18), (109, 72, 36), (145, 90, 54), (182, 127, 72), (218, 164, 109), (72, 36, 18),
            (182, 218, 255), (109, 145, 218), (218, 218, 182), (145, 145, 109), (36, 18, 18), (182, 127, 72),
            (182, 127, 72), (218, 218, 182), (72, 109, 72), (255, 255, 218)]
    cv = Canvas(fill=2)
    # mur en lambris de bois (PAL0)
    cv.rect(0, 0, W, 144, 2)
    for x in range(0, W, 16):
        cv.rect(x, 0, x + 1, 144, 1)
        cv.rect(x + 1, 0, x + 2, 144, 3)
    for y in (40, 104):
        cv.rect(0, y, W, y + 3, 5)
        cv.rect(0, y, W, y + 1, 4)
    # fenetres hautes (ciel)
    for x in range(32, W, 128):
        cv.rect(x, 48, x + 56, 96, 10)
        cv.rect(x + 2, 50, x + 54, 94, 6)
        cv.rect(x + 2, 72, x + 54, 94, 7)
        cv.rect(x + 28, 50, x + 30, 94, 10)
        cv.rect(x + 2, 71, x + 54, 73, 10)
        for k in range(8):
            cv.px(x + 6 + k * 2, 54 + k, 15)
    # drapeaux et ceintures accrochees (PAL3 sur tuiles alignees)
    for x in (96, 352):
        flag_bra(cv, x, 56)
    for x in (224, 480):
        club_banner(cv, x, 56, 56, 56)
    for x in range(160 - 64, W, 256):
        cv.rect(x, 112, x + 56, 128, P3(K3))
        cv.rect(x + 1, 113, x + 55, 127, P3(WH3))
        cv.text("OSS", x + 18, 117, P3(RD3))
    # rack de ceintures
    for x in range(0, W, 256):
        cv.rect(x + 200, 112, x + 248, 114, 5)
        for k, col in enumerate((8, 4, 7, 2, 0, 0)):
            cv.rect(x + 202 + k * 8, 114, x + 206 + k * 8, 136, col if col else 10)
    # plinthe
    cv.rect(0, 136, W, 144, 5)
    cv.rect(0, 136, W, 137, 3)
    # tatamis puzzle vert / bleu (PAL3)
    cv.rect(0, 144, W, H, P3(GR3))
    yy = 150
    gap = 10
    row = 0
    while yy < H:
        nxt = min(H, yy + gap)
        for x in range(0, W, 32):
            if ((x // 32) + row) % 2:
                cv.rect(x, yy, x + 32, nxt, P3(BL3))
        cv.rect(0, yy, W, yy + 1, P3(DGR3))
        yy = nxt
        gap = int(gap * 1.35) + 1
        row += 1
    cv.rect(0, 144, W, 150, P3(DGR3))
    cv.dither(0, 150, W, 158, P3(DGR3))
    for x in range(0, W, 32):
        cv.rect(x, 150, x + 1, H, P3(DGR3))
    return cv, pal0


def flag_bra(cv, x, y):
    cv.rect(x, y, x + 32, y + 24, P3(GR3))
    for k in range(10):
        cv.rect(x + 16 - (k + 1) * 14 // 10, y + 2 + k, x + 16 + (k + 1) * 14 // 10, y + 3 + k, P3(YE3))
        cv.rect(x + 16 - (k + 1) * 14 // 10, y + 21 - k, x + 16 + (k + 1) * 14 // 10, y + 22 - k, P3(YE3))
    cv.disc(x + 16, y + 12, 5.5, P3(BL3), wrap=False)
    cv.rect(x + 11, y + 12, x + 21, y + 13, P3(WH3))


# ---------------------------------------------------------------- plage
def beach_flag(cv):
    """drapeau du club plante sur la plage (mat en PAL0, toile en PAL3)."""
    for y in range(52, 112):
        cv.px(185, y, 0)
        cv.px(186, y, 14)
        cv.px(187, y, 0)
    cv.disc(186, 52, 1.6, 15, wrap=False)
    club_banner(cv, 192, 56, 56, 56)


def stage_beach():
    pal0 = [(36, 18, 54), (255, 182, 72), (255, 145, 54), (218, 90, 72), (145, 54, 90), (72, 36, 90),
            (255, 218, 145), (36, 54, 109), (54, 90, 145), (109, 145, 182), (18, 36, 54), (218, 164, 109),
            (218, 164, 109), (255, 255, 218), (36, 36, 72), (255, 255, 218)]
    cv = Canvas(fill=1)
    # ciel couchant en bandes tramees (PAL0)
    bands = [(0, 5), (18, 4), (34, 3), (52, 2), (70, 1), (86, 6)]
    for i, (y, c) in enumerate(bands):
        y2 = bands[i + 1][0] if i + 1 < len(bands) else 96
        cv.rect(0, y, W, y2, c)
        if i + 1 < len(bands):
            cv.dither(0, y2 - 4, W, y2, bands[i + 1][1])
    # soleil
    cv.disc(352, 92, 18, 13, wrap=False)
    cv.disc(352, 92, 14, 15, wrap=False)
    # Pain de Sucre et collines (silhouettes)
    for x in range(W):
        dx = (x - 120) / 22.0
        dome = 50 * math.sqrt(max(0.0, 1 - dx * dx)) if abs(dx) < 1 else 0
        h1 = 18 + max(dome, 30 * math.exp(-((x - 150) / 30) ** 2)) + 6 * math.sin(x / 37.0) \
            + 26 * math.exp(-((x - 440) / 40) ** 2)
        top = int(108 - h1)
        for y in range(max(0, top), 112):
            cv.px(x, y, 0 if y < top + 2 else 14)
    # mer
    cv.rect(0, 104, W, 120, 7)
    for y in range(104, 120, 3):
        for x in range(W):
            if (x * 7 + y * 13) % 23 < 3:
                cv.px(x, y, 9)
    for x in range(332, 372):
        for y in range(106, 120, 2):
            if (x + y) % 3 == 0:
                cv.px(x, y, 15)
    # public sur le sable (PAL0 : peaux et tenues de plage)
    pal_people = crowd(cv, 112, 144, pal0, seed=11, tiers=2, shirts=(3, 2, 9, 4, 6), skins=(11, 12) if False else (3, 2),
                       hair=0, bg=6, bg2=6)
    # cordes de la zone de combat (PAL3)
    cv.rect(0, 136, W, 144, P3(YE3))
    for x in range(0, W, 32):
        cv.rect(x, 128, x + 2, 144, P3(WH3))
    cv.rect(0, 130, W, 131, P3(RD3))
    cv.rect(0, 136, W, 137, P3(OR3))
    # tapis sur le sable
    mat(cv, main=(RD3, DR3), border=(YE3, OR3), center_light=False)
    beach_flag(cv)
    # palmiers (bords, PAL0) : tronc + palmes, devant la mer
    for px_ in (40, 296):
        for k in range(46):
            x = px_ + int(6 * math.sin(k / 14.0))
            cv.rect(x, 96 - k, x + 3, 97 - k, 14)
        for a in range(0, 360, 40):
            for r in range(16):
                x = px_ + int(4 * math.sin(46 / 14.0)) + int(r * math.cos(math.radians(a)))
                y = 50 + int(r * math.sin(math.radians(a)) * 0.6 + r * r * 0.03)
                cv.px(x, y, 14)
                cv.px(x, y + 1, 0)
    return cv, pal0


def hud_band(cv):
    """bandeau du tableau de score (lignes 0-55) : fond sombre uni, liseres dores."""
    cv.rect(0, 0, W, 56, P3(NV3))
    cv.dither(0, 0, W, 6, P3(K3))
    for x in range(0, W, 32):
        for k in range(4):
            cv.px(x + 8 + k, 44 - k, P3(DBL3))
    cv.rect(0, 55, W, 56, P3(OR3))


def with_band(fn):
    def g():
        cv, pal0 = fn()
        hud_band(cv)
        return cv, pal0
    return g


STAGES = [("arena", with_band(stage_arena)), ("academy", with_band(stage_academy)), ("beach", with_band(stage_beach))]


def main():
    os.makedirs(os.path.join(ROOT, "res", "bg"), exist_ok=True)
    pals = []
    for name, fn in STAGES:
        cv, pal0 = fn()
        export(cv, pal0, os.path.join(ROOT, "res", "bg", "stage_%s.png" % name), name)
        pals.append(pal0)


if __name__ == "__main__":
    main()
