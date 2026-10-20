#!/usr/bin/env python3
"""Exporte les sprites des combattants pour SGDK.

  res/sprites/solo.png            animations individuelles (cadre 128 x 112)
  res/sprites/pair_l0..2.png      animations en paire, 3 calques (cadre 176 x 112)
  src/gen/anim_data.h / .c        tables : sequences, durees, proprietaires des calques...
"""
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from render import Layer, render_fighter, remap, FIGHTER_MAP  # noqa: E402
from preview import PAL_A, layers_from_order  # noqa: E402
from skel import Frame  # noqa: E402
import solo  # noqa: E402
import pairs  # noqa: E402
import trans  # noqa: E402
from poses import shift  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SW, SH = 128, 112
SOLO_X = 56
PW, PH = 176, 112


def indexed(cells, cw, ch, cols_per_row):
    rows = len(cells)
    cols = max(len(r) for r in cells)
    im = np.zeros((rows * ch, cols * cw), np.uint8)
    for r, row in enumerate(cells):
        for c, cell in enumerate(row):
            im[r * ch:(r + 1) * ch, c * cw:(c + 1) * cw] = cell
    out = Image.fromarray(im, "P")
    flat = []
    for col in PAL_A:
        flat += list(col)
    out.putpalette(flat + [0] * (768 - len(flat)))
    return out


def render_solo(p):
    L = Layer(SW, SH)
    render_fighter(L, shift(p, SOLO_X - solo.X))
    L.outline()
    return remap(L.idx, FIGHTER_MAP)


def split3(order):
    runs = layers_from_order(order)
    if len(runs) > 3:
        # approximation : X avant, tout Y au milieu, reste de X devant
        x = runs[0][0]
        first = runs[0][1]
        mid = [g for w, gs in runs[1:] if w != x for g in gs]
        last = [g for w, gs in runs[1:] if w == x for g in gs]
        y = "A" if x == "B" else "B"
        print("  ordre simplifie : %s" % order, file=sys.stderr)
        runs = [(x, first), (y, mid), (x, last)]
    runs = [(w, list(g)) for w, g in runs]
    while len(runs) < 3:
        # coupe la plus longue couche en deux (sans changer la profondeur)
        i = max(range(len(runs)), key=lambda k: len(runs[k][1]))
        w, g = runs[i]
        if len(g) < 2:
            raise ValueError("impossible de remplir 3 couches : %s" % order)
        h = len(g) // 2
        runs[i:i + 1] = [(w, g[:h]), (w, g[h:])]
    return runs


