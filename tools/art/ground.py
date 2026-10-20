"""Positions au sol canoniques : B sur le dos tete a GAUCHE, A (dessus) regarde a gauche.

Chaque fonction renvoie (A, B, ordre). L'ordre liste (qui, groupe) de l'arriere vers l'avant.
Groupes : legF, armF, body, legN, armN.
"""
from poses import supine, FLOOR, ANKLE, jpos
from skel import Pose, Frame, ik_arm, ik_leg, foot_flat, look, wrap

BX = 86.0           # bassin de B
BY = FLOOR - 9.5    # bassin de B (dos au sol)

ORDER_B_UNDER = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
                 ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]


def _kneel_shins_back(p, knee_n, knee_f, foot_dx=21.0):
    """genoux au sol, tibias a plat vers l'arriere (vers la droite)."""
    p = ik_leg(p, "N", knee_n, 1) if False else p
    return p


def leg_by_knee(p, side, knee, ankle, bend=1):
    """Place une jambe par son genou puis sa cheville (cuisse et tibia de longueur fixe)."""
    fr = Frame(p)
    from skel import vec_ang, v_sub
    hip = fr.H
    th = fr.to_local_angle(_unit(v_sub(knee, hip)))
    sh = fr.to_local_angle(_unit(v_sub(ankle, knee)))
    leg = getattr(p, "leg" + side)
    return p.copy(**{"leg" + side: (th, wrap(sh - th), leg[2])})


def arm_by_elbow(p, side, elbow, hand):
    fr = Frame(p)
    from skel import v_sub
    up = fr.to_local_angle(_unit(v_sub(elbow, fr.S)))
    fo = fr.to_local_angle(_unit(v_sub(hand, elbow)))
    return p.copy(**{"arm" + side: (up, wrap(fo - up))})


def _unit(v):
    import math
    l = math.hypot(v[0], v[1]) or 1
    return (v[0] / l, v[1] / l)


def foot_dir(p, side, world_deg):
    fr = Frame(p)
    from skel import ang_vec
    leg = getattr(p, "leg" + side)
    fa = fr.to_local_angle(ang_vec(world_deg))
    return p.copy(**{"leg" + side: (leg[0], leg[1], wrap(fa - leg[0] - leg[1]))})


# ---------------------------------------------------------------- garde fermee
def guard(bx=BX, breath=0.0):
    B = supine(bx, lift=6, head_up=28)
    A = Pose(root=(bx + 15, 85 + breath), tor=-104, front=-1, head=168)
    A = leg_by_knee(A, "N", (bx + 10, 103.5), (bx + 30, 104.5))
    A = leg_by_knee(A, "F", (bx + 13, 103.5), (bx + 33, 104.5))
    A = foot_dir(A, "N", 0)
    A = foot_dir(A, "F", 0)
    A = ik_arm(A, "N", (bx - 2, 91 + breath), 1)
    A = ik_arm(A, "F", (bx - 13, 90 + breath), 1)
    A = look(A, 160)
    fa = Frame(A)
    B = ik_leg(B, "N", (fa.P[0] + 16, fa.P[1] - 6), -1)
    B = ik_leg(B, "F", (fa.P[0] + 13, fa.P[1] - 9), -1)
    B = foot_dir(B, "N", -60)
    B = foot_dir(B, "F", -120)
    wN = fa.limbs["armN"][2]
    B = ik_arm(B, "N", (wN[0] + 1, wN[1] - 1), -1)
    B = ik_arm(B, "F", (fa.N[0] - 1, fa.N[1] + 4), -1)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
             ("A", "legN"), ("A", "armN"), ("B", "legN"), ("B", "armN")]
    return A, B, order


