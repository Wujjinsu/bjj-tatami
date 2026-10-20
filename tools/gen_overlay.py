#!/usr/bin/env python3
"""Genere l'overlay tactile RetroArch de BJJ Tatami (paysage, ecran 20:9 type 2400x1080).

    python3 tools/gen_overlay.py

Sorties : retroarch/overlay/bjj-tatami.png et retroarch/overlay/bjj-tatami.cfg
Correspondance Genesis Plus GX : bouton RetroPad Y = A Mega Drive, B = B, A = C.
"""
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 2400, 1080                 # 20:9 ; le jeu 4:3 occupe x = 480..1920
HIT = 1.08                        # zones tactiles un peu plus grandes que le dessin (sans chevauchement A/B/C)

FILL = (25, 25, 35, 150)
EDGE = (255, 255, 255, 190)
TEXT = (255, 255, 255, 230)


def font(size):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "C:/Windows/Fonts/arialbd.ttf",
              "/System/Library/Fonts/Supplemental/Arial Bold.ttf"):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def label(d, cx, cy, txt, size):
    f = font(size)
    b = d.textbbox((0, 0), txt, font=f)
    d.text((cx - (b[2] - b[0]) / 2 - b[0], cy - (b[3] - b[1]) / 2 - b[1]), txt, font=f, fill=TEXT)


descs = []


def radial(name, cx, cy, r):
    descs.append(f'"{name},{cx / W:.4f},{cy / H:.4f},radial,{r * HIT / W:.4f},{r * HIT / H:.4f}"')


def rect(name, cx, cy, rx, ry):
    descs.append(f'"{name},{cx / W:.4f},{cy / H:.4f},rect,{rx / W:.4f},{ry / H:.4f}"')


def main():
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)

    # --- croix directionnelle (gauche)
    cx, cy, arm, half = 240, 650, 175, 62
    d.rounded_rectangle([cx - half, cy - arm, cx + half, cy + arm], 18, fill=FILL, outline=EDGE, width=5)
    d.rounded_rectangle([cx - arm, cy - half, cx + arm, cy + half], 18, fill=FILL, outline=EDGE, width=5)
    d.rectangle([cx - half + 4, cy - half + 4, cx + half - 4, cy + half - 4], fill=FILL)
    for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):          # petites fleches
        tx, ty = cx + dx * 120, cy + dy * 120
        if dx == 0:
            pts = [(tx - 22, ty - dy * 14), (tx + 22, ty - dy * 14), (tx, ty + dy * 22)]
        else:
            pts = [(tx - dx * 14, ty - 22), (tx - dx * 14, ty + 22), (tx + dx * 22, ty)]
        d.polygon(pts, fill=TEXT)
    zx, zy = 75, 75
    rect("up", cx, cy - 115, zx, zy)
    rect("down", cx, cy + 115, zx, zy)
    rect("left", cx - 115, cy, zx, zy)
    rect("right", cx + 115, cy, zx, zy)
    rect("left|up", cx - 115, cy - 115, 40, 40)
    rect("right|up", cx + 115, cy - 115, 40, 40)
    rect("left|down", cx - 115, cy + 115, 40, 40)
    rect("right|down", cx + 115, cy + 115, 40, 40)

    # --- A B C en arc montant, comme la manette Mega Drive (droite)
    r = 88
    for name, letter, bx, by in (("y", "A", 2005, 835), ("b", "B", 2150, 712), ("a", "C", 2295, 590)):
        d.ellipse([bx - r, by - r, bx + r, by + r], fill=FILL, outline=EDGE, width=6)
        label(d, bx, by, letter, 92)
        radial(name, bx, by, r)

    # --- START
    sx, sy = 2140, 990
    d.rounded_rectangle([sx - 85, sy - 34, sx + 85, sy + 34], 34, fill=FILL, outline=EDGE, width=4)
    label(d, sx, sy, "START", 40)
    rect("start", sx, sy, 95, 45)

    # --- menu RetroArch (haut gauche)
    mx, my = 95, 85
    d.ellipse([mx - 50, my - 50, mx + 50, my + 50], fill=FILL, outline=EDGE, width=4)
    for k in (-16, 0, 16):
        d.rectangle([mx - 24, my + k - 4, mx + 24, my + k + 4], fill=TEXT)
    radial("menu_toggle", mx, my, 50)

    out = os.path.join(ROOT, "retroarch", "overlay")
    os.makedirs(out, exist_ok=True)
    im.save(os.path.join(out, "bjj-tatami.png"))
    lines = [
        "# Overlay tactile BJJ Tatami - paysage, ecran 20:9 (2400x1080)",
        "# Genesis Plus GX : y = A Mega Drive, b = B, a = C",
        "overlays = 1",
        'overlay0_overlay = "bjj-tatami.png"',
        "overlay0_full_screen = true",
        "overlay0_normalized = true",
        "overlay0_range_mod = 1.5",
        "overlay0_alpha_mod = 2.0",
        f"overlay0_descs = {len(descs)}",
    ] + [f"overlay0_desc{i} = {v}" for i, v in enumerate(descs)]
    with open(os.path.join(out, "bjj-tatami.cfg"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print("ok :", len(descs), "zones")


if __name__ == "__main__":
    main()
