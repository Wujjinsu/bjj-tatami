"""Rendu pixel art des combattants (style 16 bits) a partir d'une Pose.

Chaque piece (membre, torse, tete...) est evaluee en sur-echantillonnage, ombree comme un
volume eclaire en haut a gauche, quantifiee en 3-4 tons, puis composee de l'arriere vers
l'avant avec un liseré sombre (contour selectif) et un contour exterieur.

Les pixels contiennent des CODES de matiere, convertis ensuite en index de palette :
  1 contour | 2,3,4 peau (clair, moyen, ombre) | 5,6 cheveux | 7-10 veste (clair -> tres sombre)
  11 ceinture | 21 reflet ceinture | 12 barbe | 13 rouge | 14 blanc des yeux | 15 ecusson
  17-20 pantalon (memes tons que la veste chez les combattants)
"""
import math

import numpy as np

from skel import (Frame, HEAD_R, v_add, v_sub, v_mul, v_len, v_norm, ang_vec)

SS = 4
_L = np.array([-0.45, -0.72, 0.55])
LIGHT = _L / np.linalg.norm(_L)

OUT, SK_HI, SK_MID, SK_SH, HAIR_HI, HAIR_DK = 1, 2, 3, 4, 5, 6
GI = (7, 8, 9, 10)
PANTS = (17, 18, 19, 20)
BELT, BELT_HI, BEARD, RED, EYE, ACC = 11, 21, 12, 13, 14, 15
SKIN = (SK_HI, SK_MID, SK_SH)
HAIR = (HAIR_HI, HAIR_HI, HAIR_DK)

BAYER = np.array([[0, 2], [3, 1]]) / 4.0 - 0.375


class Costume:
    def __init__(self, sleeve=1.0, pants=1.0, belt=True, skirt=True, patch=True, hair=True,
                 cuffs=True, collar=True, tie=False, glasses=False, band=False, pocket=False,
                 shoes=False, goatee=False, brow=True, shine=False):
        self.tie = tie            # chemise blanche + cravate rouge
        self.glasses = glasses    # lunettes a monture noire
        self.band = band          # poignets rouges
        self.pocket = pocket      # ecusson blanc sur la poche
        self.shoes = shoes        # chaussures noires (au lieu de pieds nus)
        self.goatee = goatee      # moustache + bouc
        self.brow = brow
        self.shine = shine        # crane rase brillant
        self.sleeve = sleeve      # fraction de l'avant-bras couverte (0 = manches courtes)
        self.pants = pants
        self.belt = belt
        self.skirt = skirt
        self.patch = patch
        self.hair = hair
        self.cuffs = cuffs
        self.collar = collar


GI_COSTUME = Costume()
REF_COSTUME = Costume(sleeve=1.0, belt=False, skirt=False, patch=False, collar=False, cuffs=False,
                      hair=False, tie=True, glasses=True, band=True, pocket=True, shoes=True,
                      goatee=True, brow=False, shine=True)


# ---------------------------------------------------------------- primitives
class Prim:
    """Forme evaluable sur une grille de points : renvoie (dedans, normale_x, normale_y, nz, t)."""

    def bbox(self):
        raise NotImplementedError


class Cap(Prim):
    """Capsule effilee de A (rayon ra) a B (rayon rb)."""

    def __init__(self, a, b, ra, rb, bulge=(0.0, 0.0)):
        self.a, self.b, self.ra, self.rb = a, b, ra, rb

    def bbox(self):
        r = max(self.ra, self.rb) + 1
        return (min(self.a[0], self.b[0]) - r, min(self.a[1], self.b[1]) - r,
                max(self.a[0], self.b[0]) + r, max(self.a[1], self.b[1]) + r)

    def eval(self, X, Y):
        ax, ay = self.a
        dx, dy = self.b[0] - ax, self.b[1] - ay
        L2 = dx * dx + dy * dy or 1e-6
        t = np.clip(((X - ax) * dx + (Y - ay) * dy) / L2, 0, 1)
        cx, cy = ax + dx * t, ay + dy * t
        ox, oy = X - cx, Y - cy
        d = np.sqrt(ox * ox + oy * oy)
        r = self.ra + (self.rb - self.ra) * t
        inside = d <= r
        rr = np.maximum(r, 1e-3)
        nx, ny = ox / rr, oy / rr
        nz = np.sqrt(np.clip(1 - nx * nx - ny * ny, 0, 1))
        return inside, nx, ny, nz, t, d / rr


