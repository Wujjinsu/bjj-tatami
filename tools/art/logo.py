"""Blason du club (Ronin Fight Team) redessine en pixel art aux couleurs de PAL3.

Le soleil rouge, les katanas croises et les lettres sont dessines a la main (traits nets),
le casque et le masque de samourai sont dessines pixel par pixel (MASK).
"""
import math
import os

import numpy as np
from PIL import Image, ImageEnhance

from build_ui import PAL3, K, WH, LG, DG, RD, DR, YE, OR
from font import glyph

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "res", "logo", "ronin_src.jpg")


def _quant(a, use):
    pal = np.array([PAL3[i] for i in use], float)
    d = ((a[:, :, None, :] - pal[None, None]) ** 2).sum(-1)
    return np.array(use)[d.argmin(-1)]


MASK = [
    "..W..................W..",
    "..WL................LW..",
    "...WL..............LW...",
    "....WL.....YY.....LW....",
    ".....WLL..YWWY..LLW.....",
    "......KWWWLYYLWWWK......",
    ".....KDDDDDDDDDDDDK.....",
    "....KDLLLDDDDDDLLLDK....",
    "...KDDDDDDDDDDDDDDDDK...",
    "..KDDKKKKKKKKKKKKKKDDK..",
    "..KDKWWWWKLLLLKWWWWKDK..",
    ".KDDKLKRKLLLLLLKRKLKDDK.",
    ".KDKLLKKLLLWWLLLKKLLKDK.",
    ".KDKLLLLLLKLLKLLLLLLKDK.",
    "KDDKLLLLLLLKKLLLLLLLKDDK",
    "KDKDLLKKKKKKKKKKKKLLDKDK",
    "KDKDLKWKWKWKKWKWKWKLDKDK",
    "KDDKLKKRRRRRRRRRRKKLKDDK",
    ".KDKLKWKWKWKKWKWKWKLKDK.",
    ".KDDKLKKKKKKKKKKKKLKDDK.",
    "..KDDKLLLLLLLLLLLLKDDK..",
    "..KKDDKLLLWWWWLLLKDDKK..",
    "....KKDKKLLLLLLKKDKK....",
    "......KKKKKKKKKKKK......",
]
MCOL = {"K": K, "D": DG, "L": LG, "W": WH, "R": RD, "Y": YE}


def _mask_paste(a, x0, y0):
    for y, row in enumerate(MASK):
        assert len(row) == 24, (y, len(row))
        for x, ch in enumerate(row):
            if ch != "." and 0 <= y0 + y < a.shape[0] and 0 <= x0 + x < a.shape[1]:
                a[y0 + y, x0 + x] = MCOL[ch]


def _text(a, s, x, y, c, shadow=None):
    for i, ch in enumerate(s):
        g = glyph(ch)
        for gy in range(7):
            for gx in range(6):
                if g[gy][gx]:
                    if shadow is not None and 0 <= y + gy + 1 < a.shape[0] and 0 <= x + i * 7 + gx + 1 < a.shape[1]:
                        if a[y + gy + 1, x + i * 7 + gx + 1] != c:
                            a[y + gy + 1, x + i * 7 + gx + 1] = shadow
        for gy in range(7):
            for gx in range(6):
                if g[gy][gx] and 0 <= y + gy < a.shape[0] and 0 <= x + i * 7 + gx < a.shape[1]:
                    a[y + gy, x + i * 7 + gx] = c


def _line(a, x0, y0, x1, y1, c, wdt=1.0):
    n = int(max(abs(x1 - x0), abs(y1 - y0)) * 3) + 1
    for k in range(n + 1):
        t = k / n
        x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        for dy in range(-2, 3):
            for dx in range(-2, 3):
                xx, yy = int(round(x + dx * 0.5)), int(round(y + dy * 0.5))
                if (dx * 0.5) ** 2 + (dy * 0.5) ** 2 <= (wdt / 2) ** 2 + 0.01 and 0 <= yy < a.shape[0] and 0 <= xx < a.shape[1]:
                    a[yy, xx] = c


