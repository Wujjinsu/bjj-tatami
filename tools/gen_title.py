#!/usr/bin/env python3
"""Ecran titre BJJ TATAMI (320x224, 16 couleurs) -> res/bg/title.png

    python3 tools/gen_title.py
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_art  # noqa: E402

ROOT = gen_art.ROOT
W, H = 320, 224

PAL = [
    (18, 18, 54),     # 0 fond
    (36, 36, 90),     # 1 rayons
    (0, 0, 0),        # 2 noir
    (162, 170, 198),  # 3 gris (ombre kimono blanc)
    (252, 216, 36),   # 4 jaune
    (0, 144, 54),     # 5 vert
    (0, 90, 36),      # 6 vert fonce
    (36, 54, 162),    # 7 bleu drapeau
    (216, 36, 36),    # 8 rouge
    (252, 180, 144),  # 9 peau A
    (198, 126, 90),   # 10 peau ombre
    (90, 54, 18),     # 11 cheveux A
    (36, 90, 216),    # 12 kimono bleu
    (18, 54, 144),    # 13 kimono bleu ombre
    (198, 138, 90),   # 14 peau B
    (252, 252, 252),  # 15 blanc (aussi couleur de la police SGDK)
]
# index du sprite "ground" -> index de cette palette
GROUND_MAP = {1: 2, 2: 9, 3: 10, 4: 11, 5: 15, 6: 3, 7: 2, 8: 12, 9: 13, 10: 2, 11: 8, 12: 14, 13: 10, 14: 2}


def font(size):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "C:/Windows/Fonts/arialbd.ttf"):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def text_outlined(d, xy_center, txt, size, fill, outline=2, shadow=None):
    f = font(size)
    b = d.textbbox((0, 0), txt, font=f)
    x = xy_center[0] - (b[2] - b[0]) // 2 - b[0]
    y = xy_center[1] - b[1]
    if shadow is not None:
        for ox in range(-outline, outline + 1):
            for oy in range(-outline, outline + 1):
                d.text((x + ox + 3, y + oy + 3), txt, font=f, fill=shadow)
    for ox in range(-outline, outline + 1):
        for oy in range(-outline, outline + 1):
            d.text((x + ox, y + oy), txt, font=f, fill=2)
    d.text((x, y), txt, font=f, fill=fill)


def main():
    im = gen_art.new_img(W, H, PAL)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"                                  # pas d'anticrenelage : vrais pixels

    cx, cy = 160, 100
    # rayons autour du badge
    for k in range(20):
        a0 = 2 * math.pi * k / 20
        a1 = a0 + math.pi / 20
        pts = [(cx, cy), (cx + 400 * math.cos(a0), cy + 400 * math.sin(a0)),
               (cx + 400 * math.cos(a1), cy + 400 * math.sin(a1))]
        d.polygon(pts, fill=1)

    # badge : anneau noir, filet jaune, disque vert, losange jaune, cercle bleu
    def disk(r, c):
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=c)
    disk(68, 2)
    disk(58, 4)
    disk(56, 5)
    d.polygon([(cx - 50, cy), (cx, cy - 34), (cx + 50, cy), (cx, cy + 34)], fill=4)
    disk(22, 7)
    d.arc([cx - 22, cy - 22, cx + 22, cy + 22], 200, 340, fill=15, width=3)   # bandeau du drapeau

    # deux combattants (garde fermee) en pixel art x2
    B, A, order = gen_art.G_GUARD
    pair = gen_art.render_pair(B, A, order)
    big = pair.resize((128, 80), Image.NEAREST)
    bp = big.load()
    ip = im.load()
    ox, oy = cx - 64, cy - 44
    for y in range(80):
        for x in range(128):
            v = bp[x, y]
            if v:
                ip[ox + x, oy + y] = GROUND_MAP.get(v, 2)

    # etoiles du badge (gauche / droite de l'anneau)
    for sx in (cx - 63, cx + 63):
        d.polygon([(sx, cy - 5), (sx + 2, cy - 1), (sx + 6, cy - 1), (sx + 3, cy + 2),
                   (sx + 4, cy + 6), (sx, cy + 3), (sx - 4, cy + 6), (sx - 3, cy + 2),
                   (sx - 6, cy - 1), (sx - 2, cy - 1)], fill=4)

    # titre
    text_outlined(d, (160, 3), "BJJ TATAMI", 34, 4, outline=2, shadow=8)
    # sous-titre sous le badge
    text_outlined(d, (160, 171), "JIU-JITSU BRESILIEN", 13, 15, outline=1)

    # bande du bas pour le menu (texte ecrit par le jeu avec la police SGDK)
    d.rectangle([0, 186, W - 1, H - 1], fill=0)
    d.rectangle([0, 186, W - 1, 187], fill=4)

    out = os.path.join(ROOT, "res", "bg", "title.png")
    im.save(out)

    # nombre de tuiles uniques (avec retournements), budget VRAM ~ 1000
    tiles = set()
    for ty in range(0, H, 8):
        for tx in range(0, W, 8):
            t = tuple(ip[tx + x, ty + y] for y in range(8) for x in range(8))
            rows = [t[i * 8:(i + 1) * 8] for i in range(8)]
            hf = tuple(v for r in rows for v in r[::-1])
            vf = tuple(v for r in rows[::-1] for v in r)
            hvf = tuple(v for r in rows[::-1] for v in r[::-1])
            tiles.add(min(t, hf, vf, hvf))
    print("title.png :", len(tiles), "tuiles uniques")


if __name__ == "__main__":
    main()
