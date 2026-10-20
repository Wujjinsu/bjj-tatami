"""Transitions au sol et soumissions (convention canonique, voir pairs.py)."""
import ground as G
from pairs import Seq, mirror_state, swap_state, standing_pair, GAP_FAR, AXIS
from poses import stand, supine, FLOOR, ANKLE
from skel import Pose, Frame, ik_arm, ik_leg, foot_flat, look, wrap, interp

BX = G.BX
O_B_UNDER = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]


def face(st, fa=None, fb=None):
    A, B, o = st
    if fa is not None:
        A = A.copy(face=fa)
    if fb is not None:
        B = B.copy(face=fb)
    return A, B, o


def simple(name, start, end, mids=(), end_flip=0, swap=0, tween=2):
    s = Seq(name, end_flip=end_flip, swap=swap)
    s.key(start(), hold=3, tween=tween, tick=4)
    for m in mids:
        s.key(m, hold=5, tween=tween, tick=4)
    s.key(face(end(), 1, None), hold=6, tween=0)
    return s


def standup_end():
    return standing_pair(gap=GAP_FAR, grip=False)


# ---------------------------------------------------------------- poses intermediaires
def combat_base():
    """A se releve dans la garde (un pied au sol), B garde encore les jambes."""
    B = supine(BX, lift=4, head_up=24)
    A = Pose(root=(BX + 16, 76), tor=-100, front=-1, head=165)
    A = ik_leg(A, "N", (BX + 6, ANKLE), -1)
    A = foot_flat(A, "N")
    A = ik_leg(A, "F", (BX + 26, 103), 1)
    A = A.copy(legF=(A.legF[0], A.legF[1], 60))
    A = ik_arm(A, "N", (BX - 2, 92), 1)
    A = ik_arm(A, "F", (BX + 6, 88), 1)
    A = look(A, 170)
    fa = Frame(A)
    B = ik_leg(B, "N", (fa.P[0] + 14, fa.P[1] - 4), -1)
    B = ik_leg(B, "F", (fa.P[0] + 10, fa.P[1] - 6), -1)
    return A, B, O_B_UNDER


def toreando_mid():
    """A debout, controle les genoux de B et les pousse sur le cote."""
    B = supine(BX, lift=0, head_up=18)
    B = B.copy(legN=(150, -110, 90), legF=(145, -105, 90))
    fb = Frame(B)
    kb = fb.limbs["legN"][1]
    A = stand(BX + 26, facing=-1, h=30, lean=44, step_n=12, step_f=-12)
    A = ik_arm(A, "N", (kb[0] + 2, kb[1] + 1), 1)
    A = ik_arm(A, "F", (kb[0] + 6, kb[1] - 2), 1)
    return A, B, O_B_UNDER


def knee_slide_mid():
    """genou qui glisse en travers du ventre."""
    A, B, o = G.half()
    A2, B2, o2 = G.mount()
    A = interp(A, A2, 0.5)
    A = A.copy(root=(A.root[0] + 2, A.root[1] - 8))
    B = interp(B, B2, 0.5)
    return A, B, O_B_UNDER


