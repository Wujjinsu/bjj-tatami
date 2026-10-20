#!/usr/bin/env python3
"""Interface et effets (palette PAL3 partagee) :

  res/ui/font_gold.png, font_white.png  polices 8x8 (caracteres 32..127)
  res/ui/hud.png                        chiffres geants, cases de score, drapeaux, icones
  res/sprites/banner.png                grands mots (zoom d'apparition)
  res/sprites/pops.png                  +2 +3 +4 AV PEN...
  res/sprites/fx.png                    impact, poussiere, sueur, danger, ombres
  res/sprites/referee.png               l'arbitre
  src/gen/ui_data.h                     index des tuiles
"""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font import glyph, BIG  # noqa: E402
from render import Layer, render_fighter, REF_COSTUME, remap, Cap, Ell, to_vdp  # noqa: E402
import solo  # noqa: E402
from poses import stand, shift  # noqa: E402
from skel import Frame, ik_arm, look  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

PAL3 = [(255, 0, 255), (0, 0, 0), (252, 252, 252), (182, 182, 182), (100, 100, 108), (218, 36, 36),
        (128, 0, 0), (36, 182, 72), (0, 100, 36), (252, 218, 36), (252, 128, 0), (36, 100, 218),
        (0, 36, 128), (252, 182, 145), (182, 110, 72), (36, 28, 72)]
PAL3 = [to_vdp(c) for c in PAL3]
K, WH, LG, DG, RD, DR, GR, DGR, YE, OR, BL, DBL, SK, SKS, NV = range(1, 16)


def save_idx(a, path):
    im = Image.fromarray(a.astype(np.uint8), "P")
    flat = []
    for c in PAL3:
        flat += list(c)
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.save(path)