class Ell(Prim):
    """Ellipse (centre c, demi-axes ax le long de dir, ay en travers)."""

    def __init__(self, c, ax, ay, dirv=(1.0, 0.0)):
        self.c, self.ax, self.ay = c, ax, ay
        self.d = v_norm(dirv)

    def bbox(self):
        r = max(self.ax, self.ay) + 1
        return (self.c[0] - r, self.c[1] - r, self.c[0] + r, self.c[1] + r)

    def eval(self, X, Y):
        ox, oy = X - self.c[0], Y - self.c[1]
        du = (ox * self.d[0] + oy * self.d[1]) / self.ax
        dv = (-ox * self.d[1] + oy * self.d[0]) / self.ay
        q = du * du + dv * dv
        inside = q <= 1
        # normale dans le repere ecran
        nx = du * self.d[0] - dv * self.d[1]
        ny = du * self.d[1] + dv * self.d[0]
        nz = np.sqrt(np.clip(1 - q, 0, 1))
        return inside, nx, ny, nz, np.zeros_like(X), np.sqrt(q)


class Union(Prim):
    def __init__(self, prims):
        self.prims = prims

    def bbox(self):
        bs = [p.bbox() for p in self.prims]
        return (min(b[0] for b in bs), min(b[1] for b in bs), max(b[2] for b in bs), max(b[3] for b in bs))

    def eval(self, X, Y):
        best = None
        for i, p in enumerate(self.prims):
            ins, nx, ny, nz, t, q = p.eval(X, Y)
            q = np.where(ins, q, 9.0)
            if best is None:
                best = [ins, nx, ny, nz, t, q, np.zeros(X.shape, np.int8)]
            else:
                take = q < best[5]
                best[0] = best[0] | ins
                for k, v in ((1, nx), (2, ny), (3, nz), (4, t), (5, q)):
                    best[k] = np.where(take, v, best[k])
                best[6] = np.where(take, i, best[6])
        return best[0], best[1], best[2], best[3], best[4] + best[6], best[5]


# ---------------------------------------------------------------- calque
class Layer:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.idx = np.zeros((h, w), np.uint8)

    def _grid(self, bb):
        x0 = max(0, int(math.floor(bb[0])))
        y0 = max(0, int(math.floor(bb[1])))
        x1 = min(self.w, int(math.ceil(bb[2])) + 1)
        y1 = min(self.h, int(math.ceil(bb[3])) + 1)
        if x1 <= x0 or y1 <= y0:
            return None
        xs = x0 + (np.arange((x1 - x0) * SS) + 0.5) / SS
        ys = y0 + (np.arange((y1 - y0) * SS) + 0.5) / SS
        X, Y = np.meshgrid(xs, ys)
        return x0, y0, x1, y1, X, Y

    def draw(self, prim, tones, thresholds=None, darken=0.0, edge=True, edge_code=None, code_fn=None,
             clip=None, cover=0.5, dither=True):
        """tones : codes du plus clair au plus sombre. code_fn(t, inside_px) -> tableau de codes optionnel."""
        g = self._grid(prim.bbox())
        if g is None:
            return None
        x0, y0, x1, y1, X, Y = g
        ins, nx, ny, nz, t, q = prim.eval(X, Y)
        if clip is not None:
            ins = ins & clip(X, Y)
        shade = nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2] - darken
        h, w = y1 - y0, x1 - x0
        insr = ins.reshape(h, SS, w, SS)
        cov = insr.mean(axis=(1, 3))
        sh = np.where(ins, shade, 0).reshape(h, SS, w, SS).sum(axis=(1, 3)) / np.maximum(insr.sum(axis=(1, 3)), 1)
        tt = np.where(ins, t, 0).reshape(h, SS, w, SS).sum(axis=(1, 3)) / np.maximum(insr.sum(axis=(1, 3)), 1)
        mask = cov >= cover
        if dither:
            yy, xx = np.mgrid[y0:y1, x0:x1]
            sh = sh + BAYER[yy % 2, xx % 2] * 0.06
        if thresholds is None:
            thresholds = [0.62, 0.30, 0.0][: len(tones) - 1]
        code = np.full(sh.shape, tones[-1], np.uint8)
        for i in range(len(thresholds) - 1, -1, -1):
            code = np.where(sh > thresholds[i], tones[i], code)
        if code_fn is not None:
            code = code_fn(tt, code, x0, y0)
        region = self.idx[y0:y1, x0:x1]
        if edge:
            ring = np.zeros_like(mask)
            ring[1:, :] |= mask[:-1, :]
            ring[:-1, :] |= mask[1:, :]
            ring[:, 1:] |= mask[:, :-1]
            ring[:, :-1] |= mask[:, 1:]
            ring &= ~mask
            ec = edge_code if edge_code is not None else tones[-1]
            region[ring & (region > 0)] = ec
        region[mask] = code[mask]
        return mask, (x0, y0)

    def pixel(self, p, code, only_on=None):
        x, y = int(round(p[0] - 0.5)), int(round(p[1] - 0.5))
        if 0 <= x < self.w and 0 <= y < self.h:
            if only_on is None or self.idx[y, x] in only_on:
                self.idx[y, x] = code

    def line(self, a, b, code, only_on=None):
        n = int(max(abs(b[0] - a[0]), abs(b[1] - a[1]))) + 1
        for i in range(n + 1):
            t = i / max(n, 1)
            self.pixel((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t), code, only_on)

    def outline(self, code=OUT):
        m = self.idx > 0
        ring = np.zeros_like(m)
        ring[1:, :] |= m[:-1, :]
        ring[:-1, :] |= m[1:, :]
        ring[:, 1:] |= m[:, :-1]
        ring[:, :-1] |= m[:, 1:]
        ring &= ~m
        self.idx[ring] = code


