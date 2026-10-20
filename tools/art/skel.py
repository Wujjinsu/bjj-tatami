"""Squelette 2D des combattants : cinematique directe, IK a deux os, interpolation.

Conventions (pixels, y vers le bas) :
  - root  : bassin (x, y)
  - tor   : angle absolu (degres, ecran) de la direction bassin -> cou. Debout = -90.
  - front : +1 si la poitrine est du cote "tourne de +90" de l'axe du torse (debout, regard a droite).
  - Les membres sont exprimes en angles dans le repere du torse :
      0 = vers le bas du torse, 90 = vers l'avant (poitrine), 180 = vers le haut, -90 = vers l'arriere.
    upper / thigh : angle absolu dans ce repere ; forearm / shin / foot : angle relatif au segment parent.
  - head : angle du cou dans le repere du torse (180 = droit).
"""
import math
from dataclasses import dataclass, field, replace

# ---------------------------------------------------------------- proportions
TORSO = 29.0        # bassin -> base du cou
NECK = 9.5          # base du cou -> centre de la tete
HEAD_R = 7.5
UPPER = 15.5
FORE = 13.5
HAND = 2.0          # distance poignet -> centre du poing
THIGH = 20.5
SHIN = 20.0
FOOT = 8.0
SHOULDER_DROP = 3.5  # epaule sous la base du cou


def v_add(a, b):
    return (a[0] + b[0], a[1] + b[1])


def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def v_mul(a, k):
    return (a[0] * k, a[1] * k)


def v_len(a):
    return math.hypot(a[0], a[1])


def v_norm(a):
    l = v_len(a) or 1.0
    return (a[0] / l, a[1] / l)


def ang_vec(deg):
    r = math.radians(deg)
    return (math.cos(r), math.sin(r))


def vec_ang(v):
    return math.degrees(math.atan2(v[1], v[0]))


def wrap(a):
    while a > 180:
        a -= 360
    while a <= -180:
        a += 360
    return a


@dataclass
class Pose:
    root: tuple = (0.0, 0.0)
    tor: float = -90.0
    front: int = 1
    head: float = 172.0
    # bras : [upper, forearm] (proche / loin)
    armN: tuple = (20.0, 60.0)
    armF: tuple = (35.0, 55.0)
    # jambes : [thigh, shin, foot]
    legN: tuple = (15.0, -25.0, 95.0)
    legF: tuple = (-12.0, -10.0, 95.0)
    fist: tuple = (1, 1)      # poings fermes (proche, loin)
    face: int = 0             # 0 normal, 1 effort (dents), 2 yeux fermes (douleur), 3 cri
    extra: dict = field(default_factory=dict)

    def copy(self, **kw):
        return replace(self, **kw)


class Frame:
    """Repere du torse + positions de toutes les articulations pour une pose."""

    def __init__(self, p: Pose):
        self.p = p
        self.up = ang_vec(p.tor)
        self.fw = (p.front * -self.up[1], p.front * self.up[0])
        self.P = p.root
        self.N = v_add(self.P, v_mul(self.up, TORSO))
        self.S = v_add(v_sub(self.N, v_mul(self.up, SHOULDER_DROP)), v_mul(self.fw, -0.5))
        self.H = v_add(self.P, v_mul(self.fw, 0.5))
        self.headDir = self.local(p.head)
        self.C = v_add(self.N, v_mul(self.headDir, NECK))
        # le visage regarde perpendiculairement au cou, cote poitrine
        self.faceDir = self.local(p.head - 90)
        self.limbs = {}
        for side in ("N", "F"):
            a = getattr(p, "arm" + side)
            u = self.local(a[0])
            e = v_add(self.S, v_mul(u, UPPER))
            f = self.local(a[0] + a[1])
            w = v_add(e, v_mul(f, FORE))
            h = v_add(w, v_mul(f, HAND))
            self.limbs["arm" + side] = (self.S, e, w, h)
            l = getattr(p, "leg" + side)
            t = self.local(l[0])
            k = v_add(self.H, v_mul(t, THIGH))
            s = self.local(l[0] + l[1])
            an = v_add(k, v_mul(s, SHIN))
            fd = self.local(l[0] + l[1] + l[2])
            toe = v_add(an, v_mul(fd, FOOT))
            self.limbs["leg" + side] = (self.H, k, an, toe)

    def local(self, a):
        """angle dans le repere du torse -> vecteur ecran."""
        r = math.radians(a)
        c, s = math.cos(r), math.sin(r)
        return (-self.up[0] * c + self.fw[0] * s, -self.up[1] * c + self.fw[1] * s)

    def to_local_angle(self, v):
        """vecteur ecran -> angle dans le repere du torse."""
        d = -(v[0] * self.up[0] + v[1] * self.up[1])
        f = v[0] * self.fw[0] + v[1] * self.fw[1]
        return math.degrees(math.atan2(f, d))


