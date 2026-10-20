#!/usr/bin/env python3
"""Genere les images du jeu (PNG indexes 16 couleurs, format attendu par rescomp/SGDK).

    python3 tools/gen_art.py      (depuis le dossier du projet)

Sorties : res/sprites/fighter.png, res/sprites/ground.png, res/bg/dojo.png
"""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ------------------------------------------------------------------ palettes
# Index 0 = transparent. Couleurs choisies sur la grille Mega Drive (pas de 36).
SPRITE_PAL = [
    (255, 0, 255),   # 0 transparent
    (18, 18, 36),    # 1 contour
    (252, 180, 144), # 2 peau
    (216, 126, 90),  # 3 peau ombre
    (54, 36, 18),    # 4 cheveux
    (236, 236, 236), # 5 kimono A (haut / joueur)
    (162, 170, 198), # 6 kimono A ombre
    (24, 24, 24),    # 7 ceinture A
    (36, 90, 216),   # 8 kimono B (bas / CPU)
    (18, 54, 144),   # 9 kimono B ombre
    (144, 54, 180),  # 10 ceinture B
    (216, 36, 36),   # 11 barrette rouge de la ceinture noire
    (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0),
]

SKIN, SKIN_SH, HAIR, OUT = 2, 3, 4, 1
GRP_A = dict(gi=5, sh=6, belt=7)
GRP_B = dict(gi=8, sh=9, belt=10)


def new_img(w, h, pal):
    im = Image.new("P", (w, h), 0)
    flat = []
    for c in pal:
        flat += list(c)
    im.putpalette(flat + [0] * (768 - len(flat)))
    return im


def line(d, a, b, col, w):
    d.line([a, b], fill=col, width=w)
    r = w // 2
    for p in (a, b):  # bouts arrondis
        d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=col)


def lerp(a, b, t):
    return (round(a[0] + (b[0] - a[0]) * t), round(a[1] + (b[1] - a[1]) * t))


def draw_fig(d, J, g, parts=("back", "body", "front")):
    """Personnage dessine a partir de ses articulations (pixel art 1:1)."""
    if "back" in parts:
        if "kneeB" in J:
            line(d, J["hip"], J["kneeB"], g["sh"], 5)
            line(d, J["kneeB"], J["footB"], g["sh"], 4)
            d.ellipse([J["footB"][0] - 2, J["footB"][1] - 1, J["footB"][0] + 2, J["footB"][1] + 1], fill=SKIN_SH)
        if "elbowB" in J:
            line(d, J["neck"], J["elbowB"], g["sh"], 4)
            line(d, J["elbowB"], J["handB"], g["sh"], 3)
            h = J["handB"]
            d.ellipse([h[0] - 2, h[1] - 2, h[0] + 2, h[1] + 2], fill=SKIN_SH)
    if "body" in parts:
        line(d, J["neck"], J["hip"], g["gi"], 9)
        # revers du kimono
        line(d, J["neck"], lerp(J["neck"], J["hip"], 0.6), g["sh"], 1)
        # ceinture
        b0, b1 = lerp(J["hip"], J["neck"], 0.10), lerp(J["hip"], J["neck"], 0.18)
        d.line([b0, b1], fill=g["belt"], width=9)
        if g["belt"] == 7:  # ceinture noire : barrette rouge
            p = lerp(J["hip"], J["neck"], 0.14)
            d.point(p, fill=11)
        # tete : cheveux derriere, visage decale vers le regard
        hx, hy = J["head"]
        fx, fy = J.get("face", (1, 0))
        d.ellipse([hx - 5, hy - 5, hx + 5, hy + 5], fill=HAIR)
        d.ellipse([hx - 4 + fx, hy - 4 + fy, hx + 4 + fx, hy + 4 + fy], fill=SKIN)
        ex, ey = hx + 2 * fx + (1 if fx == 0 else 0), hy + 2 * fy - (1 if fy == 0 else 0)
        d.point((ex, ey), fill=OUT)  # oeil
    if "front" in parts:
        if "kneeF" in J:
            line(d, J["hip"], J["kneeF"], g["gi"], 5)
            line(d, J["kneeF"], J["footF"], g["gi"], 4)
            d.ellipse([J["footF"][0] - 2, J["footF"][1] - 1, J["footF"][0] + 2, J["footF"][1] + 1], fill=SKIN)
        if "elbowF" in J:
            line(d, J["neck"], J["elbowF"], g["gi"], 4)
            line(d, J["elbowF"], J["handF"], g["gi"], 3)
            h = J["handF"]
            d.ellipse([h[0] - 2, h[1] - 2, h[0] + 2, h[1] + 2], fill=SKIN)


def outline(im):
    w, h = im.size
    px = im.load()
    src = [[px[x, y] for x in range(w)] for y in range(h)]
    for y in range(h):
        for x in range(w):
            if src[y][x] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and src[ny][nx] not in (0, OUT):
                    px[x, y] = OUT
                    break


def shift(J, dx, dy, keys=None):
    return {k: ((v[0] + dx, v[1] + dy) if (keys is None or k in keys) and k != "face" else v)
            for k, v in J.items()}