def badge(w=56, h=56, subtitle=False):
    """blason complet : tableau h x w d'indices PAL3, fond K."""
    a = np.full((h, w), K, np.int32)
    cx = w / 2 - 0.5
    top = 9
    cy = top + (h - top - (17 if subtitle else 9)) / 2
    R = min(w * 0.27, (h - top) * 0.33)
    yy, xx = np.mgrid[0:h, 0:w]
    r = np.hypot(xx - cx, yy - cy)
    # soleil rouge
    a[r <= R] = RD
    a[(r <= R) & (r > R - 1.5) & ((xx + yy) % 2 == 0)] = DR
    # katanas croises : poignee en haut, lame vers le bas
    for s in (-1, 1):
        hx, hy = cx + s * (w * 0.46), top - 6           # bout de la poignee
        gx_, gy_ = cx + s * (w * 0.30), top + 5         # garde (tsuba)
        bx, by = cx - s * (w * 0.44), h - (12 if subtitle else 6)   # pointe
        _line(a, hx, hy, gx_, gy_, DG, 2.2)
        # tresse de la poignee
        for k in range(1, 4):
            t = k / 4
            px, py = int(round(hx + (gx_ - hx) * t)), int(round(hy + (gy_ - hy) * t))
            if 0 <= py < h and 0 <= px < w:
                a[py, px] = YE if k == 2 else K
        _line(a, gx_, gy_, bx, by, LG, 1.6)
        _line(a, gx_ + s * 0.5, gy_ - 0.5, bx + s * 0.5, by - 0.5, WH, 0.9)
        dx, dy = bx - hx, by - hy
        n = math.hypot(dx, dy)
        px_, py_ = -dy / n * 2.6, dx / n * 2.6
        _line(a, gx_ - px_, gy_ - py_, gx_ + px_, gy_ + py_, YE, 1.6)
        a[int(round(gy_)), int(round(gx_))] = OR
    # casque et masque de samourai (kabuto + menpo)
    _mask_paste(a, int(round(cx - 11.5)), int(round(cy - 13)))
    # lettres R, F, T
    _text(a, "F", int(cx - 2), 0, RD)
    _text(a, "R", 1, int(cy) - 3, WH, DG)
    _text(a, "T", w - 7, int(cy) - 3, WH, DG)
    # nom du club
    ty = h - (16 if subtitle else 8)
    _text(a, "RONIN", int(cx - 17), ty, WH, DG)
    if subtitle:
        _text(a, "FIGHT TEAM", int(cx - 35), ty + 8, RD)
    return a


def banner(w=56, h=64):
    """etendard suspendu : tringle, blason, franges dorees."""
    a = np.full((h, w), K, np.int32)
    a[0:3, :] = DG
    a[1, :] = LG
    a[0:3, 0] = YE
    a[0:3, w - 1] = YE
    a[3:h - 6, 3:w - 3] = badge(w - 6, h - 9)
    a[3:h - 6, 0] = DG
    a[3:h - 6, w - 1] = DG
    a[3:h - 6, 1:3] = K
    a[3:h - 6, w - 3:w - 1] = K
    for x in range(w):
        for y in range(h - 6, h):
            a[y, x] = YE if (y - (h - 6)) < 2 else (OR if (x % 3 != 2 and y - (h - 6) < 3 + (x % 3) * 2) else K)
    return a


def to_rgb(a):
    return np.array(PAL3, np.uint8)[a]


if __name__ == "__main__":
    out = [Image.fromarray(to_rgb(badge(56, 56))).resize((224, 224), Image.NEAREST),
           Image.fromarray(to_rgb(badge(72, 72, True))).resize((288, 288), Image.NEAREST),
           Image.fromarray(to_rgb(badge(40, 40))).resize((160, 160), Image.NEAREST)]
    W = Image.new("RGB", (224 + 288 + 160 + 40, 288), (255, 0, 255))
    x = 0
    for i in out:
        W.paste(i, (x, 0)); x += i.width + 20
    W.save(os.environ.get("OUT", "/tmp/logo_test.png"))