# ---------------------------------------------------------------- polices
def font_sheet(rows_colors):
    a = np.zeros((48, 128), np.uint8)
    for code in range(32, 128):
        ch = chr(code)
        g = glyph(ch) if ch in __import__("font").GLYPHS else None
        if g is None:
            continue
        cx, cy = ((code - 32) % 16) * 8, ((code - 32) // 16) * 8
        for y in range(7):
            for x in range(6):
                if g[y][x]:
                    a[cy + y + 1, cx + x + 1] = K       # ombre
        for y in range(7):
            for x in range(6):
                if g[y][x]:
                    a[cy + y, cx + x] = rows_colors[y]
    return a


# ---------------------------------------------------------------- HUD
class Tiles:
    def __init__(self):
        self.tiles = []
        self.names = {}

    def add(self, name, arr):
        """arr : multiple de 8x8 ; tuiles ajoutees colonne par colonne puis ligne (ordre ligne)."""
        h, w = arr.shape
        idx = len(self.tiles)
        for ty in range(h // 8):
            for tx in range(w // 8):
                self.tiles.append(arr[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8].copy())
        self.names[name] = (idx, w // 8, h // 8)
        return idx

    def sheet(self):
        n = len(self.tiles)
        cols = 16
        rows = (n + cols - 1) // cols
        a = np.zeros((rows * 8, cols * 8), np.uint8)
        for i, t in enumerate(self.tiles):
            a[(i // cols) * 8:(i // cols + 1) * 8, (i % cols) * 8:(i % cols + 1) * 8] = t
        return a


def big_digit(ch, fg, bg, shadow=K, hi=None):
    a = np.full((16, 8), bg, np.uint8)
    rows = BIG[ch]
    for y, line in enumerate(rows):
        for x, c in enumerate(line):
            if c == "#":
                a[y + 2, x + 1] = shadow
    for y, line in enumerate(rows):
        for x, c in enumerate(line):
            if c == "#":
                col = fg
                if hi is not None and y < 6:
                    col = hi
                a[y + 1, x] = col
    return a


def small_on(ch, fg, bg):
    a = np.full((8, 8), bg, np.uint8)
    g = glyph(ch)
    for y in range(7):
        for x in range(6):
            if g[y][x]:
                a[y + 1, x + 1] = fg
    return a


def bevel(a, top, bottom):
    a = a.copy()
    a[0, :] = np.where(a[0, :] == a[1, 3], top, a[0, :]) if False else a[0, :]
    return a


def flag(code):
    a = np.zeros((16, 16), np.uint8)
    f = np.zeros((11, 15), np.uint8)
    W_, H_ = 15, 11
    if code == "BRA":
        f[:] = GR
        for y in range(H_):
            half = int(round(6.5 * (1 - abs(y - 5) / 5.5)))
            f[y, 7 - half:8 + half] = YE
        f[3:8, 5:10] = BL
        f[4:7, 4:11] = np.where(f[4:7, 4:11] == YE, BL, f[4:7, 4:11])
        f[5, 5:10] = WH
    elif code == "JPN":
        f[:] = WH
        yy, xx = np.mgrid[0:H_, 0:W_]
        f[((yy - 5) ** 2 + (xx - 7) ** 2) <= 9] = RD
    elif code in ("FRA", "ITA", "MLI", "SEN"):
        cols = {"FRA": (BL, WH, RD), "ITA": (GR, WH, RD), "MLI": (GR, YE, RD), "SEN": (GR, YE, RD)}[code]
        f[:, 0:5], f[:, 5:10], f[:, 10:15] = cols
        if code == "SEN":
            f[4:7, 7] = GR
            f[5, 6:9] = GR
    elif code == "UKR":
        f[0:6, :], f[6:, :] = BL, YE
    elif code == "HUN":
        f[0:4, :], f[4:7, :], f[7:, :] = RD, WH, GR
    elif code == "POL":
        f[0:6, :], f[6:, :] = WH, RD
    elif code == "POR":
        f[:, 0:6], f[:, 6:] = GR, RD
        f[3:8, 4:8] = YE
        f[4:7, 5:7] = RD
    elif code == "ALG":
        f[:, 0:8], f[:, 8:] = GR, WH
        yy, xx = np.mgrid[0:H_, 0:W_]
        ring = ((yy - 5) ** 2 + (xx - 7.5) ** 2 <= 12) & ((yy - 5) ** 2 + (xx - 8.8) ** 2 > 7)
        f[ring] = RD
        f[5, 10] = RD
    elif code == "LBN":
        f[:] = WH
        f[0:3, :], f[8:, :] = RD, RD
        f[3:8, 7] = GR
        f[4:7, 6:9] = GR
        f[5, 5:10] = GR
    elif code == "CAN":
        f[:] = WH
        f[:, 0:4], f[:, 11:] = RD, RD
        f[3:8, 7] = RD
        f[4:7, 5:10] = RD
        f[3, 6:9] = RD
    elif code == "NOR":
        f[:] = RD
        f[:, 4:7], f[4:7, :] = WH, WH
        f[:, 5], f[5, :] = DBL, DBL
    a[2:13, 0:15] = f
    a[1, 0:16] = K
    a[13, 0:16] = K
    a[1:14, 15] = K
    a[2:13, 0] = np.where(a[2:13, 0] == 0, K, a[2:13, 0])
    return a


FLAGS = ["BRA", "JPN", "FRA", "SEN", "UKR", "ITA", "ALG", "HUN", "POR", "LBN", "CAN", "NOR", "MLI", "POL"]


def button_icon(letter, color):
    a = np.zeros((8, 8), np.uint8)
    yy, xx = np.mgrid[0:8, 0:8]
    a[((yy - 3.5) ** 2 + (xx - 3.5) ** 2) <= 14] = K
    a[((yy - 3.5) ** 2 + (xx - 3.5) ** 2) <= 9] = color
    g = {"A": ["##.", "#.#", "###", "#.#"], "B": ["##.", "###", "#.#", "##."], "C": [".##", "#..", "#..", ".##"]}[letter]
    for y, l in enumerate(g):
        for x, c in enumerate(l):
            if c == "#":
                a[2 + y, 2 + x] = WH
    return a


def build_hud():
    T = Tiles()
    T.add("blank", np.zeros((8, 8), np.uint8))
    # chiffres geants : points (fond vert), chrono (fond nuit)
    for name, fg, bg, hi in (("big_pts", WH, GR, None), ("big_time", YE, NV, WH)):
        arr = np.concatenate([big_digit(c, fg, bg, hi=hi) for c in "0123456789:"], axis=1)
        # tuiles en colonnes : haut puis bas pour chaque chiffre
        idx = len(T.tiles)
        for i in range(11):
            T.tiles.append(arr[0:8, i * 8:(i + 1) * 8].copy())
            T.tiles.append(arr[8:16, i * 8:(i + 1) * 8].copy())
        T.names[name] = (idx, 11, 2)
    # biseau des cases (bord haut / bas de la case des points)
    for name, top, mid, bot in (("box_pts", (WH, GR), GR, DGR), ("box_time", (LG, NV), NV, K)):
        a = np.full((8, 8), mid, np.uint8)
        a[0, :] = top[0]
        T.add(name + "_top", a)
        a = np.full((8, 8), mid, np.uint8)
        a[7, :] = bot
        T.add(name + "_bot", a)
        T.add(name + "_fill", np.full((8, 8), mid, np.uint8))
    # avantages (jaune) et penalites (rouge) : etiquette + chiffres
    T.add("adv_lab", small_on("A", K, YE))
    T.add("adv_dig", np.concatenate([small_on(c, K, YE) for c in "0123456789"], axis=1))
    T.add("pen_lab", small_on("P", WH, RD))
    T.add("pen_dig", np.concatenate([small_on(c, WH, RD) for c in "0123456789"], axis=1))
    for code in FLAGS:
        T.add("flag_" + code, flag(code))
    T.add("btn_a", button_icon("A", DR))
    T.add("btn_b", button_icon("B", DBL))
    T.add("btn_c", button_icon("C", DGR))
    # mini-jeu de soumission : cases de la barre et cadenas des portes
    def cell(fill, top, edge=K):
        a = np.full((8, 8), NV, np.uint8)
        a[1:7, 0:8] = fill
        a[1, :] = top
        a[0, :] = edge
        a[7, :] = edge
        return a
    T.add("sub_empty", cell(NV, DBL))
    T.add("sub_att", cell(RD, OR))
    T.add("sub_def", cell(GR, WH))
    T.add("sub_both", cell(YE, WH))
    a = np.zeros((8, 8), np.uint8)
    a[0:3, 2:6] = DG
    a[1:3, 3:5] = 0
    a[3:8, 1:7] = DG
    a[4:7, 2:6] = LG
    T.add("gate_off", a)
    a2 = a.copy()
    a2[a2 == DG] = OR
    a2[a2 == LG] = YE
    T.add("gate_on", a2)
    # cadre de jauge
    a = np.zeros((8, 8), np.uint8)
    a[1:7, 0] = K
    a[0, :] = 0
    T.add("bar_capl", a)
    a = np.zeros((8, 8), np.uint8)
    a[1:7, 7] = K
    T.add("bar_capr", a)
    # panneau (barre d'aide en bas, fond de menus)
    T.add("panel", np.full((8, 8), NV, np.uint8))
    a = np.full((8, 8), NV, np.uint8)
    a[0, :] = YE
    a[1, :] = OR
    T.add("panel_top", a)
    a = np.full((8, 8), NV, np.uint8)
    a[7, :] = YE
    a[6, :] = OR
    T.add("panel_bot", a)
    return T


# ---------------------------------------------------------------- sprites PAL3
def text_img(txt, size, grad=(WH, YE, YE, OR, OR, RD), outline=K, thick=2):
    fnt = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)
    tmp = Image.new("L", (size * len(txt) + 20, size * 2), 0)
    d = ImageDraw.Draw(tmp)
    d.fontmode = "1"
    d.text((10, size // 3), txt, font=fnt, fill=255)
    m = np.array(tmp) > 0
    ys, xs = np.where(m)
    m = m[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    h, w = m.shape
    pad = thick + 2
    out = np.zeros((h + 2 * pad, w + 2 * pad), np.uint8)
    core = np.zeros_like(out, bool)
    core[pad:pad + h, pad:pad + w] = m
    # contour epais
    dil = core.copy()
    for _ in range(thick):
        d2 = dil.copy()
        d2[1:, :] |= dil[:-1, :]
        d2[:-1, :] |= dil[1:, :]
        d2[:, 1:] |= dil[:, :-1]
        d2[:, :-1] |= dil[:, 1:]
        dil = d2
    # ombre portee
    sh = np.zeros_like(dil)
    sh[2:, 2:] = dil[:-2, :-2]
    out[sh] = K
    out[dil] = outline
    rows = np.where(core.any(axis=1))[0]
    y0, y1 = rows.min(), rows.max()
    for y in range(out.shape[0]):
        t = (y - y0) / max(1, (y1 - y0))
        gi = int(np.clip(t * len(grad), 0, len(grad) - 1))
        out[y, core[y]] = grad[gi]
    # reflet : premiere ligne de chaque colonne blanche
    return out


def fit(a, w, h):
    out = np.zeros((h, w), np.uint8)
    ah, aw = a.shape
    if aw > w or ah > h:
        a = a[:min(ah, h), :min(aw, w)]
        ah, aw = a.shape
    x0, y0 = (w - aw) // 2, (h - ah) // 2
    out[y0:y0 + ah, x0:x0 + aw] = a
    return out


def scale_nn(a, s):
    h, w = a.shape
    nh, nw = max(1, int(round(h * s))), max(1, int(round(w * s)))
    ys = (np.arange(nh) / s).astype(int).clip(0, h - 1)
    xs = (np.arange(nw) / s).astype(int).clip(0, w - 1)
    return a[ys][:, xs]


BANNERS = [("OSS!", 30), ("COMBATE!", 30), ("FINALIZOU!", 28), ("TEMPO!", 30), ("VITORIA!", 30),
           ("EMPATE", 30), ("DESCLASSIFICADO", 17), ("PERFEITO!", 28), ("CAMPEAO!", 30), ("FINAL", 30),
           ("PRONTO?", 30), ("PAROU!", 30)]
BW, BH = 248, 48


def build_banners():
    rows = []
    for word, size in BANNERS:
        base = text_img(word, size)
        frames = []
        for s in (0.5, 1.25, 1.08, 1.0):
            sc = scale_nn(base, s)
            frames.append(fit(sc, BW, BH))
        rows.append(frames)
    a = np.zeros((len(rows) * BH, 4 * BW), np.uint8)
    for r, fr in enumerate(rows):
        for c, f in enumerate(fr):
            a[r * BH:(r + 1) * BH, c * BW:(c + 1) * BW] = f
    return a


POPS = ["+2", "+3", "+4", "AV", "PEN", "-2", "OSS", "!"]


def build_pops():
    cells = []
    for p in POPS:
        if p == "!":
            img = text_img("!", 18, grad=(WH, YE, OR, RD), thick=1)
        elif p in ("PEN", "-2"):
            img = text_img(p, 12, grad=(WH, WH, LG), outline=RD, thick=1)
        elif p == "AV":
            img = text_img(p, 13, grad=(WH, YE, YE, OR), outline=K, thick=1)
        else:
            img = text_img(p, 14, grad=(WH, YE, YE, OR), outline=DGR, thick=1)
        cells.append(fit(img, 32, 24))
    return np.concatenate(cells, axis=1)


def build_fx():
    """Grille de 32x32 : impact (4), poussiere (4), sueur (2), ombre solo (1), ombre paire (2 cellules)."""
    cells = []
    yy, xx = np.mgrid[0:32, 0:32]
    r = np.hypot(yy - 15.5, xx - 15.5)
    ang = np.arctan2(yy - 15.5, xx - 15.5)
    for k, rad in enumerate((6, 12, 15, 13)):
        a = np.zeros((32, 32), np.uint8)
        spikes = rad * (0.55 + 0.45 * np.abs(np.cos(ang * 4 + k * 0.4)))
        a[r <= spikes] = OR if k < 3 else RD
        a[r <= spikes * 0.72] = YE
        a[r <= spikes * 0.42] = WH
        if k == 3:
            a[(r <= spikes) & ((yy + xx) % 2 == 0)] = 0
        cells.append(a)
    for k in range(4):
        a = np.zeros((32, 32), np.uint8)
        for cx, cy, rr in ((10 - k * 2, 24, 4 + k), (22 + k * 2, 24, 4 + k), (16, 21 - k, 5 + k)):
            m = np.hypot(yy - cy, xx - cx) <= rr
            a[m] = LG if k < 2 else DG
            a[m & (np.hypot(yy - cy + 1, xx - cx + 1) <= rr - 1.5)] = WH if k < 2 else LG
        if k >= 2:
            a[(a > 0) & ((yy + xx) % 2 == 0)] = 0
        cells.append(a)
    for k in range(2):
        a = np.zeros((32, 32), np.uint8)
        for cx, cy in ((10, 10 + k * 4), (20, 6 + k * 5)):
            m = (np.hypot((yy - cy) * 0.8, xx - cx) <= 2.2) | ((yy < cy) & (yy > cy - 4) & (np.abs(xx - cx) <= (yy - (cy - 4)) * 0.5))
            a[m] = BL
            a[m & (xx < cx) & (yy < cy)] = WH
        cells.append(a)
    # ombres tramees (noir un pixel sur deux)
    a = np.zeros((32, 32), np.uint8)
    m = ((yy - 28) / 3.2) ** 2 + ((xx - 15.5) / 15) ** 2 <= 1
    a[m & ((yy + xx) % 2 == 0)] = K
    cells.append(a)
    return np.concatenate(cells, axis=1)


def build_shadow_pair():
    yy, xx = np.mgrid[0:16, 0:128]
    a = np.zeros((16, 128), np.uint8)
    m = ((yy - 11) / 3.6) ** 2 + ((xx - 63.5) / 62) ** 2 <= 1
    a[m & ((yy + xx) % 2 == 0)] = K
    return a


# ---------------------------------------------------------------- arbitre
REF_MAP = {0: 0, 1: K, 2: SK, 3: SK, 4: SKS, 5: K, 6: K, 7: DG, 8: K, 9: K, 10: K, 11: K, 12: SKS,
           13: RD, 14: WH, 15: WH, 17: LG, 18: LG, 19: DG, 20: DG, 21: DG}
RW, RH, RX = 96, 112, 48


def ref_pose(kind, k=0):
    X = RX
    p = stand(X, h=36.5, lean=4, step_n=7, step_f=-7)
    fr = Frame(p)
    S = fr.S
    if kind == "idle":
        p = ik_arm(p, "N", (fr.P[0] + 6, fr.P[1] + 6 + k), 1)
        p = ik_arm(p, "F", (fr.P[0] - 4, fr.P[1] + 7 + k), 1)
    elif kind == "walk":
        p = stand(X, h=36.5, lean=6, step_n=[10, 3, -6, 3][k], step_f=[-8, 1, 9, 1][k])
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.P[0] + [-4, 2, 8, 2][k], fr.P[1] + 7), 1)
        p = ik_arm(p, "F", (fr.P[0] + [8, 2, -4, 2][k], fr.P[1] + 7), 1)
    elif kind == "points":     # bras leve droit au-dessus de la tete
        p = ik_arm(p, "N", (S[0] + 3, S[1] - 27), 1)
        p = ik_arm(p, "F", (fr.P[0] - 4, fr.P[1] + 7), 1)
    elif kind == "adv":        # bras tendu a l'horizontale
        p = ik_arm(p, "N", (S[0] + 29, S[1] + 1), 1)
        p = ik_arm(p, "F", (fr.P[0] - 4, fr.P[1] + 7), 1)
    elif kind == "pen":        # pointe le fautif, l'autre main sur le poing
        p = ik_arm(p, "N", (S[0] + 27, S[1] + 6 - 2 * k), 1)
        p = ik_arm(p, "F", (S[0] + 12, S[1] + 4), 1)
    elif kind == "stop":       # deux bras en avant, paumes ouvertes
        p = ik_arm(p, "N", (S[0] + 24, S[1] - 6), 1)
        p = ik_arm(p, "F", (S[0] + 22, S[1] - 2), 1)
        p = p.copy(fist=(0, 0))
    elif kind == "start":      # coupe vers le bas
        if k == 0:
            p = ik_arm(p, "N", (S[0] + 10, S[1] - 22), 1)
        else:
            p = ik_arm(p, "N", (S[0] + 22, S[1] + 14), 1)
        p = ik_arm(p, "F", (fr.P[0] - 4, fr.P[1] + 7), 1)
        p = p.copy(face=3 if k else 0)
    elif kind == "winner":     # leve le bras du vainqueur (main tendue en haut, vers l'avant)
        p = ik_arm(p, "N", (S[0] + 14, S[1] - 22), 1)
        p = ik_arm(p, "F", (fr.P[0] - 4, fr.P[1] + 7), 1)
    return look(p, 0)


REF_ANIMS = [("IDLE", "idle", 2), ("WALK", "walk", 4), ("POINTS", "points", 1), ("ADV", "adv", 1),
             ("PEN", "pen", 2), ("STOP", "stop", 1), ("START", "start", 2), ("WINNER", "winner", 1)]


def build_referee():
    rows = []
    for name, kind, n in REF_ANIMS:
        row = []
        for k in range(n):
            L = Layer(RW, RH)
            render_fighter(L, ref_pose(kind, k), cos=REF_COSTUME)
            L.outline(1)
            row.append(remap(L.idx, REF_MAP))
        rows.append(row)
    cols = max(len(r) for r in rows)
    a = np.zeros((len(rows) * RH, cols * RW), np.uint8)
    for r, row in enumerate(rows):
        for c, f in enumerate(row):
            a[r * RH:(r + 1) * RH, c * RW:(c + 1) * RW] = f
    return a


def main():
    os.makedirs(os.path.join(ROOT, "res", "ui"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "res", "sprites"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "src", "gen"), exist_ok=True)
    save_idx(font_sheet([WH, YE, YE, YE, OR, OR, OR]), os.path.join(ROOT, "res", "ui", "font_gold.png"))
    save_idx(font_sheet([WH, WH, WH, WH, LG, LG, LG]), os.path.join(ROOT, "res", "ui", "font_white.png"))
    T = build_hud()
    save_idx(T.sheet(), os.path.join(ROOT, "res", "ui", "hud.png"))
    save_idx(build_banners(), os.path.join(ROOT, "res", "sprites", "banner.png"))
    save_idx(build_pops(), os.path.join(ROOT, "res", "sprites", "pops.png"))
    save_idx(build_fx(), os.path.join(ROOT, "res", "sprites", "fx.png"))
    save_idx(build_shadow_pair(), os.path.join(ROOT, "res", "sprites", "shadow_pair.png"))
    save_idx(build_referee(), os.path.join(ROOT, "res", "sprites", "referee.png"))

    h = ["/* genere par tools/art/build_ui.py */", "#ifndef _UI_DATA_H_", "#define _UI_DATA_H_", ""]
    for name, (idx, w, hh) in T.names.items():
        h.append("#define HT_%s %d" % (name.upper(), idx))
    h.append("#define HUD_TILE_COUNT %d" % len(T.tiles))
    h.append("")
    h.append("enum { %s, FLAG_COUNT };" % ", ".join("FLAG_" + f for f in FLAGS))
    h.append("enum { %s, BANNER_COUNT };" % ", ".join(
        "BAN_" + w.strip("!?").replace(" ", "_") for w, _ in BANNERS))
    h.append("enum { POP_P2, POP_P3, POP_P4, POP_AV, POP_PEN, POP_M2, POP_OSS, POP_DANGER, POP_COUNT };")
    h.append("enum { %s, REF_COUNT };" % ", ".join("REF_" + n for n, _, _ in REF_ANIMS))
    h.append("static const u8 refFrames[REF_COUNT] = { %s };" % ", ".join(str(n) for _, _, n in REF_ANIMS))
    h.append("#define FX_IMPACT 0")
    h.append("#define FX_DUST 4")
    h.append("#define FX_SWEAT 8")
    h.append("#define FX_SHADOW 10")
    h.append("#define REF_X %d" % RX)
    h += ["", "#endif", ""]
    open(os.path.join(ROOT, "src", "gen", "ui_data.h"), "w").write("\n".join(h))
    pal = ", ".join("0x%03X" % (((c[2] >> 5) << 9) | ((c[1] >> 5) << 5) | ((c[0] >> 5) << 1)) for c in PAL3)
    open(os.path.join(ROOT, "src", "gen", "pal3.h"), "w").write(
        "/* genere par tools/art/build_ui.py : palette partagee PAL3 (interface, arbitre, effets) */\n"
        "static const u16 pal3Colors[16] = { %s };\n" % pal)
    print("hud tuiles :", len(T.tiles))


if __name__ == "__main__":
    main()
