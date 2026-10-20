"""Bibliotheque de poses (reperes canoniques).

Sprites individuels : cadre 104 x 112, bassin a x = 52, sol a y = 108, regard vers la DROITE.
Sprites de paire    : cadre 176 x 112, ancre a x = 88, sol a y = 108.
  Convention de paire : A (attaquant / dessus) a DROITE regardant a GAUCHE,
  B (defenseur / dessous) a GAUCHE ; au sol, B est sur le dos tete a GAUCHE.
"""
from skel import Pose, Frame, ik_arm, ik_leg, foot_flat, look, place, joint_pos, v_add, v_sub, v_mul, wrap

FLOOR = 108.0
ANKLE = FLOOR - 3.2


# ---------------------------------------------------------------- debout
def stand(x, facing=1, lean=14.0, h=33.5, step_n=11.0, step_f=-13.0, hands=None, look_deg=None,
          knee=1, face=0, fist=(1, 1), tor_extra=0.0):
    """Garde de jiu-jitsu debout. facing=+1 regarde a droite. hands = ((xn,yn),(xf,yf)) en absolu."""
    f = facing
    p = Pose(root=(x, ANKLE - h), tor=wrap(-90 + f * (lean + tor_extra)), front=f, head=170,
             armN=(50, 70), armF=(40, 80), legN=(20, -30, 95), legF=(-10, -15, 95), fist=fist, face=face)
    p = ik_leg(p, "N", (x + f * step_n, ANKLE), bend=knee)
    p = ik_leg(p, "F", (x + f * step_f, ANKLE), bend=knee)
    p = foot_flat(p, "N")
    p = foot_flat(p, "F")
    if hands is None:
        fr = Frame(p)
        hands = ((fr.S[0] + f * 22, fr.S[1] + 5), (fr.S[0] + f * 17, fr.S[1] + 11))
    p = ik_arm(p, "N", hands[0], bend=1)
    p = ik_arm(p, "F", hands[1], bend=1)
    p = look(p, 0 if f > 0 else 180) if look_deg is None else look(p, look_deg)
    return p


def head_down(p, deg=25):
    return p.copy(head=wrap(p.head - deg))


def shift(p, dx, dy=0.0):
    return p.copy(root=(p.root[0] + dx, p.root[1] + dy))


def mirror_x(p, axis):
    """Miroir horizontal d'une pose (autour de x = axis)."""
    return p.copy(root=(2 * axis - p.root[0], p.root[1]), tor=wrap(180 - p.tor), front=-p.front)


def arm_to(p, side, target, bend=1):
    return ik_arm(p, side, target, bend)


def leg_to(p, side, target, bend=1, foot=None):
    return ik_leg(p, side, target, bend, foot)


def jpos(p, joint):
    return joint_pos(Frame(p), joint)


# ---------------------------------------------------------------- au sol
def supine(px, head_left=True, lift=0.0, head_up=12.0, py=None):
    """Sur le dos. lift : relevement du buste (degres)."""
    y = FLOOR - 9.5 if py is None else py
    if head_left:
        p = Pose(root=(px, y), tor=wrap(180 + lift), front=1, head=180 - head_up, armN=(70, 50), armF=(80, 40),
                 legN=(120, -100, 90), legF=(115, -95, 90))
    else:
        p = Pose(root=(px, y), tor=wrap(-lift), front=-1, head=180 - head_up, armN=(70, 50), armF=(80, 40),
                 legN=(120, -100, 90), legF=(115, -95, 90))
    return p


def prone(px, head_left=True, py=None):
    """A plat ventre."""
    y = FLOOR - 9.0 if py is None else py
    if head_left:
        return Pose(root=(px, y), tor=180, front=-1, head=150, armN=(100, 60), armF=(110, 50),
                    legN=(-5, -10, 80), legF=(0, -15, 80))
    return Pose(root=(px, y), tor=0, front=1, head=150, armN=(100, 60), armF=(110, 50),
                legN=(-5, -10, 80), legF=(0, -15, 80))