UPPER = ("head", "neck", "elbowF", "handF", "elbowB", "handB")

# ------------------------------------------------------------ combattant debout
# Cadre 32x48, regarde vers la droite, pieds en y=46.
IDLE = dict(head=(19, 8), neck=(18, 14), hip=(16, 29), elbowF=(24, 21), handF=(28, 18),
            elbowB=(21, 22), handB=(25, 24), kneeF=(22, 37), footF=(24, 45),
            kneeB=(13, 37), footB=(10, 45), face=(1, 0))
WALK0 = dict(IDLE, kneeF=(23, 37), footF=(27, 45), kneeB=(12, 37), footB=(7, 45))
WALK1 = dict(IDLE, kneeF=(19, 37), footF=(18, 45), kneeB=(16, 37), footB=(15, 45))
SHOOT0 = dict(head=(22, 17), neck=(20, 21), hip=(14, 31), elbowF=(25, 27), handF=(28, 24),
              elbowB=(22, 28), handB=(26, 29), kneeF=(21, 38), footF=(25, 45),
              kneeB=(10, 38), footB=(5, 45), face=(1, 0))
SHOOT1 = dict(head=(25, 22), neck=(22, 25), hip=(13, 31), elbowF=(26, 31), handF=(28, 36),
              elbowB=(24, 32), handB=(27, 38), kneeF=(20, 43), footF=(14, 45),
              kneeB=(8, 39), footB=(3, 45), face=(1, 0))
SPRAWL0 = dict(head=(24, 31), neck=(21, 33), hip=(11, 37), elbowF=(25, 39), handF=(28, 44),
               elbowB=(22, 40), handB=(25, 45), kneeF=(6, 41), footF=(2, 45),
               kneeB=(7, 43), footB=(3, 46), face=(1, 1))
SPRAWL1 = shift(SPRAWL0, 0, 1, ("head", "neck", "hip"))

FIGHTER_ROWS = [
    [IDLE, shift(IDLE, 0, 1, UPPER)],  # 0 attente
    [WALK0, WALK1],                     # 1 marche
    [SHOOT0, SHOOT1],                   # 2 projection
    [SPRAWL0, SPRAWL1],                 # 3 sprawl / defense
]


def make_fighter():
    fw, fh = 32, 48
    im = new_img(fw * 2, fh * len(FIGHTER_ROWS), SPRITE_PAL)
    for r, row in enumerate(FIGHTER_ROWS):
        for c, J in enumerate(row):
            frame = new_img(fw, fh, SPRITE_PAL)
            draw_fig(ImageDraw.Draw(frame), J, GRP_A)
            outline(frame)
            im.paste(frame, (c * fw, r * fh))
    return im


# --------------------------------------------------------------- lutte au sol
# Cadre 64x40, sol en y=38. Groupe A = celui du dessus (attaquant), B = dessous.
def lying(hx, hy=32, hip_x=None):
    hip_x = hip_x if hip_x is not None else hx + 22
    return dict(head=(hx, hy), neck=(hx + 6, hy + 1), hip=(hip_x, hy + 1), face=(0, -1))


G_GUARD = (
    dict(lying(8), elbowF=(20, 26), handF=(29, 19), elbowB=(17, 28), handB=(26, 22),
         kneeF=(39, 23), footF=(45, 29), kneeB=(37, 25), footB=(44, 31)),
    dict(head=(33, 11), neck=(36, 16), hip=(43, 29), elbowF=(32, 23), handF=(29, 30),
         elbowB=(35, 24), handB=(31, 31), kneeF=(37, 37), footF=(51, 37),
         kneeB=(41, 37), footB=(55, 37), face=(-1, 0)),
    ["B.back", "B.body", "A.back", "A.body", "A.front", "B.front"],
)
G_HALF = (
    dict(lying(8), elbowF=(18, 27), handF=(24, 22), elbowB=(16, 29), handB=(22, 25),
         kneeF=(36, 32), footF=(46, 36), kneeB=(38, 29), footB=(48, 34)),
    dict(head=(21, 22), neck=(26, 24), hip=(41, 27), elbowF=(19, 30), handF=(13, 34),
         elbowB=(26, 31), handB=(21, 35), kneeF=(45, 36), footF=(57, 37),
         kneeB=(36, 37), footB=(48, 37), face=(-1, 1)),
    ["B.back", "B.body", "A.back", "A.body", "B.front", "A.front"],
)
G_SIDE = (
    dict(lying(8, 33), elbowF=(18, 28), handF=(22, 23), elbowB=(16, 30), handB=(20, 26),
         kneeF=(40, 27), footF=(48, 37), kneeB=(42, 30), footB=(52, 37)),
    dict(head=(13, 24), neck=(18, 25), hip=(36, 24), elbowF=(16, 31), handF=(10, 35),
         elbowB=(22, 31), handB=(18, 35), kneeF=(38, 36), footF=(50, 37),
         kneeB=(31, 36), footB=(43, 37), face=(0, 1)),
    ["B.back", "B.body", "B.front", "A.back", "A.body", "A.front"],
)
G_MOUNT = (
    dict(lying(8, 33), elbowF=(19, 28), handF=(24, 24), elbowB=(16, 30), handB=(21, 27),
         kneeF=(42, 29), footF=(50, 37), kneeB=(44, 32), footB=(55, 37)),
    dict(head=(27, 8), neck=(28, 14), hip=(29, 29), elbowF=(22, 22), handF=(17, 30),
         elbowB=(25, 23), handB=(20, 31), kneeF=(21, 36), footF=(33, 37),
         kneeB=(37, 36), footB=(44, 37), face=(-1, 1)),
    ["B.back", "B.body", "B.front", "A.back", "A.body", "A.front"],
)
G_BACK = (
    dict(head=(32, 13), neck=(31, 19), hip=(29, 34), elbowF=(37, 25), handF=(41, 28),
         elbowB=(35, 27), handB=(39, 30), kneeF=(41, 29), footF=(52, 37),
         kneeB=(39, 31), footB=(49, 37), face=(1, 0)),
    dict(head=(24, 12), neck=(23, 18), hip=(19, 34), elbowF=(31, 21), handF=(36, 18),
         elbowB=(28, 25), handB=(33, 24), kneeF=(30, 28), footF=(38, 33),
         kneeB=(28, 31), footB=(36, 35), face=(1, 0)),
    ["A.back", "A.body", "B.back", "B.body", "B.front", "A.front"],
)
GROUND_POS = [G_GUARD, G_HALF, G_SIDE, G_MOUNT, G_BACK]


