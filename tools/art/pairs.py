"""Animations en paire (cadre 176 x 112, ancre x = 88, sol y = 108).

Convention canonique : A = attaquant / combattant du dessus, a DROITE, regarde a GAUCHE.
B = defenseur / combattant du dessous, a GAUCHE ; au sol B est sur le dos, tete a GAUCHE.
Les etiquettes A / B designent les roles AU DEBUT de la sequence.

end_flip : l'etat final est le miroir de l'etat canonique (le jeu inverse son affichage).
swap     : a la fin, le combattant du dessus a change (A est passe dessous).
"""
import ground as G
from poses import stand, supine, prone, FLOOR, ANKLE, mirror_x, shift, jpos
from skel import Pose, Frame, ik_arm, ik_leg, foot_flat, look, wrap, interp, ease

AXIS = 88.0
GAP_GRIP = 46.0
GAP_FAR = 62.0


# ---------------------------------------------------------------- outils
def mirror_state(st):
    A, B, order = st
    return mirror_x(A, AXIS), mirror_x(B, AXIS), order


def swap_state(st):
    """(dessus, dessous, ordre) -> le premier combattant devient B, le second A."""
    top, bot, order = st
    sw = [("B" if w == "A" else "A", g) for w, g in order]
    return bot, top, sw


def solo_order(front_first="B"):
    other = "A" if front_first == "B" else "B"
    return [(front_first, g) for g in ("legF", "armF", "body", "legN", "armN")] + \
           [(other, g) for g in ("legF", "armF", "body", "legN", "armN")]


def standing_pair(gap=GAP_GRIP, grip=True, a_kw=None, b_kw=None):
    a_kw = a_kw or {}
    b_kw = b_kw or {}
    ax, bx = AXIS + gap / 2, AXIS - gap / 2
    A = stand(ax, facing=-1, **a_kw)
    B = stand(bx, facing=1, **b_kw)
    if grip:
        fa, fb = Frame(A), Frame(B)
        A = ik_arm(A, "N", (fb.N[0] + 4, fb.N[1] + 4), 1)
        A = ik_arm(A, "F", (fb.limbs["armN"][2][0] + 1, fb.limbs["armN"][2][1] + 1), 1)
        fa = Frame(A)
        B = ik_arm(B, "N", (fa.N[0] - 4, fa.N[1] + 4), 1)
        B = ik_arm(B, "F", (fa.limbs["armN"][2][0] - 1, fa.limbs["armN"][2][1] + 1), 1)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    return A, B, order


class Seq:
    def __init__(self, name, end_flip=0, swap=0, loop=False):
        self.name = name
        self.end_flip = end_flip
        self.swap = swap
        self.loop = loop
        self.keys = []      # (A, B, order, hold, tween, tick, easing, fx)

    def key(self, st, hold=4, tween=2, tick=4, easing="io", fx=0):
        A, B, order = st
        self.keys.append((A, B, order, hold, tween, tick, easing, fx))
        return self

    def frames(self):
        out = []
        n = len(self.keys)
        for i, (A, B, order, hold, tween, tick, easing, fx) in enumerate(self.keys):
            out.append((A, B, order, hold, fx))
            if i + 1 < n and tween > 0:
                A2, B2, order2 = self.keys[i + 1][:3]
                for k in range(1, tween + 1):
                    t = ease(k / (tween + 1), easing)
                    o = order if t < 0.5 else order2
                    out.append((interp(A, A2, t), interp(B, B2, t), o, tick, 0))
        return out


# ---------------------------------------------------------------- positions (boucles)
def idle_seqs():
    seqs = []
    for name, fn in G.POSITIONS:
        s = Seq("G_" + name, loop=True)
        s.key(fn(), hold=24, tween=0)
        s.key(fn(breath=1.0), hold=24, tween=0)
        seqs.append(s)
    return seqs


# ---------------------------------------------------------------- debout -> sol
def shoot_contact(single=False):
    """A en penetration sur B (canonique : A arrive de la droite)."""
    A, B, _ = standing_pair(gap=40, grip=False, b_kw=dict(h=35.5, lean=6))
    bx = B.root[0]
    A = Pose(root=(bx + 30, ANKLE - 22), tor=-90 - 58, front=-1, head=150)
    A = ik_leg(A, "N", (bx + 20, ANKLE - 1), 1)
    A = ik_leg(A, "F", (bx + 54, ANKLE), 1)
    A = foot_flat(A, "N")
    A = foot_flat(A, "F")
    fb = Frame(B)
    kb = fb.limbs["legN"][1]
    if single:
        A = ik_arm(A, "N", (kb[0] - 2, kb[1] + 2), 1)
        A = ik_arm(A, "F", (kb[0] + 1, kb[1] - 6), 1)
    else:
        A = ik_arm(A, "N", (fb.P[0] - 6, fb.P[1] + 12), 1)
        A = ik_arm(A, "F", (fb.P[0] - 3, fb.P[1] + 6), 1)
    A = look(A, 180).copy(face=1)
    fa = Frame(A)
    B = ik_arm(B, "N", (fa.N[0] + 2, fa.N[1] - 4), -1)
    B = ik_arm(B, "F", (fa.N[0] + 6, fa.N[1] - 6), -1)
    B = B.copy(face=1)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    return A, B, order