# ---------------------------------------------------------------- dessin du combattant
GI_SET = set(GI) | set(PANTS)


def _arm(L, fr, side, cos, darken):
    S, E, W, Hh = fr.limbs["arm" + side]
    p = fr.p
    sl = cos.sleeve
    # manche : bras + avant-bras (une seule forme pour eviter les coutures)
    if sl > 0:
        cuff = v_add(E, v_mul(v_sub(W, E), sl))
        sleeve = Union([Cap(v_add(S, v_mul(v_norm(v_sub(E, S)), 1.5)), E, 5.6, 4.4), Cap(E, cuff, 4.4, 3.9)])
        L.draw(sleeve, GI, darken=darken)
        if cos.band:       # revers de manche rouge au poignet
            dvb = v_norm(v_sub(W, E))
            L.draw(Cap(v_add(W, v_mul(dvb, -3.4)), v_add(W, v_mul(dvb, 0.4)), 4.0, 3.7),
                   (RED, RED, HAIR_DK), thresholds=[0.55, 0.15], darken=darken, edge=False)
        # avant-bras nu + poignet
        if sl < 1:
            L.draw(Cap(cuff, W, 2.6, 2.2), SKIN, darken=darken)
        if cos.cuffs:
            dv = v_norm(v_sub(W, E))
            pv = (-dv[1], dv[0])
            c0 = v_add(cuff, v_mul(dv, -1.2))
            L.line(v_add(c0, v_mul(pv, -3.0)), v_add(c0, v_mul(pv, 3.0)), GI[3], only_on=GI_SET)
        L.draw(Cap(W, Hh, 2.1, 2.1), SKIN, darken=darken, edge=False)
    else:
        # manche courte (polo) : la manche couvre une partie du bras
        mid = v_add(S, v_mul(v_sub(E, S), -sl))
        L.draw(Cap(S, E, 3.3, 2.8), SKIN, darken=darken)
        L.draw(Cap(E, W, 2.7, 2.2), SKIN, darken=darken)
        L.draw(Cap(S, mid, 4.4, 4.1), GI, darken=darken)
    # poing
    L.draw(Ell(Hh, 3.4, 3.0, v_sub(Hh, W)), SKIN, darken=darken)
    fist = p.fist[0 if side == "N" else 1]
    if fist:
        dv = v_norm(v_sub(Hh, W))
        pv = (-dv[1], dv[0])
        L.line(v_add(Hh, v_mul(pv, -1.5)), v_add(v_add(Hh, v_mul(pv, 1.5)), v_mul(dv, 0.5)), SK_SH,
               only_on=set(SKIN))
    # coude : petit pli
    if sl >= 0.5:
        ang = abs(((math.degrees(math.atan2(*(v_sub(W, E))[::-1])) -
                    math.degrees(math.atan2(*(v_sub(E, S))[::-1])) + 180) % 360) - 180)
        if ang > 25:
            dv = v_norm(v_add(v_norm(v_sub(S, E)), v_norm(v_sub(W, E))))
            L.line(E, v_add(E, v_mul(dv, 3.0)), GI[3], only_on=GI_SET)
    # ecusson d'epaule
    if cos.patch and side == "N" and sl > 0:
        a = v_add(S, v_mul(v_sub(E, S), 0.30))
        b = v_add(S, v_mul(v_sub(E, S), 0.42))
        L.draw(Cap(a, b, 1.3, 1.3), (ACC,), edge=False, dither=False, cover=0.45)