# ---------------------------------------------------------------- demi-garde
def half(bx=BX, breath=0.0):
    B = supine(bx, lift=6, head_up=22)
    fb = Frame(B)
    A = Pose(root=(bx + 14, 82 + breath), tor=-162, front=-1, head=160)
    A = leg_by_knee(A, "N", (bx + 6, 103.5), (bx + 26, 105))
    A = foot_dir(A, "N", 0)
    A = ik_leg(A, "F", (bx + 50, ANKLE), -1)
    A = foot_dir(A, "F", 80)
    A = ik_arm(A, "N", (fb.C[0] - 8, fb.C[1] + 6), 1)
    A = ik_arm(A, "F", (fb.N[0] + 4, fb.N[1] - 2), 1)
    A = look(A, 130)
    fa = Frame(A)
    kn = fa.limbs["legN"][1]
    an = fa.limbs["legN"][2]
    # B : jambe proche par-dessus la cuisse de A, jambe loin sous le tibia
    B = leg_by_knee(B, "N", (bx + 15, 91), (an[0] - 3, an[1] - 7))
    B = foot_dir(B, "N", 60)
    B = ik_leg(B, "F", (an[0] - 2, an[1] - 1), -1)
    B = foot_dir(B, "F", 0)
    B = ik_arm(B, "N", (fa.N[0] + 7, fa.N[1] + 2), -1)
    B = ik_arm(B, "F", (fa.P[0] - 6, fa.P[1] - 7), -1)
    order = [("B", "legF"), ("B", "body"), ("A", "legF"), ("A", "armF"), ("A", "body"),
             ("A", "legN"), ("A", "armN"), ("B", "legN"), ("B", "armN"), ("B", "armF")]
    return A, B, order


# ---------------------------------------------------------------- 100 kilos
def side(bx=BX, breath=0.0):
    B = supine(bx, lift=0, head_up=14)
    B = ik_leg(B, "N", (bx + 30, ANKLE), 1)
    B = ik_leg(B, "F", (bx + 34, ANKLE), 1)
    B = foot_dir(B, "N", 0)
    B = foot_dir(B, "F", 0)
    fb = Frame(B)
    A = Pose(root=(bx + 12, 85 + breath), tor=-176, front=-1, head=140)
    A = leg_by_knee(A, "N", (bx + 20, 104), (bx + 40, 105))
    A = leg_by_knee(A, "F", (bx + 28, 104), (bx + 48, 105.5))
    A = foot_dir(A, "N", 0)
    A = foot_dir(A, "F", 0)
    A = ik_arm(A, "N", (fb.C[0] - 7, 104), 1)
    A = ik_arm(A, "F", (fb.N[0] + 4, fb.N[1] + 8), 1)
    A = look(A, 100)
    fa = Frame(A)
    B = ik_arm(B, "N", (fa.N[0] + 6, fa.N[1] + 7), -1)
    B = ik_arm(B, "F", (fa.N[0] + 8, fa.N[1] - 5), -1)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"), ("B", "armN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN")]
    return A, B, order


# ---------------------------------------------------------------- genou sur le ventre
def knee(bx=BX, breath=0.0):
    B = supine(bx, lift=0, head_up=16)
    fb = Frame(B)
    kn = (bx - 8, 86 + breath * 0.3)
    A = Pose(root=(bx + 6, 66 + breath), tor=-104, front=-1, head=170)
    A = leg_by_knee(A, "N", kn, (bx + 11, 89))
    A = foot_dir(A, "N", 20)
    A = ik_leg(A, "F", (bx + 34, ANKLE), -1)
    A = foot_dir(A, "F", 180)
    A = ik_arm(A, "N", (fb.N[0] + 3, fb.N[1] - 7), 1)
    A = ik_arm(A, "F", (bx - 4, 90), 1)
    A = look(A, 150)
    fa = Frame(A)
    B = ik_arm(B, "N", (kn[0] + 2, kn[1] - 4), -1)
    B = ik_arm(B, "F", (kn[0] - 2, kn[1] - 6), -1)
    B = ik_leg(B, "N", (bx + 24, ANKLE), 1)
    B = ik_leg(B, "F", (bx + 32, ANKLE), 1)
    B = foot_dir(B, "N", 0)
    B = foot_dir(B, "F", 0)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "legN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN"), ("B", "armN")]
    return A, B, order