def td_double():
    s = Seq("TD_DOUBLE")
    A0, B0, o0 = shoot_contact()
    s.key((A0, B0, o0), hold=4, tween=1)
    # A souleve et pousse : B part en arriere
    bx = B0.root[0]
    B1 = Pose(root=(bx - 4, B0.root[1] - 6), tor=-90 - 38, front=1, head=150)
    B1 = ik_leg(B1, "N", (bx + 6, ANKLE - 14), 1)
    B1 = ik_leg(B1, "F", (bx + 2, ANKLE - 10), 1)
    B1 = ik_arm(B1, "N", (bx - 4, 50), -1)
    B1 = ik_arm(B1, "F", (bx - 10, 54), -1)
    B1 = B1.copy(face=3)
    A1 = Pose(root=(bx + 22, ANKLE - 26), tor=-90 - 40, front=-1, head=160)
    A1 = ik_leg(A1, "N", (bx + 14, ANKLE - 1), 1)
    A1 = ik_leg(A1, "F", (bx + 44, ANKLE), 1)
    A1 = foot_flat(A1, "F")
    f1 = Frame(B1)
    A1 = ik_arm(A1, "N", (f1.limbs["legN"][1][0] + 2, f1.limbs["legN"][1][1] + 1), 1)
    A1 = ik_arm(A1, "F", (f1.P[0], f1.P[1] + 4), 1)
    A1 = look(A1, 190).copy(face=1)
    s.key((A1, B1, o0), hold=5, tween=1, tick=3)
    # B en l'air, a 45 degres
    B2 = Pose(root=(bx - 14, 74), tor=-90 - 70, front=1, head=150)
    B2 = B2.copy(legN=(110, -40, 90), legF=(100, -30, 90), armN=(140, 30), armF=(150, 20), face=3)
    A2 = Pose(root=(bx + 10, ANKLE - 26), tor=-90 - 62, front=-1, head=160)
    A2 = ik_leg(A2, "N", (bx + 4, ANKLE - 2), 1)
    A2 = ik_leg(A2, "F", (bx + 36, ANKLE), 1)
    f2 = Frame(B2)
    A2 = ik_arm(A2, "N", (f2.limbs["legN"][1][0] + 1, f2.limbs["legN"][1][1] + 1), 1)
    A2 = ik_arm(A2, "F", (f2.P[0] + 2, f2.P[1] + 2), 1)
    A2 = look(A2, 200).copy(face=1)
    s.key((A2, B2, o0), hold=3, tween=1, tick=3)
    # impact : garde fermee
    Ag, Bg, og = G.guard()
    s.key((Ag.copy(face=1), Bg.copy(face=2), og), hold=10, tween=1, fx=1)
    s.key(G.guard(), hold=4, tween=0)
    return s


def td_double_sprawl(single=False):
    s = Seq("TD_SINGLE_SPRAWL" if single else "TD_DOUBLE_SPRAWL")
    A0, B0, o0 = shoot_contact(single)
    s.key((A0, B0, o0), hold=4, tween=1)
    bx = B0.root[0]
    # B sprawle : hanches sur les epaules de A, jambes chassees en arriere
    B1 = Pose(root=(bx + 4, FLOOR - 26), tor=-90 + 68, front=1, head=150)
    B1 = ik_leg(B1, "N", (bx - 28, ANKLE), -1)
    B1 = ik_leg(B1, "F", (bx - 34, ANKLE), -1)
    B1 = B1.copy(legN=(B1.legN[0], B1.legN[1], 60), legF=(B1.legF[0], B1.legF[1], 60), face=1)
    A1 = Pose(root=(bx + 30, FLOOR - 16), tor=-90 - 80, front=-1, head=140)
    A1 = ik_leg(A1, "N", (bx + 30, ANKLE - 1), 1)
    A1 = ik_leg(A1, "F", (bx + 56, ANKLE), 1)
    fa1 = Frame(A1)
    B1 = ik_arm(B1, "N", (fa1.N[0] + 6, fa1.N[1] + 2), 1)
    B1 = ik_arm(B1, "F", (fa1.N[0] + 10, fa1.N[1] - 2), 1)
    A1 = ik_arm(A1, "N", (bx + 2, FLOOR - 2), 1)
    A1 = ik_arm(A1, "F", (bx + 6, FLOOR - 1), 1)
    A1 = A1.copy(face=2)
    o1 = [("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"),
          ("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"), ("B", "armN")]
    s.key((A1, B1, o1), hold=12, tween=2, tick=3, fx=2)
    # les deux se relevent et se separent
    A2, B2, o2 = standing_pair(gap=GAP_FAR, grip=False)
    s.key((A2, B2, o2), hold=6, tween=2, tick=4)
    return s