def turtle_mid():
    """B se retourne a quatre pattes (pour prise de dos)."""
    B = Pose(root=(BX, FLOOR - 26), tor=180 + 10, front=-1, head=150)
    B = ik_leg(B, "N", (BX - 6, FLOOR - 4), 1)
    B = B.copy(legN=(B.legN[0], B.legN[1], 0))
    B = ik_leg(B, "F", (BX - 2, FLOOR - 4), 1)
    fb = Frame(B)
    B = ik_arm(B, "N", (fb.S[0] - 2, FLOOR - 2), 1)
    B = ik_arm(B, "F", (fb.S[0] + 2, FLOOR - 2), 1)
    A = stand(BX + 24, facing=-1, h=28, lean=48)
    A = ik_arm(A, "N", (fb.N[0] + 4, fb.N[1] - 4), 1)
    A = ik_arm(A, "F", (fb.P[0] + 2, fb.P[1] - 6), 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"), ("B", "armN"),
         ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN")]
    return A, B, o


def roll_mid():
    """les deux roulent (balayage / upa) : B a moitie releve, A bascule."""
    B = Pose(root=(BX - 2, FLOOR - 14), tor=-150, front=1, head=160)
    B = B.copy(legN=(110, -60, 90), legF=(100, -40, 90))
    A = Pose(root=(BX + 6, 70), tor=wrap(-20), front=-1, head=150)
    A = A.copy(legN=(140, -40, 90), legF=(150, -60, 90), armN=(100, 30), armF=(120, 20), face=3)
    o = [("A", "legF"), ("A", "armF"), ("A", "body"), ("B", "legF"), ("B", "armF"), ("B", "body"),
         ("B", "legN"), ("B", "armN"), ("A", "legN"), ("A", "armN")]
    return A, B, o


def techstand_mid():
    """B se releve techniquement (une main au sol)."""
    B = Pose(root=(BX - 6, FLOOR - 14), tor=-120, front=1, head=170)
    B = ik_leg(B, "N", (BX + 12, ANKLE), 1)
    B = foot_flat(B, "N")
    B = B.copy(legF=(70, -120, 90))
    fb = Frame(B)
    B = ik_arm(B, "F", (fb.P[0] - 14, FLOOR - 2), 1)
    B = ik_arm(B, "N", (fb.S[0] + 16, fb.S[1] - 4), 1)
    A = stand(BX + 40, facing=-1, h=32, lean=24)
    return A, B, [("B", g) for g in ("legF", "armF", "body", "legN", "armN")] + \
        [("A", g) for g in ("legF", "armF", "body", "legN", "armN")]


# ---------------------------------------------------------------- transitions
def transitions():
    T = []
    # dessus
    T.append(simple("T_GUARD_HALF", G.guard, G.half, [combat_base()]))
    T.append(simple("T_GUARD_SIDE", G.guard, G.side, [combat_base(), toreando_mid()]))
    T.append(simple("T_GUARD_STAND", G.guard, standup_end, [combat_base()]))
    T.append(simple("T_HALF_SIDE", G.half, G.side))
    T.append(simple("T_HALF_MOUNT", G.half, G.mount, [knee_slide_mid()]))
    T.append(simple("T_HALF_STAND", G.half, standup_end, [techstand_mid()]))
    T.append(simple("T_SIDE_MOUNT", G.side, G.mount, [knee_slide_mid()]))
    T.append(simple("T_SIDE_KNEE", G.side, G.knee))
    T.append(simple("T_SIDE_BACK", G.side, G.back, [turtle_mid()]))
    T.append(simple("T_KNEE_MOUNT", G.knee, G.mount))
    T.append(simple("T_KNEE_SIDE", G.knee, G.side))
    T.append(simple("T_MOUNT_BACK", G.mount, G.back, [turtle_mid()]))
    # dessous (B agit)
    T.append(simple("B_GUARD_SWEEP", G.guard, lambda: mirror_state(swap_state(G.half())), [roll_mid()],
                    end_flip=1, swap=1))
    T.append(simple("B_GUARD_BACK", G.guard, lambda: swap_state(G.back()), [turtle_mid()], swap=1))
    T.append(simple("B_GUARD_STAND", G.guard, standup_end, [techstand_mid()]))
    T.append(simple("B_HALF_GUARD", G.half, G.guard))
    T.append(simple("B_HALF_SWEEP", G.half, lambda: mirror_state(swap_state(G.half())), [roll_mid()],
                    end_flip=1, swap=1))
    T.append(simple("B_HALF_STAND", G.half, standup_end, [techstand_mid()]))
    T.append(simple("B_SIDE_HALF", G.side, G.half))
    T.append(simple("B_SIDE_GUARD", G.side, G.guard))
    T.append(simple("B_KNEE_HALF", G.knee, G.half))
    T.append(simple("B_MOUNT_UPA", G.mount, lambda: swap_state(G.guard()), [roll_mid()], swap=1))
    T.append(simple("B_MOUNT_HALF", G.mount, G.half))
    T.append(simple("B_BACK_ROLL", G.back, lambda: mirror_state(swap_state(G.guard())), [roll_mid()],
                    end_flip=1, swap=1))
    T.append(simple("B_BACK_HALF", G.back, G.half))
    return T


# ---------------------------------------------------------------- soumissions : verrous
def lk_ankle(sq=0.0):
    """A assis en arriere, le pied de B coince sous l'aisselle."""
    B = supine(BX, lift=8, head_up=30)
    A = Pose(root=(BX + 34, FLOOR - 10), tor=-60 - 6 * sq, front=-1, head=150)
    A = ik_leg(A, "N", (BX + 6, ANKLE - 4), 1)
    A = ik_leg(A, "F", (BX + 10, ANKLE), 1)
    fa = Frame(A)
    pit = (fa.S[0] - 2, fa.S[1] + 4)
    B = ik_leg(B, "N", pit, -1)
    B = B.copy(legF=(90, -60, 90))
    A = ik_arm(A, "N", (pit[0] + 2, pit[1] + 5), 1)
    A = ik_arm(A, "F", (pit[0] + 4, pit[1] + 3), 1)
    A = look(A, 200).copy(face=1)
    B = B.copy(face=2 if sq else 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
         ("B", "legN"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    return A, B, o


def lk_kimura(sq=0.0, from_half=False):
    """Kimura : A tient le poignet de B et tord le bras derriere le dos."""
    A, B, o = G.half() if from_half else G.side()
    fb = Frame(B)
    A = A.copy(root=(A.root[0] + 2, A.root[1] - 6), tor=A.tor + 20)
    sh = fb.S
    el = (sh[0] + 4, sh[1] - 14 - 2 * sq)
    hand = (sh[0] - 6 - 3 * sq, sh[1] - 18)
    B = ik_arm(B, "N", hand, 1)
    A = ik_arm(A, "N", (hand[0] + 1, hand[1]), 1)
    A = ik_arm(A, "F", (el[0] + 2, el[1] + 2), 1)
    A = look(A, 150).copy(face=1)
    B = B.copy(face=2 if sq else 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
         ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("B", "armN"), ("A", "armN")]
    return A, B, o


def lk_americana(sq=0.0):
    A, B, o = G.side()
    fb = Frame(B)
    hand = (fb.C[0] + 2, FLOOR - 4)
    B = ik_arm(B, "N", hand, -1)
    A = A.copy(root=(A.root[0], A.root[1] - 2 + sq))
    A = ik_arm(A, "N", (hand[0] + 1, hand[1] - 2), 1)
    A = ik_arm(A, "F", (fb.S[0] + 2, fb.S[1] - 6 - 2 * sq), 1)
    A = A.copy(face=1)
    B = B.copy(face=2 if sq else 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
         ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("B", "armN"), ("A", "armN")]
    return A, B, o


def lk_darce(sq=0.0):
    A, B, o = G.half()
    A = A.copy(root=(A.root[0] - 4, A.root[1] + 2), tor=-170)
    fb = Frame(B)
    B = ik_arm(B, "N", (fb.C[0] + 2, fb.C[1] - 12), -1)
    fa = Frame(A)
    A = ik_arm(A, "N", (fb.C[0] - 6, fb.C[1] + 4), 1)
    A = ik_arm(A, "F", (fb.N[0] - 2, fb.N[1] - 8 - sq), 1)
    A = A.copy(face=1)
    B = B.copy(face=2 if sq else 1)
    return A, B, o


def lk_armbar_top(sq=0.0):
    """A assis sur la poitrine, penche en arriere, bras de B tendu entre ses jambes."""
    B = supine(BX, lift=0, head_up=10)
    B = ik_leg(B, "N", (BX + 26, ANKLE), 1)
    B = ik_leg(B, "F", (BX + 32, ANKLE), 1)
    B = foot_flat(B, "N")
    fb = Frame(B)
    A = Pose(root=(fb.S[0] + 6, FLOOR - 12), tor=-40 - 8 * sq, front=-1, head=150)
    A = ik_leg(A, "N", (fb.C[0] - 6, FLOOR - 6), 1)
    A = ik_leg(A, "F", (fb.S[0] - 16, FLOOR - 10), 1)
    fa = Frame(A)
    hand = (fa.N[0] + 6, fa.N[1] - 4)
    B = ik_arm(B, "N", hand, 1)
    A = ik_arm(A, "N", (hand[0] - 1, hand[1] + 2), 1)
    A = ik_arm(A, "F", (hand[0] + 2, hand[1] + 4), 1)
    A = look(A, 160).copy(face=1)
    B = B.copy(face=2 if sq else 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
         ("A", "legF"), ("A", "armF"), ("A", "body"), ("B", "armN"), ("A", "legN"), ("A", "armN")]
    return A, B, o


def lk_cross(sq=0.0):
    A, B, o = G.mount()
    A = A.copy(root=(A.root[0] + 2, A.root[1] + 4 + sq), tor=-130)
    fb = Frame(B)
    A = ik_arm(A, "N", (fb.N[0] + 1, fb.N[1] - 2), 1)
    A = ik_arm(A, "F", (fb.N[0] - 3, fb.N[1] - 3), 1)
    fa = Frame(A)
    B = ik_arm(B, "N", (fa.limbs["armN"][1][0], fa.limbs["armN"][1][1]), -1)
    B = ik_arm(B, "F", (fa.limbs["armF"][1][0], fa.limbs["armF"][1][1]), -1)
    A = A.copy(face=1)
    B = B.copy(face=2 if sq else 1)
    return A, B, o


def lk_rnc(sq=0.0):
    A, B, o = G.back()
    fb = Frame(B)
    A = ik_arm(A, "N", (fb.N[0] - 3, fb.N[1] - 1 - sq), -1)
    A = ik_arm(A, "F", (fb.C[0] + 5, fb.C[1] - 2), 1)
    fa = Frame(A)
    w = fa.limbs["armN"][2]
    B = ik_arm(B, "N", (w[0] + 4, w[1] + 1), 1)
    B = ik_arm(B, "F", (w[0] - 1, w[1] + 3), 1)
    A = A.copy(face=1)
    B = B.copy(face=2 if sq else 3)
    return A, B, o


def lk_bowarrow(sq=0.0):
    A, B, o = G.back()
    B = B.copy(tor=B.tor + 24 + 4 * sq, root=(B.root[0] + 6, B.root[1]))
    B = ik_leg(B, "N", (B.root[0] - 30, ANKLE - 10), 1)
    fb = Frame(B)
    A = A.copy(tor=A.tor + 24)
    A = ik_arm(A, "N", (fb.N[0] - 3, fb.N[1] + 2), -1)
    A = ik_arm(A, "F", (fb.limbs["legN"][1][0] + 2, fb.limbs["legN"][1][1] - 2), 1)
    A = A.copy(face=1)
    B = B.copy(face=2 if sq else 3)
    return A, B, o


def lk_triangle(sq=0.0):
    """B (dessous) : jambes autour du cou et d'un bras de A, tete de A tiree vers le bas."""
    B = supine(BX, lift=10, head_up=30)
    A = Pose(root=(BX + 18, 82), tor=-150 + 4 * sq, front=-1, head=140)
    A = ik_leg(A, "N", (BX + 14, 104), 1)
    A = ik_leg(A, "F", (BX + 18, 104), 1)
    fa = Frame(A)
    B = ik_leg(B, "N", (fa.C[0] + 10, fa.C[1] - 6), -1)
    B = ik_leg(B, "F", (fa.N[0] + 8, fa.N[1] - 8), -1)
    B = ik_arm(B, "N", (fa.C[0] - 2, fa.C[1] - 6), -1)
    B = ik_arm(B, "F", (fa.C[0] + 2, fa.C[1] - 7), -1)
    A = ik_arm(A, "N", (BX - 8, 92), 1)
    A = ik_arm(A, "F", (BX - 2, 96), 1)
    A = A.copy(face=2 if sq else 1)
    B = B.copy(face=1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
         ("A", "legN"), ("A", "armN"), ("B", "legN"), ("B", "armN")]
    return A, B, o


def lk_armbar_guard(sq=0.0):
    B = supine(BX, lift=-4 - 3 * sq, head_up=30)
    A = Pose(root=(BX + 18, 84), tor=-120, front=-1, head=160)
    A = ik_leg(A, "N", (BX + 12, 104), 1)
    A = ik_leg(A, "F", (BX + 16, 104), 1)
    fb = Frame(B)
    hand = (fb.N[0] + 4, fb.N[1] - 8)
    A = ik_arm(A, "N", hand, 1)
    fa = Frame(A)
    B = ik_leg(B, "N", (fa.C[0] - 8, fa.C[1] - 2), -1)
    B = ik_leg(B, "F", (fa.N[0] + 6, fa.N[1] + 2), -1)
    B = ik_arm(B, "N", (hand[0] + 2, hand[1] + 1), -1)
    B = ik_arm(B, "F", (hand[0] + 4, hand[1] + 3), -1)
    A = ik_arm(A, "F", (BX + 2, 98), 1)
    A = A.copy(face=2 if sq else 1)
    o = [("B", "legF"), ("B", "armF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
         ("A", "legN"), ("A", "armN"), ("B", "legN"), ("B", "armN")]
    return A, B, o


def lk_guillotine(sq=0.0):
    B = supine(BX, lift=18 + 4 * sq, head_up=30)
    fb = Frame(B)
    A = Pose(root=(BX + 18, 84), tor=-160, front=-1, head=140)
    A = ik_leg(A, "N", (BX + 14, 104), 1)
    A = ik_leg(A, "F", (BX + 18, 104), 1)
    A = place_head(A, (fb.S[0] + 8, fb.S[1] - 6))
    fa = Frame(A)
    B = ik_arm(B, "N", (fa.C[0] + 1, fa.C[1] + 6), -1)
    B = ik_arm(B, "F", (fa.C[0] - 2, fa.C[1] + 7), -1)
    B = ik_leg(B, "N", (fa.P[0] + 12, fa.P[1] - 6), -1)
    B = ik_leg(B, "F", (fa.P[0] + 10, fa.P[1] - 8), -1)
    A = ik_arm(A, "N", (BX - 4, 98), 1)
    A = A.copy(face=2 if sq else 1)
    return A, B, O_GUARD


def place_head(A, target):
    fa = Frame(A)
    d = (target[0] - fa.C[0], target[1] - fa.C[1])
    return A.copy(root=(A.root[0] + d[0], A.root[1] + d[1]))


O_GUARD = [("B", "legF"), ("B", "armF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
           ("A", "legN"), ("A", "armN"), ("B", "legN"), ("B", "armN")]


def lk_kimura_bottom(sq=0.0):
    """B (dessous en demi-garde) se releve sur un coude et tord le bras de A."""
    A, B, o = G.half()
    B = B.copy(lift=0) if False else B
    B = B.copy(tor=wrap(B.tor + 30))
    fa = Frame(A)
    sh = fa.S
    hand = (sh[0] + 10, sh[1] - 8 - 2 * sq)
    A = ik_arm(A, "N", hand, -1)
    fb = Frame(B)
    B = ik_arm(B, "N", (hand[0] - 1, hand[1]), -1)
    B = ik_arm(B, "F", (fa.limbs["armN"][1][0], fa.limbs["armN"][1][1] - 2), -1)
    A = A.copy(face=2 if sq else 1)
    B = B.copy(face=1)
    return A, B, [("B", "legF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
                  ("A", "legN"), ("B", "legN"), ("A", "armN"), ("B", "armN"), ("B", "armF")]


# (nom, position de depart, fonction de verrou, attaquant est le dessous ?)
SUBS = [
    ("S_ANKLE", G.guard, lk_ankle, False),
    ("S_KIMURA_H", G.half, lambda sq=0.0: lk_kimura(sq, True), False),
    ("S_DARCE", G.half, lk_darce, False),
    ("S_KIMURA_S", G.side, lk_kimura, False),
    ("S_AMERICANA", G.side, lk_americana, False),
    ("S_ARMBAR_K", G.knee, lk_armbar_top, False),
    ("S_ARMBAR_M", G.mount, lk_armbar_top, False),
    ("S_CROSS", G.mount, lk_cross, False),
    ("S_RNC", G.back, lk_rnc, False),
    ("S_BOWARROW", G.back, lk_bowarrow, False),
    ("S_TRIANGLE", G.guard, lk_triangle, True),
    ("S_ARMBAR_G", G.guard, lk_armbar_guard, True),
    ("S_GUILLOTINE", G.guard, lk_guillotine, True),
    ("S_KIMURA_HB", G.half, lk_kimura_bottom, True),
]


def tap_variant(st, bottom_attacks):
    """le defenseur tape : sa main libre frappe le sol / l'adversaire."""
    A, B, o = st
    if bottom_attacks:
        # A est le defenseur : main loin qui tape le sol
        fa = Frame(A)
        A = ik_arm(A, "F", (fa.S[0] - 4, FLOOR - 2), 1)
        return A.copy(face=2), B, o
    fb = Frame(B)
    B = ik_arm(B, "F", (fb.S[0] + 8, FLOOR - 2), -1)
    return A, B.copy(face=2), o


def tap_variant_up(st, bottom_attacks):
    A, B, o = st
    if bottom_attacks:
        fa = Frame(A)
        A = ik_arm(A, "F", (fa.S[0] - 6, FLOOR - 12), 1)
        return A.copy(face=2), B, o
    fb = Frame(B)
    B = ik_arm(B, "F", (fb.S[0] + 10, FLOOR - 16), -1)
    return A, B.copy(face=2), o


def submissions():
    out = []
    for name, start, lock, bottom in SUBS:
        st = Seq(name + "_SET")
        st.key(start(), hold=2, tween=2, tick=4)
        st.key(lock(0.0), hold=4, tween=0)
        out.append(st)
        lk = Seq(name + "_LOCK", loop=True)
        lk.key(lock(0.0), hold=6, tween=0)
        lk.key(lock(1.0), hold=6, tween=0)
        out.append(lk)
        tp = Seq(name + "_TAP", loop=True)
        tp.key(tap_variant_up(lock(1.0), bottom), hold=5, tween=0)
        tp.key(tap_variant(lock(1.0), bottom), hold=5, tween=0)
        out.append(tp)
    return out