def _leg(L, fr, side, cos, darken):
    Hh, K, A, T = fr.limbs["leg" + side]
    hem = v_add(K, v_mul(v_sub(A, K), 0.88 * cos.pants))
    jacket = v_add(Hh, v_mul(v_sub(K, Hh), 0.30))
    parts = Union([Cap(Hh, K, 6.8, 5.4), Cap(K, hem, 5.4, 4.7)])

    def code_fn(t, code, x0, y0):
        # t (de l'union) : 0..1 cuisse, 1..2 tibia. Haut de cuisse = pan de veste.
        out = code.copy()
        pants = np.isin(code, GI)
        tone_i = np.searchsorted(np.array(GI), code)
        tone_i = np.clip(tone_i, 0, 3)
        out = np.where(pants & (t > 0.30), np.array(PANTS, np.uint8)[tone_i], out)
        return out

    def all_pants(t, c, x0, y0):
        ti = np.clip(np.searchsorted(np.array(GI), c), 0, 3)
        return np.where(np.isin(c, GI), np.array(PANTS, np.uint8)[ti], c)

    B = v_add(fr.P, v_mul(fr.up, 3.0))

    def below_belt(X, Y):
        near = (X - Hh[0]) ** 2 + (Y - Hh[1]) ** 2 < 8.0 ** 2
        above = (X - B[0]) * fr.up[0] + (Y - B[1]) * fr.up[1] > -1.9
        return ~(near & above)

    L.draw(parts, GI, darken=darken, code_fn=code_fn if cos.skirt else all_pants,
           clip=below_belt if cos.belt else None)
    # ourlet de la veste sur la cuisse
    if cos.skirt:
        dv = v_norm(v_sub(K, Hh))
        pv = (-dv[1], dv[0])
        L.line(v_add(jacket, v_mul(pv, -6)), v_add(jacket, v_mul(pv, 6)), GI[3], only_on=GI_SET)
    # pli du genou
    dk = v_norm(v_add(v_norm(v_sub(Hh, K)), v_norm(v_sub(A, K))))
    if v_len(dk) > 0.1:
        L.line(K, v_add(K, v_mul(dk, 3.5)), PANTS[3], only_on=GI_SET)
    # ecusson de cuisse
    if cos.patch and side == "N":
        a = v_add(Hh, v_mul(v_sub(K, Hh), 0.50))
        b = v_add(Hh, v_mul(v_sub(K, Hh), 0.60))
        fw = fr.fw
        a = v_add(a, v_mul(fw, 1.5))
        b = v_add(b, v_mul(fw, 1.5))
        L.draw(Cap(a, b, 1.4, 1.4), (ACC,), edge=False, dither=False, cover=0.45)
    # cheville + pied
    fd = v_norm(v_sub(T, A))
    dn = v_mul(v_norm(v_sub(A, K)), 1.0)
    heel = v_add(v_add(A, v_mul(fd, -1.6)), v_mul(dn, 1.2))
    toe = v_add(T, v_mul(dn, 1.0))
    if cos.shoes:      # chaussettes + chaussures noires vernies
        L.draw(Cap(hem, A, 2.9, 2.7), (PANTS[1], PANTS[2], PANTS[3]), darken=darken)
        L.draw(Union([Cap(heel, v_add(toe, v_mul(fd, 0.6)), 2.8, 2.2)]), (BELT_HI, PANTS[2], PANTS[3]),
               thresholds=[0.72, 0.25], darken=darken)
        return
    L.draw(Cap(hem, A, 2.6, 2.4), SKIN, darken=darken)
    L.draw(Union([Cap(heel, toe, 2.4, 1.8)]), SKIN, darken=darken)