def td_single():
    s = Seq("TD_SINGLE")
    A0, B0, o0 = shoot_contact(single=True)
    s.key((A0, B0, o0), hold=4, tween=1)
    bx = B0.root[0]
    # A se releve en tenant la jambe ; B sautille sur un pied
    B1 = Pose(root=(bx - 2, ANKLE - 36), tor=-90 - 18, front=1, head=165)
    B1 = ik_leg(B1, "F", (bx - 6, ANKLE), 1)
    B1 = B1.copy(legN=(85, -30, 90))
    B1 = ik_arm(B1, "N", (bx - 6, 46), -1)
    B1 = ik_arm(B1, "F", (bx - 16, 52), -1)
    B1 = B1.copy(face=3)
    f1 = Frame(B1)
    an = f1.limbs["legN"][2]
    A1 = stand(bx + 34, facing=-1, h=33, lean=20)
    A1 = ik_arm(A1, "N", (an[0] + 2, an[1] + 1), 1)
    A1 = ik_arm(A1, "F", (an[0] + 5, an[1] - 3), 1)
    s.key((A1, B1, o0), hold=8, tween=2, tick=3)
    # balayage de la jambe d'appui : B tombe
    B2 = Pose(root=(bx - 8, 82), tor=-90 - 64, front=1, head=150)
    B2 = B2.copy(legN=(100, -30, 90), legF=(60, -10, 90), armN=(130, 30), armF=(150, 20), face=3)
    A2 = stand(bx + 24, facing=-1, h=29, lean=40)
    f2 = Frame(B2)
    A2 = ik_arm(A2, "N", (f2.limbs["legN"][2][0] + 2, f2.limbs["legN"][2][1]), 1)
    A2 = ik_arm(A2, "F", (f2.limbs["legN"][1][0] + 4, f2.limbs["legN"][1][1]), 1)
    s.key((A2, B2, o0), hold=3, tween=1, tick=3)
    Ah, Bh, oh = G.half()
    s.key((Ah.copy(face=1), Bh.copy(face=2), oh), hold=10, tween=1, fx=1)
    s.key(G.half(), hold=4, tween=0)
    return s


def td_judo():
    """Projection de hanche : A rentre, charge B sur sa hanche, B passe par-dessus et tombe a droite."""
    s = Seq("TD_JUDO", end_flip=1)
    s.key(standing_pair(gap=GAP_GRIP), hold=4, tween=1)
    bx = AXIS - GAP_GRIP / 2
    # A pivote : dos contre B (regarde a droite), hanches sous les hanches de B
    A1 = stand(bx + 12, facing=1, h=31, lean=24, step_n=10, step_f=-8)
    B1 = stand(bx - 2, facing=1, h=35, lean=34, step_n=8, step_f=-10)
    f1 = Frame(A1)
    B1 = ik_arm(B1, "N", (f1.N[0] + 2, f1.N[1] + 6), -1)
    B1 = ik_arm(B1, "F", (f1.N[0] - 2, f1.N[1] + 8), -1)
    fb = Frame(B1)
    A1 = ik_arm(A1, "N", (fb.limbs["armN"][1][0] + 6, fb.limbs["armN"][1][1] + 4), 1)
    A1 = ik_arm(A1, "F", (fb.N[0] + 2, fb.N[1] + 2), -1)
    A1 = A1.copy(face=1)
    o1 = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
          ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    s.key((A1, B1, o1), hold=6, tween=1, tick=3)
    # B charge sur la hanche, a l'horizontale
    A2 = stand(bx + 12, facing=1, h=30, lean=58, step_n=11, step_f=-9)
    f2 = Frame(A2)
    B2 = Pose(root=(f2.P[0] - 2, f2.P[1] - 12), tor=wrap(-4), front=-1, head=150)
    B2 = B2.copy(legN=(150, -40, 90), legF=(140, -30, 90), armN=(60, 20), armF=(80, 30), face=3)
    fb2 = Frame(B2)
    A2 = ik_arm(A2, "N", (fb2.N[0] + 2, fb2.N[1] + 5), 1)
    A2 = ik_arm(A2, "F", (fb2.P[0] + 4, fb2.P[1] + 4), 1)
    A2 = A2.copy(face=1)
    o2 = [("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"),
          ("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"), ("B", "armN"), ("A", "armN")]
    s.key((A2, B2, o2), hold=3, tween=1, tick=3)
    # B retombe tete en bas de l'autre cote
    A3 = stand(bx + 12, facing=1, h=29, lean=50, step_n=12, step_f=-10)
    B3 = Pose(root=(bx + 40, 66), tor=wrap(60), front=-1, head=150)
    B3 = B3.copy(legN=(150, -20, 90), legF=(140, -10, 90), armN=(90, 20), armF=(110, 30), face=3)
    s.key((A3, B3, o2), hold=3, tween=1, tick=3)
    # impact, puis 100 kilos (miroir)
    As, Bs, os_ = mirror_state(G.side())
    s.key((As.copy(face=1), Bs.copy(face=2), os_), hold=10, tween=2, tick=3, fx=1)
    s.key(mirror_state(G.side()), hold=4, tween=0)
    return s