def render_pair(B, A, order, dx=0, dy=0):
    fr = new_img(64, 40, SPRITE_PAL)
    d = ImageDraw.Draw(fr)
    A2 = shift(A, dx, dy)
    for step in order:
        who, part = step.split(".")
        if who == "A":
            draw_fig(d, A2, GRP_A, (part,))
        else:
            draw_fig(d, B, GRP_B, (part,))
    outline(fr)
    return fr


def make_ground():
    im = new_img(64 * 2, 40 * len(GROUND_POS), SPRITE_PAL)
    for r, (B, A, order) in enumerate(GROUND_POS):
        im.paste(render_pair(B, A, order), (0, r * 40))
        im.paste(render_pair(B, A, order, 1, 1), (64, r * 40))
    return im


# ------------------------------------------------------------------- le dojo
BG_PAL = [
    (18, 18, 36),    # 0 fond
    (216, 198, 144), # 1 mur
    (180, 162, 108), # 2 mur ombre / panneaux
    (126, 72, 36),   # 3 bois
    (90, 54, 18),    # 4 bois sombre
    (180, 36, 36),   # 5 banniere rouge
    (126, 18, 18),   # 6 banniere ombre
    (36, 90, 180),   # 7 tatami bleu
    (54, 126, 216),  # 8 tatami bleu clair
    (198, 54, 54),   # 9 tatami rouge (zone de securite)
    (18, 54, 126),   # 10 joints du tatami
    (252, 216, 72),  # 11 or
    (72, 72, 90),    # 12 gris
    (0, 0, 0), (0, 0, 0),
    (252, 252, 252), # 15 blanc (police SGDK)
]


def make_dojo():
    im = new_img(320, 224, BG_PAL)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, 319, 119], fill=1)
    for x in range(0, 320, 40):           # panneaux du mur
        d.rectangle([x, 0, x, 119], fill=2)
    d.rectangle([0, 112, 319, 119], fill=2)
    # banniere (le titre est ecrit par-dessus avec la police)
    d.rectangle([88, 32, 231, 63], fill=5)
    d.rectangle([88, 56, 231, 63], fill=6)
    d.rectangle([88, 32, 95, 63], fill=6)
    d.rectangle([224, 32, 231, 63], fill=6)
    d.rectangle([80, 24, 239, 31], fill=4)
    d.rectangle([152, 40, 167, 47], fill=11)  # embleme
    # lambris
    d.rectangle([0, 120, 319, 143], fill=3)
    d.rectangle([0, 120, 319, 127], fill=4)
    for x in range(0, 320, 32):
        d.rectangle([x, 128, x, 143], fill=4)
    # tatamis : bord rouge, centre bleu, joints tous les 64 px
    d.rectangle([0, 144, 319, 223], fill=9)
    d.rectangle([0, 152, 319, 215], fill=7)
    for x in range(0, 320, 64):
        d.rectangle([x, 152, x, 215], fill=10)
    d.rectangle([0, 183, 319, 183], fill=10)
    d.rectangle([64, 160, 255, 207], fill=8)    # zone de combat
    for x in range(64, 256, 64):
        d.rectangle([x, 160, x, 207], fill=10)
    d.rectangle([64, 183, 255, 183], fill=10)
    return im


def main():
    os.makedirs(os.path.join(ROOT, "res", "sprites"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "res", "bg"), exist_ok=True)
    make_fighter().save(os.path.join(ROOT, "res", "sprites", "fighter.png"))
    make_ground().save(os.path.join(ROOT, "res", "sprites", "ground.png"))
    make_dojo().save(os.path.join(ROOT, "res", "bg", "dojo.png"))
    print("ok")


if __name__ == "__main__":
    main()
