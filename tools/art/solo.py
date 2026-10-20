"""Animations individuelles (debout), regard a droite. Chaque anim = liste de (pose, duree en images)."""
from poses import stand, head_down, shift, jpos, arm_to, leg_to, FLOOR, ANKLE
from skel import Pose, Frame, ik_arm, ik_leg, foot_flat, look, wrap, interp

X = 52.0


def base(**kw):
    d = dict(h=35.0, lean=12.0, step_n=11.0, step_f=-13.0)
    d.update(kw)
    return stand(X, **d)


def idle():
    out = []
    for i, (dh, dy) in enumerate([(0.0, 0), (-0.6, 1), (-1.0, 1), (-0.6, 0)]):
        p = base(h=35.0 + dh)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.S[0] + 22, fr.S[1] + 5 + dy), 1)
        p = ik_arm(p, "F", (fr.S[0] + 17, fr.S[1] + 11 + dy), 1)
        out.append((p, 10))
    return out


def tired():
    out = []
    for dh in (0.0, -1.2):
        p = stand(X, h=33.0 + dh, lean=42, step_n=9, step_f=-11, look_deg=10)
        k = jpos(p, "legN1")
        kf = jpos(p, "legF1")
        p = ik_arm(p, "N", (k[0] + 3, k[1] - 3), 1)
        p = ik_arm(p, "F", (kf[0] + 4, kf[1] - 4), 1)
        p = p.copy(face=1)
        out.append((p, 14))
    return out


def walk(forward=True):
    """Pas chasses : le pied avant avance puis le pied arriere suit."""
    keys = [(11, -13, 0.0), (15, -13, -0.8), (16, -10, -0.5), (12, -14, 0.0), (11, -15, -0.6), (10, -12, -0.3)]
    out = []
    for sn, sf, dh in keys:
        lift_n = 2.5 if sn > 13 else 0
        p = base(step_n=sn, step_f=sf, h=35 + dh)
        if lift_n:
            p = ik_leg(p, "N", (X + sn, ANKLE - lift_n), 1)
        out.append((p, 6))
    if not forward:
        out = out[::-1]
    return out


def grip_reach():
    """Les deux mains vont chercher le col et la manche (distance de prise ~46 px)."""
    out = []
    for k, (rx, ry) in enumerate([(30, 2), (36, 0)]):
        p = base(lean=16 + 4 * k)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.S[0] + rx, fr.S[1] + ry), 1)
        p = ik_arm(p, "F", (fr.S[0] + rx - 4, fr.S[1] + ry + 7), 1)
        p = p.copy(fist=(1, 1))
        out.append((p, 5))
    return out


def grip_hold():
    """Combat de prises : on tire et on pousse (2 images en boucle)."""
    out = []
    for lean, dx, face in ((14, 34, 1), (20, 37, 1)):
        p = base(lean=lean)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.S[0] + dx, fr.S[1] + 1), 1)
        p = ik_arm(p, "F", (fr.S[0] + dx - 5, fr.S[1] + 8), 1)
        out.append((p.copy(face=face), 8))
    return out


def shoot():
    """Changement de niveau puis pas de penetration (le genou avant descend)."""
    out = []
    # 1 : on baisse le centre de gravite
    p = stand(X, h=29, lean=34, step_n=12, step_f=-14, look_deg=0)
    out.append((p, 4))
    # 2 : pas de penetration : genou avant au sol
    p = Pose(root=(X + 6, ANKLE - 24), tor=-90 + 52, front=1, head=150)
    p = ik_leg(p, "N", (X + 18, ANKLE - 1), 1)
    p = ik_leg(p, "F", (X - 22, ANKLE), 1)
    p = foot_flat(p, "N")
    p = foot_flat(p, "F")
    fr = Frame(p)
    p = ik_arm(p, "N", (fr.S[0] + 24, fr.S[1] + 14), 1)
    p = ik_arm(p, "F", (fr.S[0] + 20, fr.S[1] + 18), 1)
    p = look(p, 5)
    out.append((p.copy(face=1), 4))
    # 3 : extension maximale
    p = Pose(root=(X + 10, ANKLE - 20), tor=-90 + 62, front=1, head=150)
    p = ik_leg(p, "N", (X + 20, ANKLE - 9), 1)
    p = p.copy(legN=(p.legN[0], p.legN[1], 150))
    p = ik_leg(p, "F", (X - 26, ANKLE), 1)
    p = foot_flat(p, "F")
    fr = Frame(p)
    p = ik_arm(p, "N", (fr.S[0] + 28, fr.S[1] + 16), 1)
    p = ik_arm(p, "F", (fr.S[0] + 25, fr.S[1] + 21), 1)
    p = look(p, 8)
    out.append((p.copy(face=1), 6))
    return out