def td_judo_block():
    s = Seq("TD_JUDO_BLOCK")
    s.key(standing_pair(gap=GAP_GRIP), hold=4, tween=1)
    bx = AXIS - GAP_GRIP / 2
    A1 = stand(bx + 12, facing=1, h=31, lean=24, step_n=10, step_f=-8)
    # B baisse les hanches et pousse : A ne peut pas charger
    B1 = stand(bx - 6, facing=1, h=28, lean=2, step_n=12, step_f=-14)
    f1 = Frame(A1)
    B1 = ik_arm(B1, "N", (f1.P[0] - 4, f1.P[1] - 2), 1)
    B1 = ik_arm(B1, "F", (f1.P[0] - 8, f1.P[1] + 2), 1)
    A1 = ik_arm(A1, "N", (f1.S[0] + 12, f1.S[1] + 8), 1).copy(face=1)
    o1 = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
          ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    s.key((A1, B1.copy(face=1), o1), hold=12, tween=2, tick=3)
    s.key(standing_pair(gap=GAP_FAR, grip=False), hold=6, tween=2)
    return s


def pull_guard():
    """A tire la garde : il s'assoit et tombe sur le dos, B tombe dans sa garde (a gauche, regarde a droite)."""
    s = Seq("PULL_GUARD", end_flip=1, swap=1)
    s.key(standing_pair(gap=GAP_GRIP), hold=4, tween=1)
    bx = AXIS - GAP_GRIP / 2
    ax = AXIS + GAP_GRIP / 2
    # A s'assoit en tirant
    A1 = Pose(root=(ax - 4, FLOOR - 14), tor=-90 + 30, front=-1, head=160)
    A1 = ik_leg(A1, "N", (bx + 18, ANKLE - 12), -1)
    A1 = ik_leg(A1, "F", (bx + 22, ANKLE), -1)
    B1 = stand(bx + 4, facing=1, h=33, lean=40)
    fb = Frame(B1)
    A1 = ik_arm(A1, "N", (fb.N[0] + 4, fb.N[1] + 2), 1)
    A1 = ik_arm(A1, "F", (fb.limbs["armN"][2][0], fb.limbs["armN"][2][1]), 1)
    fa = Frame(A1)
    B1 = ik_arm(B1, "N", (fa.N[0] - 2, fa.N[1] + 6), 1)
    B1 = ik_arm(B1, "F", (fa.P[0] - 4, fa.P[1]), 1)
    o1 = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
          ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    s.key((A1.copy(face=1), B1.copy(face=1), o1), hold=6, tween=2, tick=3)
    end = mirror_state(swap_state(G.guard()))
    s.key(end, hold=6, tween=2, tick=3)
    return s


def handshake():
    """Salut d'avant combat : on se tape la main puis on cogne les poings."""
    s = Seq("SHAKE")
    A0, B0, o0 = standing_pair(gap=58, grip=False, a_kw=dict(h=36, lean=6), b_kw=dict(h=36, lean=6))
    s.key((A0, B0, o0), hold=8, tween=1)
    A1, B1, o1 = standing_pair(gap=40, grip=False, a_kw=dict(h=36, lean=8), b_kw=dict(h=36, lean=8))
    mid = (AXIS, 52)
    A1 = ik_arm(A1, "N", (mid[0] + 2, mid[1]), 1)
    B1 = ik_arm(B1, "N", (mid[0] - 2, mid[1]), 1)
    A1 = ik_arm(A1, "F", (mid[0] + 2, mid[1] + 6), 1)
    B1 = ik_arm(B1, "F", (mid[0] - 2, mid[1] + 6), 1)
    s.key((A1.copy(face=0), B1.copy(face=0), o1), hold=12, tween=2, tick=4)
    s.key((A0, B0, o0), hold=6, tween=2, tick=4)
    return s


TAKEDOWNS = [td_double, lambda: td_double_sprawl(False), td_single, lambda: td_double_sprawl(True),
             td_judo, td_judo_block, pull_guard, handshake]