# ---------------------------------------------------------------- montee
def mount(bx=BX, breath=0.0):
    B = supine(bx, lift=0, head_up=18)
    B = ik_leg(B, "N", (bx + 26, ANKLE), 1)
    B = ik_leg(B, "F", (bx + 32, ANKLE), 1)
    B = foot_dir(B, "N", 0)
    B = foot_dir(B, "F", 0)
    fb = Frame(B)
    A = Pose(root=(bx - 12, 82 + breath), tor=-98, front=-1, head=170)
    A = leg_by_knee(A, "N", (bx - 26, 103), (bx - 6, 104.5))
    A = leg_by_knee(A, "F", (bx - 24, 102), (bx - 4, 103.5))
    A = foot_dir(A, "N", 0)
    A = foot_dir(A, "F", 0)
    A = ik_arm(A, "N", (fb.N[0] + 2, fb.N[1] - 7 + breath), 1)
    A = ik_arm(A, "F", (fb.N[0] - 3, fb.N[1] - 6 + breath), 1)
    A = look(A, 150)
    fa = Frame(A)
    B = ik_arm(B, "N", (fa.P[0] + 5, fa.P[1] + 4), -1)
    B = ik_arm(B, "F", (fa.P[0] + 7, fa.P[1] - 1), -1)
    order = [("B", "legF"), ("B", "armF"), ("B", "body"), ("B", "armN"), ("B", "legN"),
             ("A", "legF"), ("A", "armF"), ("A", "body"), ("A", "legN"), ("A", "armN")]
    return A, B, order


# ---------------------------------------------------------------- dos
def back(bx=BX, breath=0.0):
    # B assis, adosse contre A ; A assis derriere, crochets dans les cuisses
    ax = bx + 24
    A = Pose(root=(ax, FLOOR - 9 + breath * 0.2), tor=-52, front=-1, head=150)
    B = Pose(root=(ax - 20, FLOOR - 10), tor=-78, front=-1, head=168)
    B = ik_leg(B, "N", (ax - 56, ANKLE), 1)
    B = ik_leg(B, "F", (ax - 52, ANKLE), 1)
    B = foot_dir(B, "N", 180)
    B = foot_dir(B, "F", 180)
    fb = Frame(B)
    kbn = fb.limbs["legN"][1]
    kbf = fb.limbs["legF"][1]
    A = ik_leg(A, "N", (kbn[0] + 4, kbn[1] + 5), 1)
    A = ik_leg(A, "F", (kbf[0] + 5, kbf[1] + 6), 1)
    A = foot_dir(A, "N", 150)
    A = foot_dir(A, "F", 150)
    # ceinture de securite : bras proche par-dessus l'epaule, bras loin sous l'aisselle
    A = ik_arm(A, "N", (fb.N[0] - 9, fb.N[1] + 13 + breath), -1)
    A = ik_arm(A, "F", (fb.P[0] - 6, fb.P[1] - 16), 1)
    A = look(A, 200)
    fa = Frame(A)
    wn = fa.limbs["armN"][2]
    B = ik_arm(B, "N", (wn[0] + 3, wn[1] - 1), 1)
    B = ik_arm(B, "F", (wn[0] - 2, wn[1] + 2), 1)
    B = look(B, 190)
    B = B.copy(face=1)
    order = [("A", "legF"), ("A", "armF"), ("A", "body"), ("B", "legF"), ("B", "armF"), ("B", "body"),
             ("B", "legN"), ("B", "armN"), ("A", "legN"), ("A", "armN")]
    return A, B, order


POSITIONS = [("GUARD", guard), ("HALF", half), ("SIDE", side), ("KNEE", knee), ("MOUNT", mount), ("BACK", back)]