def stumble():
    """Projection dans le vide : on tombe en avant puis on se releve."""
    out = shoot()[1:]
    p = Pose(root=(X + 14, FLOOR - 16), tor=-90 + 75, front=1, head=150)
    p = ik_leg(p, "N", (X + 18, ANKLE - 2), 1)
    p = ik_leg(p, "F", (X - 24, ANKLE), 1)
    fr = Frame(p)
    p = ik_arm(p, "N", (fr.S[0] + 10, FLOOR - 2), 1)
    p = ik_arm(p, "F", (fr.S[0] + 6, FLOOR - 2), 1)
    out.append((p.copy(face=2), 10))
    out.append((shoot()[0][0], 8))
    return out


def sprawl_solo():
    """Sprawl dans le vide : hanches en arriere et au sol, jambes chassees."""
    out = []
    p = stand(X, h=30, lean=40, step_n=10, step_f=-16)
    out.append((p, 4))
    p = Pose(root=(X - 10, FLOOR - 22), tor=-90 + 70, front=1, head=140)
    p = ik_leg(p, "N", (X - 34, ANKLE), -1)
    p = ik_leg(p, "F", (X - 40, ANKLE), -1)
    p = p.copy(legN=(p.legN[0], p.legN[1], 60), legF=(p.legF[0], p.legF[1], 60))
    fr = Frame(p)
    p = ik_arm(p, "N", (fr.S[0] + 10, FLOOR - 4), 1)
    p = ik_arm(p, "F", (fr.S[0] + 6, FLOOR - 2), 1)
    p = look(p, 0)
    out.append((p.copy(face=1), 14))
    out.append((stand(X, h=30, lean=40, step_n=10, step_f=-16), 6))
    return out


def hip_block():
    """Blocage de hanche : hanches en avant, bassin bas, mains qui repoussent."""
    out = []
    for k in range(2):
        p = stand(X, h=29 - k, lean=4 - 2 * k, step_n=13, step_f=-15, tor_extra=0)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.S[0] + 20, fr.S[1] + 22), 1)
        p = ik_arm(p, "F", (fr.S[0] + 16, fr.S[1] + 26), 1)
        out.append((p.copy(face=1), 8))
    return out


def victory():
    out = []
    for k, (h, ay) in enumerate([(35, 0), (36.5, -4), (36, -6)]):
        p = stand(X, h=h, lean=-4, step_n=8, step_f=-9, look_deg=-20)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.S[0] + 8, fr.S[1] - 24 + ay * 0), 1)
        p = ik_arm(p, "F", (fr.S[0] - 6, fr.S[1] - 25), 1)
        p = p.copy(face=3)
        out.append((p, 8))
    return out


def defeat():
    out = []
    for k in range(2):
        p = Pose(root=(X, FLOOR - 22 - k * 0.6), tor=-90 + 20, front=1, head=135)
        p = ik_leg(p, "N", (X + 6, FLOOR - 3), 1)
        p = p.copy(legN=(p.legN[0], p.legN[1], 0))
        p = ik_leg(p, "F", (X - 4, FLOOR - 4), 1)
        p = p.copy(legF=(70, -150, 10))
        p = ik_leg(p, "F", (X + 2, FLOOR - 4), 1)
        k1 = jpos(p, "legN1")
        p = ik_arm(p, "N", (k1[0] - 1, k1[1] - 4), 1)
        p = ik_arm(p, "F", (k1[0] - 5, k1[1] - 3), 1)
        out.append((p.copy(face=2), 16))
    return out


def bow():
    out = []
    for lean, hd in ((2, 0), (30, 10), (38, 20), (30, 10)):
        p = stand(X, h=36.5, lean=lean, step_n=4, step_f=-4)
        fr = Frame(p)
        p = ik_arm(p, "N", (fr.P[0] + 6, fr.P[1] + 8), 1)
        p = ik_arm(p, "F", (fr.P[0] + 3, fr.P[1] + 9), 1)
        p = head_down(p, hd)
        out.append((p, 8))
    return out


ANIMS = [
    ("IDLE", idle),
    ("WALK", lambda: walk(True)),
    ("GRIP_REACH", grip_reach),
    ("GRIP_HOLD", grip_hold),
    ("SHOOT", shoot),
    ("STUMBLE", stumble),
    ("SPRAWL", sprawl_solo),
    ("HIPBLOCK", hip_block),
    ("TIRED", tired),
    ("VICTORY", victory),
    ("DEFEAT", defeat),
    ("BOW", bow),
]
