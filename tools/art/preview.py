"""Apercus : planches de poses (solo ou paires) avec palettes de test."""
import numpy as np
from PIL import Image, ImageDraw

from render import Layer, render_fighter, remap, FIGHTER_MAP, to_vdp, GI_COSTUME

SKINS = [((252, 200, 160), (220, 150, 110), (160, 96, 64)),
         ((214, 150, 100), (176, 110, 68), (120, 70, 40)),
         ((150, 96, 60), (108, 64, 36), (70, 40, 20))]
HAIRS = [((70, 58, 52), (28, 22, 22)), ((140, 92, 48), (84, 52, 24)), ((236, 196, 96), (176, 126, 52))]
GIS = [((252, 252, 252), (204, 214, 234), (144, 158, 196), (84, 94, 136)),
       ((84, 144, 252), (44, 100, 224), (26, 62, 164), (14, 32, 98)),
       ((96, 96, 112), (64, 64, 78), (40, 40, 50), (22, 22, 28))]


def pal_fighter(skin=0, hair=0, gi=0, beard=False, acc=(230, 196, 40), bald=False):
    s, h, g = SKINS[skin], HAIRS[hair], GIS[gi]
    if bald:
        h = (s[0], s[1])
    pal = [(255, 0, 255), (16, 16, 40), s[0], s[1], s[2], h[0], h[1], g[0], g[1], g[2], g[3],
           (26, 26, 26), (h[1] if beard else s[1]), (200, 30, 30), (250, 250, 250), acc]
    return [to_vdp(c) for c in pal]


PAL_A = pal_fighter(0, 1, 0)
PAL_B = pal_fighter(1, 0, 1, beard=True)
BG = (64, 84, 104)


def layers_from_order(order):
    """[(who, group)...] -> [(who, [groups])...] en regroupant les runs consecutifs."""
    out = []
    for who, g in order:
        if out and out[-1][0] == who:
            out[-1][1].append(g)
        else:
            out.append((who, [g]))
    return out


def render_pair_rgb(A, B, order, W=176, H=112, pa=PAL_A, pb=PAL_B, bg=BG):
    out = np.zeros((H, W, 3), np.uint8)
    out[:] = bg
    for who, gs in layers_from_order(order):
        L = Layer(W, H)
        render_fighter(L, A if who == "A" else B, groups=gs)
        L.outline()
        idx = remap(L.idx, FIGHTER_MAP)
        pal = np.array(pa if who == "A" else pb, np.uint8)
        m = idx > 0
        out[m] = pal[idx[m]]
    return out


def render_solo_rgb(p, W=104, H=112, pal=PAL_A, bg=BG, cos=GI_COSTUME):
    L = Layer(W, H)
    render_fighter(L, p, cos=cos)
    L.outline()
    idx = remap(L.idx, FIGHTER_MAP)
    out = np.zeros((H, W, 3), np.uint8)
    out[:] = bg
    m = idx > 0
    out[m] = np.array(pal, np.uint8)[idx[m]]
    return out


def sheet(images, labels=None, cols=4, scale=2, floor=108):
    h, w = images[0].shape[:2]
    rows = (len(images) + cols - 1) // cols
    canvas = Image.new("RGB", (cols * w * scale, rows * (h * scale + 12)), (30, 30, 30))
    d = ImageDraw.Draw(canvas)
    for i, im in enumerate(images):
        x, y = (i % cols) * w * scale, (i // cols) * (h * scale + 12)
        pim = Image.fromarray(im).resize((w * scale, h * scale), Image.NEAREST)
        canvas.paste(pim, (x, y + 12))
        d.line([(x, y + 12 + floor * scale), (x + w * scale, y + 12 + floor * scale)], fill=(90, 70, 40))
        if labels:
            d.text((x + 3, y), str(labels[i]), fill=(255, 255, 255))
    return canvas
