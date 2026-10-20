#!/usr/bin/env python3
"""Portraits des 27 combattants (64 x 72, palette du combattant : memes index que les sprites).

Les attributs (peau, cheveux, barbe, kimono) sont lus dans src/roster.c pour rester synchronises.
Sortie : res/portraits/portrait_XX.png
"""
import math
import os
import re
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from preview import SKINS, HAIRS, GIS  # noqa: E402
from render import to_vdp  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
W, H = 64, 72
OUT, SK_HI, SK_MID, SK_SH, HAIR_HI, HAIR_DK, GI_HI, GI_MID, GI_SH, GI_DEEP, BELT, BEARD, RED, EYE, ACC = range(1, 16)
BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def parse_roster():
    src = open(os.path.join(ROOT, "src", "roster.c")).read()
    out = []
    for m in re.finditer(r'F\("([^"]+)",\s*"([^"]+)",\s*([\d, ]+?),\s*(SUBF_\w+),\s*(GI_\w+),\s*(SKIN_\w+),\s*(HAIR_\w+),\s*(\d),\s*(\d),\s*(\w+)\)', src):
        name, nick, stats, sp, gi, skin, hair, beard, acc, flag = m.groups()
        out.append(dict(name=name, gi=["GI_WHITE", "GI_BLUE", "GI_BLACK"].index(gi),
                        skin=["SKIN_LIGHT", "SKIN_MED", "SKIN_DARK"].index(skin),
                        hair=["HAIR_BLACK", "HAIR_BROWN", "HAIR_BLOND", "HAIR_BALD"].index(hair),
                        beard=int(beard), acc=int(acc)))
    return out


def shade_idx(v, tones, x, y):
    """v en [0,1] (1 = clair) -> code, avec trame ordonnee pour les degrades."""
    t = v + (BAYER[y % 4, x % 4] - 0.5) * 0.18
    if t > 0.66:
        return tones[0]
    if t > 0.36:
        return tones[1]
    return tones[2]