def render_pair(A, B, order):
    imgs, owners = [], []
    for who, gs in split3(order):
        L = Layer(PW, PH)
        render_fighter(L, A if who == "A" else B, groups=gs)
        L.outline()
        idx = remap(L.idx, FIGHTER_MAP)
        if not (idx > 0).any():
            idx[PH - 1, PW // 2] = 1
            print("  attention : calque vide", file=sys.stderr)
        imgs.append(idx)
        owners.append(0 if who == "A" else 1)
    return imgs, owners


def all_pair_seqs():
    seqs = pairs.idle_seqs() if hasattr(pairs, "idle_seqs") else []
    seqs += [fn() for fn in pairs.TAKEDOWNS]
    seqs += trans.transitions()
    seqs += trans.submissions()
    return seqs


def owners_list(o):
    return [(o >> k) & 1 for k in range(3)]


def c_name(s):
    return "SEQ_" + s


def main():
    os.makedirs(os.path.join(ROOT, "res", "sprites"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "src", "gen"), exist_ok=True)

    # ------------------------------------------------ solo
    solo_rows, solo_meta = [], []
    for name, fn in solo.ANIMS:
        row, meta = [], []
        for p, ticks in fn():
            row.append(render_solo(p))
            meta.append((ticks, int(round(p.root[0] - solo.X))))
        solo_rows.append(row)
        solo_meta.append((name, meta))
    indexed(solo_rows, SW, SH, 0).save(os.path.join(ROOT, "res", "sprites", "solo.png"))

    # ------------------------------------------------ paires
    seqs = all_pair_seqs()
    rows = [[], [], []]
    seq_meta = []
    for s in seqs:
        frames = s.frames()
        r = [[], [], []]
        fmeta = []
        for A, B, order, ticks, fx in frames:
            imgs, owl = render_pair(A, B, order)
            owners = sum(v << k for k, v in enumerate(owl))
            for k in range(3):
                r[k].append(imgs[k])
            fa, fb = Frame(A), Frame(B)
            fmeta.append((ticks, owners | (fx << 3), (round(fa.P[0]), round(fa.P[1])), (round(fb.P[0]), round(fb.P[1]))))
        for k in range(3):
            rows[k].append(r[k])
        seq_meta.append((s, fmeta))
        print("  %-22s %2d images" % (s.name, len(frames)))
    for k in range(3):
        indexed(rows[k], PW, PH, 0).save(os.path.join(ROOT, "res", "sprites", "pair_l%d.png" % k))

    # ------------------------------------------------ C
    h = ["/* fichier genere par tools/art/build_fighters.py - ne pas modifier */",
         "#ifndef _ANIM_DATA_H_", "#define _ANIM_DATA_H_", "#include <genesis.h>", "",
         "#define SOLO_W %d" % SW, "#define SOLO_H %d" % SH, "#define SOLO_X %d" % SOLO_X,
         "#define PAIR_W %d" % PW, "#define PAIR_H %d" % PH, "#define PAIR_AX 88", "#define FLOOR_Y 108", "",
         "typedef struct { u8 ticks; s8 dx; } SoloFrame;",
         "typedef struct { u8 n; u8 loop; const SoloFrame* f; } SoloAnim;",
         "/* owners : bit k = 1 si le calque k appartient a B ; bits 3-4 : effet (1 impact, 2 poussiere) */",
         "typedef struct { u8 ticks; u8 owners; u8 ax, ay, bx, by; } PairFrame;",
         "typedef struct { u8 n; u8 loop; u8 endFlip; u8 swap; const PairFrame* f; } PairSeq;", "",
         "enum {"]
    for name, _ in solo_meta:
        h.append("    SOLO_%s," % name)
    h += ["    SOLO_COUNT", "};", "", "enum {"]
    for s, _ in seq_meta:
        h.append("    %s," % c_name(s.name))
    h += ["    SEQ_COUNT", "};", "",
          "extern const SoloAnim soloAnims[SOLO_COUNT];",
          "extern const PairSeq pairSeqs[SEQ_COUNT];", "", "#endif", ""]
    c = ["/* fichier genere par tools/art/build_fighters.py - ne pas modifier */", '#include "anim_data.h"', ""]
    for name, meta in solo_meta:
        c.append("static const SoloFrame sf_%s[] = { %s };" % (
            name, ", ".join("{%d,%d}" % m for m in meta)))
    c.append("const SoloAnim soloAnims[SOLO_COUNT] = {")
    loops = {"IDLE", "WALK", "GRIP_HOLD", "TIRED", "VICTORY", "DEFEAT"}
    for name, meta in solo_meta:
        c.append("    { %d, %d, sf_%s }," % (len(meta), 1 if name in loops else 0, name))
    c.append("};")
    c.append("")
    for s, fm in seq_meta:
        items = []
        for ticks, owners, a, b in fm:
            ow = owners
            clamp = lambda v: max(0, min(255, int(v)))
            items.append("{%d,%d,%d,%d,%d,%d}" % (ticks, ow, clamp(a[0]), clamp(a[1]), clamp(b[0]), clamp(b[1])))
        c.append("static const PairFrame pf_%s[] = { %s };" % (s.name, ", ".join(items)))
    c.append("const PairSeq pairSeqs[SEQ_COUNT] = {")
    for s, fm in seq_meta:
        c.append("    { %d, %d, %d, %d, pf_%s }," % (len(fm), 1 if s.loop else 0, s.end_flip, s.swap, s.name))
    c.append("};")
    open(os.path.join(ROOT, "src", "gen", "anim_data.h"), "w").write("\n".join(h))
    open(os.path.join(ROOT, "src", "gen", "anim_data.c"), "w").write("\n".join(c) + "\n")
    print("sequences : %d, images solo : %d" % (len(seq_meta), sum(len(m) for _, m in solo_meta)))


if __name__ == "__main__":
    main()