# ---------------------------------------------------------------- IK
def _ik2(base, target, l1, l2, bend):
    d = v_sub(target, base)
    dist = v_len(d)
    dist = max(min(dist, l1 + l2 - 0.01), abs(l1 - l2) + 0.01)
    a = math.degrees(math.atan2(d[1], d[0]))
    cos_t = (l1 * l1 + dist * dist - l2 * l2) / (2 * l1 * dist)
    t = math.degrees(math.acos(max(-1.0, min(1.0, cos_t))))
    a1 = a + bend * t
    j = v_add(base, v_mul(ang_vec(a1), l1))
    a2 = vec_ang(v_sub(target, j)) if v_len(v_sub(target, j)) > 0.01 else a1
    return a1, a2


def ik_arm(p: Pose, side, target, bend=1):
    """Place le poing du bras `side` ('N' ou 'F') sur target. bend=+1 coude vers le bas/arriere."""
    fr = Frame(p)
    a1, a2 = _ik2(fr.S, target, UPPER, FORE + HAND, bend * p.front)
    up = fr.to_local_angle(ang_vec(a1))
    fo = fr.to_local_angle(ang_vec(a2))
    return p.copy(**{"arm" + side: (up, wrap(fo - up))})


def ik_leg(p: Pose, side, target, bend=1, foot=None):
    """Place la cheville de la jambe `side` sur target. bend=+1 genou vers l'avant."""
    fr = Frame(p)
    a1, a2 = _ik2(fr.H, target, THIGH, SHIN, -bend * p.front)
    th = fr.to_local_angle(ang_vec(a1))
    sh = fr.to_local_angle(ang_vec(a2))
    old = getattr(p, "leg" + side)
    ft = old[2] if foot is None else foot
    return p.copy(**{"leg" + side: (th, wrap(sh - th), ft)})


def foot_flat(p: Pose, side, floor_dir=0.0):
    """Oriente le pied a plat (direction ecran floor_dir, 0 = vers la droite) cote avant."""
    fr = Frame(p)
    leg = getattr(p, "leg" + side)
    want = ang_vec(floor_dir if p.front > 0 else 180 - floor_dir)
    fa = fr.to_local_angle(want)
    return p.copy(**{"leg" + side: (leg[0], leg[1], wrap(fa - leg[0] - leg[1]))})


def look(p: Pose, world_deg):
    """oriente la tete pour que le visage regarde dans la direction ecran world_deg."""
    fr = Frame(p)
    a = fr.to_local_angle(ang_vec(world_deg))
    return p.copy(head=wrap(a + 90))


def place(p: Pose, joint, target):
    """Deplace toute la pose pour que l'articulation `joint` tombe sur target."""
    fr = Frame(p)
    src = joint_pos(fr, joint)
    d = v_sub(target, src)
    return p.copy(root=v_add(p.root, d))


def joint_pos(fr: Frame, joint):
    if joint in ("P", "root"):
        return fr.P
    if joint == "N":
        return fr.N
    if joint == "C" or joint == "head":
        return fr.C
    if joint == "S":
        return fr.S
    limb, idx = joint[:-1], joint[-1]
    names = {"armN": "armN", "armF": "armF", "legN": "legN", "legF": "legF"}
    pts = fr.limbs[names[limb]]
    return pts[int(idx)]


def mirror(p: Pose, axis_x):
    """Pose miroir horizontal autour de x = axis_x."""
    t = 180 - p.tor
    return p.copy(root=(2 * axis_x - p.root[0], p.root[1]), tor=wrap(t), front=-p.front)


# ---------------------------------------------------------------- interpolation
def ease(t, kind="io"):
    if kind == "lin":
        return t
    if kind == "in":
        return t * t
    if kind == "out":
        return 1 - (1 - t) * (1 - t)
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def lerp_ang(a, b, t):
    return a + wrap(b - a) * t


def lerp_tuple(a, b, t, ang=True):
    return tuple(lerp_ang(x, y, t) if ang else lerp(x, y, t) for x, y in zip(a, b))


def interp(a: Pose, b: Pose, t):
    """Interpolation de deux poses ; le cote de la poitrine bascule a mi-chemin."""
    if a.front != b.front:
        # retourne b dans le repere de a pour interpoler les angles, puis bascule a mi-chemin
        src = a if t < 0.5 else b
        return src.copy(root=(lerp(a.root[0], b.root[0], t), lerp(a.root[1], b.root[1], t)))
    return Pose(
        root=(lerp(a.root[0], b.root[0], t), lerp(a.root[1], b.root[1], t)),
        tor=lerp_ang(a.tor, b.tor, t),
        front=a.front,
        head=lerp_ang(a.head, b.head, t),
        armN=lerp_tuple(a.armN, b.armN, t),
        armF=lerp_tuple(a.armF, b.armF, t),
        legN=lerp_tuple(a.legN, b.legN, t),
        legF=lerp_tuple(a.legF, b.legF, t),
        fist=a.fist if t < 0.5 else b.fist,
        face=a.face if t < 0.5 else b.face,
        extra=a.extra if t < 0.5 else b.extra,
    )