def portrait(f, gid):
    rng = np.random.RandomState(gid * 7 + 3)
    cat = gid // 3
    heavy = cat / 8.0                      # 0 = poids coq, 1 = poids lourd
    a = np.zeros((H, W), np.uint8)
    yy, xx = np.mgrid[0:H, 0:W]
    cx = 32 + rng.randint(-1, 2)
    hw = 13.0 + heavy * 3.0 + rng.uniform(-0.5, 0.8)      # demi-largeur du visage
    hh = 18.0 + rng.uniform(-0.5, 1.0)
    cy = 29.0
    neck_w = 9 + heavy * 5
    # ---------- epaules + kimono
    sh_y = 54
    body = (yy >= sh_y - 2 + np.abs(xx - cx) * 0.18)
    gi_v = 0.75 - (xx - cx) / 70.0 - (yy - sh_y) / 60.0
    for y in range(H):
        for x in range(W):
            if body[y, x]:
                a[y, x] = shade_idx(gi_v[y, x], (GI_HI, GI_MID, GI_SH), x, y)
    # cou
    for y in range(36, sh_y + 6):
        for x in range(W):
            if abs(x - cx) <= neck_w + (y - 44) * 0.25 and y >= 38:
                v = 0.62 - (x - cx) / (neck_w * 2.2) - max(0, (46 - y)) * 0.04
                a[y, x] = shade_idx(v, (SK_HI, SK_MID, SK_SH), x, y)
    # revers croises (col en V) + col epais
    for y in range(sh_y - 4, H):
        d = (y - (sh_y - 4)) * 0.9
        for x in range(W):
            lx = x - (cx - 12 + d * 0.55)
            rx = (cx + 12 - d * 0.55) - x
            if -2.5 <= lx <= 2.5 and x < cx + 4 and y > sh_y - 4:
                a[y, x] = GI_HI if lx < 1 else GI_MID
            if -2.5 <= rx <= 2.5 and x > cx - 4 and y > sh_y - 4:
                a[y, x] = GI_MID if rx < 1 else GI_SH
        # peau visible dans le V
    for y in range(sh_y - 2, H):
        half = max(0, 10 - (y - sh_y) * 0.9)
        for x in range(int(cx - half), int(cx + half) + 1):
            if a[y, x] in (GI_HI, GI_MID, GI_SH) and abs(x - cx) < half - 2:
                a[y, x] = SK_SH if x > cx else SK_MID
    # ---------- tete : ovale + machoire
    def in_head(x, y):
        dy = (y - cy) / hh
        jaw = 1.0 if dy < 0.25 else 1.0 - (dy - 0.25) * (0.55 - heavy * 0.15)
        return ((x - cx) / (hw * jaw)) ** 2 + dy * dy <= 1.0
    head = np.zeros((H, W), bool)
    for y in range(H):
        for x in range(W):
            head[y, x] = in_head(x + 0.5, y + 0.5)
    # oreilles (chou-fleur)
    for side in (-1, 1):
        ex = cx + side * (hw + 0.5)
        for y in range(int(cy - 4), int(cy + 7)):
            for x in range(W):
                d = ((x - ex) / 3.4) ** 2 + ((y - cy - 1.5) / 5.6) ** 2
                bump = 0.25 * math.sin(y * 1.7 + side)
                if d <= 1 + bump and not head[y, x]:
                    a[y, x] = SK_MID if side < 0 else SK_SH
                    if d < 0.35:
                        a[y, x] = SK_SH
    for y in range(H):
        for x in range(W):
            if head[y, x]:
                nx = (x - cx) / hw
                ny = (y - cy) / hh
                v = 0.78 - nx * 0.55 - ny * 0.2 - (nx * nx + ny * ny) * 0.25
                a[y, x] = shade_idx(v, (SK_HI, SK_MID, SK_SH), x, y)
    # ---------- barbe / ombre de barbe (index dedie)
    if f["beard"]:
        for y in range(H):
            for x in range(W):
                if head[y, x] and (y - cy) > hh * 0.30 + abs(x - cx) * 0.05:
                    if not ((y - cy) > hh * 0.42 and abs(x - cx) < 4.5 and (y - cy) < hh * 0.56):
                        a[y, x] = BEARD
                    if (x + y) % 3 == 0 and a[y, x] == BEARD and x > cx:
                        a[y, x] = HAIR_DK
    # ---------- cheveux
    style = rng.randint(0, 4) if f["hair"] != 3 else -1
    top = cy - hh
    for y in range(H):
        for x in range(W):
            if not head[y, x] and not (y < cy and abs(x - cx) < hw + 2 and y > top - 4):
                continue
            ny = (y - cy) / hh
            nx = (x - cx) / hw
            if f["hair"] == 3:
                continue
            line = -0.42 + 0.10 * abs(nx) ** 2
            if style == 1:                 # boucles
                line += 0.06 * math.sin(x * 1.3)
            if style == 2:                 # raie sur le cote
                line += 0.05 * nx
            vol = 1.10 if style in (1, 3) else 1.06
            ex_ = (x + 0.5 - cx) / (hw * vol)
            ey_ = (y + 0.5 - cy) / (hh * vol)
            inside_vol = ex_ * ex_ + ey_ * ey_ <= 1.0
            in_hair = inside_vol and ((ny < line) or (abs(nx) > 0.93 and ny < -0.05 and style != 0))
            if style == 3 and abs(x - cx) > 4 and ny < line:   # crete
                in_hair = abs(nx) > 0.9 and ny < -0.1 and ny > -0.6
            if in_hair:
                v = 0.7 - nx * 0.4 - (ny + 1) * 0.2
                a[y, x] = HAIR_HI if (v + (BAYER[y % 4, x % 4] - 0.5) * 0.3) > 0.55 else HAIR_DK
    if f["hair"] == 3:
        # crane rase : reflet
        for y in range(int(top + 2), int(top + 6)):
            for x in range(int(cx - 8), int(cx - 2)):
                if head[y, x] and (x + y) % 2 == 0:
                    a[y, x] = EYE if y == int(top + 3) and x in (cx - 6, cx - 5) else SK_HI
    # ---------- sourcils, yeux, nez, bouche
    ey = int(cy + 0)
    brow = HAIR_DK if f["hair"] != 2 else SK_SH
    for side in (-1, 1):
        ex = cx + side * 6
        # sourcil fronce (angle vers le centre)
        for k in range(-3, 4):
            yb = ey - 4 + (abs(k + side * 3) // 3 if True else 0) - (1 if side * k < 0 else 0) + (1 if side * k > 1 else 0)
            a[ey - 4 - (1 if side * k <= -2 else 0) + (1 if side * k >= 2 else 0), ex + k] = brow
            a[ey - 3 - (1 if side * k <= -2 else 0) + (1 if side * k >= 2 else 0), ex + k] = brow if abs(k) < 3 else a[ey - 3, ex + k]
        # oeil
        for k in range(-2, 3):
            a[ey, ex + k] = EYE
        a[ey - 1, ex - 2:ex + 3] = OUT
        a[ey, ex + (0 if side > 0 else 0)] = OUT
        a[ey, ex - side] = OUT if rng.rand() < 0.5 else a[ey, ex - side]
        a[ey + 1, ex - 1:ex + 2] = SK_SH
    # nez
    for y in range(ey + 1, ey + 8):
        a[y, cx + 1] = SK_SH
    a[ey + 7, cx - 2:cx + 3] = SK_SH
    a[ey + 8, cx - 1] = OUT
    a[ey + 8, cx + 2] = OUT
    a[ey + 4:ey + 7, cx - 1] = SK_HI
    # bouche (determine : levres serrees ; certains montrent les dents)
    my = ey + 12
    grin = rng.rand() < 0.3
    for x in range(cx - 5, cx + 6):
        a[my, x] = OUT if not grin else (EYE if abs(x - cx) < 4 else OUT)
    if grin:
        a[my - 1, cx - 4:cx + 5] = OUT
        a[my + 1, cx - 4:cx + 5] = OUT
    a[my + 1, cx - 3:cx + 4] = RED if not grin else a[my + 1, cx - 3:cx + 4]
    a[my + 2, cx - 2:cx + 3] = SK_SH
    # cicatrice pour quelques-uns
    if rng.rand() < 0.25:
        sx = cx + 8 + rng.randint(-1, 2)
        for k in range(5):
            a[ey - 6 + k, sx - k // 2] = SK_SH if k % 2 else SK_HI
    # ---------- contour
    m = a > 0
    ring = np.zeros_like(m)
    ring[1:, :] |= m[:-1, :]
    ring[:-1, :] |= m[1:, :]
    ring[:, 1:] |= m[:, :-1]
    ring[:, :-1] |= m[:, 1:]
    a[ring & ~m] = OUT
    a[H - 1, :] = np.where(a[H - 1, :] > 0, a[H - 1, :], 0)
    return a


def fighter_rgb_pal(f):
    s, h, g = SKINS[f["skin"]], HAIRS[min(f["hair"], 2)], GIS[f["gi"]]
    if f["hair"] == 3:
        h = (s[1], s[2])
    beard = HAIRS[0][1] if f["hair"] == 3 else h[1]
    pal = [(255, 0, 255), (16, 16, 40), s[0], s[1], s[2], h[0], h[1], g[0], g[1], g[2], g[3], (26, 26, 26),
           beard if f["beard"] else s[1], (200, 30, 30), (250, 250, 250), (230, 196, 40)]
    return [to_vdp(c) for c in pal]


def main():
    fighters = parse_roster()
    assert len(fighters) == 27, len(fighters)
    os.makedirs(os.path.join(ROOT, "res", "portraits"), exist_ok=True)
    sheet = Image.new("RGB", (W * 9, H * 3), (40, 40, 60))
    for gid, f in enumerate(fighters):
        a = portrait(f, gid)
        im = Image.fromarray(a, "P")
        flat = []
        for c in fighter_rgb_pal(f):
            flat += list(c)
        im.putpalette(flat + [0] * (768 - len(flat)))
        im.save(os.path.join(ROOT, "res", "portraits", "portrait_%02d.png" % gid))
        rgb = im.convert("RGB")
        bg = Image.new("RGB", (W, H), (40, 40, 60))
        mask = Image.fromarray(((a > 0) * 255).astype(np.uint8))
        bg.paste(rgb, (0, 0), mask)
        sheet.paste(bg, ((gid % 9) * W, (gid // 9) * H))
    sheet.save(os.path.join(os.environ.get("PREVIEW_DIR", "/tmp"), "portraits.png"))
    print("portraits :", len(fighters))


if __name__ == "__main__":
    main()