def _torso(L, fr, cos, darken=0.0):
    P, N, up, fw = fr.P, fr.N, fr.up, fr.fw
    chest = v_add(v_add(P, v_mul(up, 18)), v_mul(fw, 1.2))
    low = v_add(P, v_mul(up, -3.5 if cos.skirt else -1.0))
    neckb = v_add(N, v_mul(up, -1.0))
    body = Union([Cap(low, chest, 8.8, 10.4), Cap(chest, neckb, 10.4, 6.4)])
    L.draw(body, GI, darken=darken)
    if cos.belt:
        B = v_add(P, v_mul(up, 3.0))

        def slab(X, Y):
            d = (X - B[0]) * up[0] + (Y - B[1]) * up[1]
            return np.abs(d) <= 1.9

        L.draw(body, (BELT,), clip=slab, edge=False, dither=False)

        def slab_top(X, Y):
            d = (X - B[0]) * up[0] + (Y - B[1]) * up[1]
            return (d <= 1.9) & (d > 0.9)

        L.draw(body, (BELT_HI, BELT), thresholds=[0.35], clip=slab_top, edge=False, dither=False)
        # noeud + pans
        kn = v_add(B, v_mul(fw, 8.2))
        tail_dir = v_norm(v_add((0, 1), v_mul(fw, 0.35)))
        t1 = v_add(kn, v_mul(tail_dir, 8.0))
        t2 = v_add(kn, v_mul(v_norm(v_add(tail_dir, v_mul(fw, 0.5))), 6.5))
        L.draw(Cap(kn, t2, 1.5, 1.3), (BELT_HI, BELT, BELT), thresholds=[0.6, 0.0], dither=False)
        L.draw(Cap(kn, t1, 1.5, 1.4), (BELT_HI, BELT, BELT), thresholds=[0.6, 0.0], dither=False)
        L.pixel(v_add(t1, v_mul(tail_dir, -1.5)), RED)
        L.pixel(v_add(t1, v_mul(tail_dir, -2.5)), RED)
        L.draw(Ell(kn, 2.4, 2.0, up), (BELT_HI, BELT, BELT), thresholds=[0.45, 0.0], dither=False)
    if cos.collar:
        # revers : du cou vers la ceinture, cote poitrine
        top = v_add(v_add(N, v_mul(fw, 3.0)), v_mul(up, -1.5))
        bot = v_add(v_add(P, v_mul(up, 5.5)), v_mul(fw, 7.5))
        L.draw(Cap(top, bot, 1.6, 1.3), (GI[0], GI[0], GI[1]), thresholds=[0.2, -0.2], edge=False,
               dither=False, darken=darken)
        L.line(v_add(top, v_mul(fw, -1.8)), v_add(bot, v_mul(fw, -1.6)), GI[3], only_on=GI_SET)
        # col derriere la nuque
        L.draw(Cap(v_add(N, v_mul(fw, -4.5)), v_add(N, v_mul(fw, 2.0)), 1.8, 1.8), (GI[0], GI[1], GI[2]),
               thresholds=[0.3, -0.1], edge=False, dither=False, darken=darken)
        # plis au-dessus de la ceinture (dos)
        b0 = v_add(v_add(P, v_mul(up, 7.0)), v_mul(fw, -7.0))
        L.line(b0, v_add(b0, v_add(v_mul(fw, 3.5), v_mul(up, 1.5))), GI[3], only_on=GI_SET)
    else:
        # col de polo / de chemise
        L.draw(Cap(v_add(N, v_mul(fw, -4.0)), v_add(N, v_mul(fw, 3.5)), 1.8, 1.8), (EYE, EYE, EYE),
               edge=False, dither=False)
    if cos.tie:
        top = v_add(v_add(N, v_mul(fw, 3.4)), v_mul(up, -1.0))
        bot = v_add(v_add(P, v_mul(up, 6.5)), v_mul(fw, 8.0))
        mid = v_add(top, v_mul(v_sub(bot, top), 0.22))
        # plastron de chemise blanche (V du veston)
        L.draw(Cap(top, bot, 3.3, 1.4), (EYE, EYE, EYE), edge=False, dither=False, cover=0.4)
        # cravate rouge : noeud + lame qui s'evase
        L.draw(Cap(v_add(top, v_mul(fw, 0.4)), mid, 1.9, 1.7), (RED, RED, HAIR_DK), thresholds=[0.4, 0.0],
               edge=False, dither=False, cover=0.4)
        L.draw(Cap(mid, bot, 1.5, 2.4), (RED, RED, HAIR_DK), thresholds=[0.45, 0.05], edge=False,
               dither=False, cover=0.4)
        L.line(top, v_add(top, v_mul(fw, 0.0)), HAIR_DK)
    if cos.pocket:
        # ecusson blanc sur la poche de poitrine
        c = v_add(v_add(P, v_mul(up, 13.5)), v_mul(fw, 5.0))
        for dx in range(-2, 2):
            for dy in range(-1, 2):
                L.pixel(v_add(c, (dx, dy)), OUT, only_on=set(GI))
        for dx in range(-1, 1):
            for dy in range(0, 1):
                L.pixel(v_add(c, (dx, dy)), EYE, only_on=set(GI) | {OUT})


def _head(L, fr, cos, beard=False):
    p = fr.p
    N, C, hd, fd = fr.N, fr.C, fr.headDir, fr.faceDir
    # cou
    L.draw(Cap(v_add(N, v_mul(fr.up, -1.0)), v_add(C, v_mul(hd, -3.0)), 3.4, 3.2), SKIN)
    jaw_c = v_add(v_add(C, v_mul(fd, 2.6)), v_mul(hd, -3.3))
    head = Union([Ell(C, HEAD_R, HEAD_R * 0.96, fd), Ell(jaw_c, 4.6, 3.6, fd)])
    L.draw(head, SKIN)
    # oreille (en chou-fleur, comme il se doit)
    ear = v_add(v_add(C, v_mul(fd, -1.2)), v_mul(hd, -0.4))
    L.draw(Ell(ear, 2.4, 1.9, hd), (SK_MID, SK_MID, SK_SH), thresholds=[0.4, 0.0], edge=True, edge_code=SK_SH)
    L.pixel(v_add(ear, v_mul(fd, 0.3)), SK_SH)
    # cheveux : calotte haut / arriere
    if cos.hair:
        def hair_clip(X, Y):
            lf = (X - C[0]) * fd[0] + (Y - C[1]) * fd[1]
            lu = (X - C[0]) * hd[0] + (Y - C[1]) * hd[1]
            return (lu > 2.2 - 0.45 * lf) | ((lf < -2.8) & (lu > -3.2))
        L.draw(Ell(C, HEAD_R + 0.6, HEAD_R + 0.4, fd), HAIR, thresholds=[0.55, 0.15], clip=hair_clip,
               edge=False)
    # barbe (index dedie : couleur de peau si imberbe)
    def beard_clip(X, Y):
        lf = (X - C[0]) * fd[0] + (Y - C[1]) * fd[1]
        lu = (X - C[0]) * hd[0] + (Y - C[1]) * hd[1]
        return (lu < -3.6 + 0.15 * lf) & (lf > -1.5)
    if cos.goatee:     # moustache et joues : zone etendue + bouc sous le menton
        def beard_clip(X, Y, _b=beard_clip):   # noqa: F811
            lf = (X - C[0]) * fd[0] + (Y - C[1]) * fd[1]
            lu = (X - C[0]) * hd[0] + (Y - C[1]) * hd[1]
            return _b(X, Y) | ((lu < -1.7) & (lu > -4.2) & (lf > 3.0))
    L.draw(head, (BEARD,), clip=beard_clip, edge=False, dither=False, cover=0.55)
    if cos.goatee:
        g0 = v_add(v_add(C, v_mul(fd, 4.6)), v_mul(hd, -5.6))
        g1 = v_add(v_add(C, v_mul(fd, 5.0)), v_mul(hd, -8.6))
        L.draw(Cap(g0, g1, 2.0, 1.2), (BEARD,), edge=False, dither=False, cover=0.45)
    if cos.shine:      # crane rase : reflet
        sp = v_add(v_add(C, v_mul(fd, 0.6)), v_mul(hd, HEAD_R - 1.2))
        L.pixel(sp, EYE, only_on=set(SKIN))
        L.pixel(v_add(sp, v_mul(fd, -1.0)), EYE, only_on=set(SKIN))
    # visage
    eye = v_add(v_add(C, v_mul(fd, 4.4)), v_mul(hd, 0.9))
    if p.face == 2:
        L.line(v_add(eye, v_mul(fd, -0.8)), v_add(eye, v_mul(fd, 0.8)), OUT)
    else:
        L.pixel(v_add(eye, v_mul(fd, -0.9)), EYE)
        L.pixel(eye, OUT)
    if cos.brow:
        brow_a = v_add(v_add(C, v_mul(fd, 3.0)), v_mul(hd, 2.4))
        brow_b = v_add(v_add(C, v_mul(fd, 6.2)), v_mul(hd, 1.6))
        L.line(brow_a, brow_b, HAIR_DK)
    if cos.glasses:    # monture noire rectangulaire + branche vers l'oreille
        e0 = v_add(eye, v_mul(fd, -1.9))
        e1 = v_add(eye, v_mul(fd, 2.1))
        for a, b in ((v_add(e0, v_mul(hd, 1.7)), v_add(e1, v_mul(hd, 1.7))),
                     (v_add(e0, v_mul(hd, -1.7)), v_add(e1, v_mul(hd, -1.7))),
                     (v_add(e0, v_mul(hd, 1.7)), v_add(e0, v_mul(hd, -1.7))),
                     (v_add(e1, v_mul(hd, 1.7)), v_add(e1, v_mul(hd, -1.7)))):
            L.line(a, b, OUT)
        L.line(e0, v_add(v_add(C, v_mul(fd, -1.0)), v_mul(hd, 1.0)), OUT)
        L.pixel(v_add(eye, v_mul(fd, -0.6)), EYE)
        L.pixel(v_add(eye, v_mul(fd, 0.7)), OUT)
    nose = v_add(v_add(C, v_mul(fd, 7.4)), v_mul(hd, -0.4))
    L.pixel(nose, SK_MID)
    L.pixel(v_add(nose, v_mul(hd, -1.0)), SK_SH)
    mouth = v_add(v_add(C, v_mul(fd, 6.0)), v_mul(hd, -3.6))
    if p.face in (1, 3):
        L.line(v_add(mouth, v_mul(fd, -1.5)), v_add(mouth, v_mul(fd, 0.6)), EYE)
        L.pixel(v_add(mouth, v_mul(hd, -1.0)), OUT)
        if p.face == 3:
            L.pixel(v_add(mouth, v_add(v_mul(hd, -1.0), v_mul(fd, -1.0))), RED)
    else:
        L.line(v_add(mouth, v_mul(fd, -1.3)), v_add(mouth, v_mul(fd, 0.3)), SK_SH)


def render_fighter(L: Layer, pose, groups=("legF", "armF", "body", "legN", "armN"), cos=GI_COSTUME,
                   far_dark=0.24):
    fr = Frame(pose)
    for g in groups:
        if g == "legF":
            _leg(L, fr, "F", cos, far_dark)
        elif g == "armF":
            _arm(L, fr, "F", cos, far_dark)
        elif g == "legN":
            _leg(L, fr, "N", cos, 0.0)
        elif g == "armN":
            _arm(L, fr, "N", cos, 0.0)
        elif g == "body":
            _torso(L, fr, cos)
            _head(L, fr, cos)
        elif g == "torso":
            _torso(L, fr, cos)
        elif g == "head":
            _head(L, fr, cos)
    return fr


# ---------------------------------------------------------------- palettes d'apercu
def to_vdp(c):
    """arrondit une couleur RGB sur la grille Mega Drive (3 bits par canal)."""
    return tuple(int(round(v / 255 * 7)) * 255 // 7 for v in c)


FIGHTER_MAP = {i: i for i in range(16)}
FIGHTER_MAP.update({17: 7, 18: 8, 19: 9, 20: 10, 21: 10})


def remap(idx, mapping):
    out = np.zeros_like(idx)
    for k, v in mapping.items():
        out[idx == k] = v
    return out
