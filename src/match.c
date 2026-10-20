/*
 * BJJ TATAMI - le combat.
 *
 * DEBOUT  : B tenu = prises du kimono ; A = double-leg, HAUT+A = projection judo (prises),
 *           BAS+A = single-leg, BAS+B = tirer la garde (prises), C = sprawl, HAUT+C = bloquer la hanche.
 * AU SOL  : A + direction = transition, B + direction = soumission ; C + meme direction = bloquer.
 * SOUMISSION : 3 portes, chacun deplace son curseur (l'attaquant doit couvrir celui du defenseur).
 * PENALITES : passivite et sortie de la zone ; 2e = avantage adverse, 3e = 2 points, 4e = disqualification.
 */
#include "game.h"

/* ------------------------------------------------------------------ reglages */
#define MIN_GAP         34
#define REACH_GRIP      58
#define REACH_SHOOT     72
#define GAP_GRIP        46
#define START_GAP       104
#define STALL_FRAMES    1500
#define BAR_CELLS       24

enum { POS_GUARD, POS_HALF, POS_SIDE, POS_KNEE, POS_MOUNT, POS_BACK, POS_COUNT };
enum { PH_INTRO, PH_STAND, PH_ANIM, PH_GROUND, PH_SUB, PH_TAP, PH_DECIDE, PH_OVER };
enum { ACT_IDLE, ACT_SHOOT, ACT_JUDO, ACT_DEFEND, ACT_STUMBLE };
enum { TD_DOUBLE, TD_JUDO, TD_SINGLE };
enum { DEF_SPRAWL, DEF_HIP };
enum { NX_NONE, NX_GROUND, NX_STAND, NX_SUB, NX_ESCAPE, NX_FIGHT };
enum { DIR_UP, DIR_SIDE, DIR_DOWN };
enum { PEND_MOVE, PEND_SUB };

#define STANDUP     (-1)

typedef struct { const char* name; s8 target; u8 swap; u8 points; u8 adv; s8 diff; u8 seq; } Move;
typedef struct { const char* name; u8 family; s8 diff; u8 seq; } SubMove;

#define MV(n, t, s, p, a, d, q) { n, t, s, p, a, d, q }
#define NOMV                    { NULL, 0, 0, 0, 0, 0, 0 }
#define SB(n, f, d, q)          { n, f, d, q }
#define NOSB                    { NULL, 0, 0, 0 }

static const Move topMoves[POS_COUNT][3] =
{
    { MV("OUVRIR LA GARDE", POS_HALF, 0, 0, 1, 0, SEQ_T_GUARD_HALF),
      MV("PASSAGE TOREANDO", POS_SIDE, 0, 3, 0, 3, SEQ_T_GUARD_SIDE),
      MV("SE RELEVER", STANDUP, 0, 0, 0, -1, SEQ_T_GUARD_STAND) },
    { MV("PASSAGE DE GARDE", POS_SIDE, 0, 3, 0, 1, SEQ_T_HALF_SIDE),
      MV("PASSAGE MONTEE", POS_MOUNT, 0, 7, 0, 4, SEQ_T_HALF_MOUNT),
      MV("SE RELEVER", STANDUP, 0, 0, 0, 0, SEQ_T_HALF_STAND) },
    { MV("MONTEE", POS_MOUNT, 0, 4, 0, 1, SEQ_T_SIDE_MOUNT),
      MV("GENOU AU VENTRE", POS_KNEE, 0, 2, 0, 0, SEQ_T_SIDE_KNEE),
      MV("PRISE DE DOS", POS_BACK, 0, 4, 0, 3, SEQ_T_SIDE_BACK) },
    { MV("MONTEE", POS_MOUNT, 0, 4, 0, 0, SEQ_T_KNEE_MOUNT),
      NOMV,
      MV("RETOUR 100 KILOS", POS_SIDE, 0, 0, 0, -2, SEQ_T_KNEE_SIDE) },
    { MV("PRISE DE DOS", POS_BACK, 0, 4, 0, 1, SEQ_T_MOUNT_BACK), NOMV, NOMV },
    { NOMV, NOMV, NOMV },
};
static const Move botMoves[POS_COUNT][3] =
{
    { MV("BALAYAGE CISEAUX", POS_HALF, 1, 2, 0, 1, SEQ_B_GUARD_SWEEP),
      MV("PRISE DE DOS", POS_BACK, 1, 4, 0, 4, SEQ_B_GUARD_BACK),
      MV("SE RELEVER", STANDUP, 0, 0, 0, 0, SEQ_B_GUARD_STAND) },
    { MV("REPRENDRE LA GARDE", POS_GUARD, 0, 0, 0, -1, SEQ_B_HALF_GUARD),
      MV("BALAYAGE", POS_HALF, 1, 2, 0, 2, SEQ_B_HALF_SWEEP),
      MV("SE RELEVER", STANDUP, 0, 0, 0, 1, SEQ_B_HALF_STAND) },
    { MV("SORTIE DE HANCHE", POS_HALF, 0, 0, 0, 0, SEQ_B_SIDE_HALF),
      MV("REPRENDRE LA GARDE", POS_GUARD, 0, 0, 0, 2, SEQ_B_SIDE_GUARD), NOMV },
    { MV("SORTIE", POS_HALF, 0, 0, 0, 0, SEQ_B_KNEE_HALF), NOMV, NOMV },
    { MV("UPA", POS_GUARD, 1, 0, 0, 1, SEQ_B_MOUNT_UPA),
      MV("COUDE-GENOU", POS_HALF, 0, 0, 0, 0, SEQ_B_MOUNT_HALF), NOMV },
    { MV("SE RETOURNER", POS_GUARD, 1, 0, 0, 1, SEQ_B_BACK_ROLL),
      MV("GLISSER AU SOL", POS_HALF, 0, 0, 0, 0, SEQ_B_BACK_HALF), NOMV },
};
/* seq = index de la sequence _SET (LOCK = +1, TAP = +2) */
static const SubMove topSubs[POS_COUNT][3] =
{
    { NOSB, NOSB, SB("CLE DE CHEVILLE", SUBF_LEG, 2, SEQ_S_ANKLE_SET) },
    { SB("KIMURA", SUBF_KIM, 1, SEQ_S_KIMURA_H_SET), SB("ETRANGLEMENT D'ARCE", SUBF_CHOKE, 2, SEQ_S_DARCE_SET), NOSB },
    { SB("KIMURA", SUBF_KIM, 0, SEQ_S_KIMURA_S_SET), SB("AMERICANA", SUBF_ARM, 1, SEQ_S_AMERICANA_SET), NOSB },
    { SB("CLE DE BRAS", SUBF_ARM, 0, SEQ_S_ARMBAR_K_SET), NOSB, NOSB },
    { SB("CLE DE BRAS", SUBF_ARM, -1, SEQ_S_ARMBAR_M_SET), SB("ETRANGLEMENT CROISE", SUBF_CHOKE, 0, SEQ_S_CROSS_SET), NOSB },
    { SB("MATA LEAO", SUBF_CHOKE, -2, SEQ_S_RNC_SET), SB("ARC ET FLECHE", SUBF_CHOKE, -1, SEQ_S_BOWARROW_SET), NOSB },
};
static const SubMove botSubs[POS_COUNT][3] =
{
    { SB("TRIANGLE", SUBF_TRI, 1, SEQ_S_TRIANGLE_SET), SB("CLE DE BRAS", SUBF_ARM, 1, SEQ_S_ARMBAR_G_SET),
      SB("GUILLOTINE", SUBF_CHOKE, 2, SEQ_S_GUILLOTINE_SET) },
    { SB("KIMURA", SUBF_KIM, 2, SEQ_S_KIMURA_HB_SET), NOSB, NOSB },
    { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB },
};

static const char* const posName[POS_COUNT] =
{ "GARDE FERMEE", "DEMI-GARDE", "100 KILOS", "GENOU AU VENTRE", "MONTEE", "DOS" };
static const char* const tdName[3] = { "DOUBLE-LEG", "PROJECTION JUDO", "SINGLE-LEG" };
static const char dirChar[3] = { '^', '>', 'v' };

static const u16 cpuDeny[3]   = { 22, 40, 58 };
static const u16 cpuRead[3]   = { 50, 62, 75 };
static const u16 cpuReact[3]  = { 16, 11, 7 };
static const u16 cpuEscape[3] = { 10, 7, 5 };

/* ------------------------------------------------------------------ etat */
static u16 phase;
static s32 matchFrames;
static s16 phaseTimer;
static u16 pos, top;
static bool flip;
static s16 anchor;
static u16 next, nextPos;

/* attaque au sol */
static bool pend;
static u16 pendWho, pendKind, pendDir;
static s16 pendTimer;
static bool pendDenyUsed;
static s16 groundCd[2];

/* soumission */
static u16 subWho;
static const SubMove* subM;
static u16 gate;
static s16 tug, lenA, lenD;

#define STAT(i, s)  ((s16)P[i].def->stat[s])
#define OPP(i)      (1 - (i))

/* ------------------------------------------------------------------ outils */
static void spend(u16 i, s16 amount)
{
    s16 c = P[i].def->stat[ST_CARDIO];
    P[i].souffle -= amount;
    if (P[i].souffle < 0) P[i].souffle = 0;
    P[i].cardio -= (amount * (11 - c)) / 40;
    if (P[i].cardio < 30) P[i].cardio = 30;
}

static void regen(u16 i, u16 every)
{
    if ((frameCount % every) == 0 && P[i].souffle < P[i].cardio) P[i].souffle++;
}

static u16 dirOf(u16 i)
{
    u16 p = P[i].pad;
    if (p & BUTTON_DOWN) return DIR_DOWN;
    if (p & (BUTTON_LEFT | BUTTON_RIGHT)) return DIR_SIDE;
    return DIR_UP;
}

static s16 dist(void) { s16 d = P[1].x - P[0].x; return d < 0 ? -d : d; }

static void headPos(u16 i, s16* x, s16* y)
{
    if (phase == PH_STAND || phase == PH_INTRO || phase == PH_OVER)
    {
        *x = P[i].x;
        *y = FLOOR_SCREEN - 104;
        return;
    }
    actorsPairWorldPos(i == top ? 0 : 1, x, y);
    *y -= 30;
}

static void popAt(u16 who, u16 kind)
{
    s16 x, y;
    headPos(who, &x, &y);
    fxPop(kind, x, y);
}

static void award(u16 who, u16 pts, u16 adv)
{
    P[who].points += pts;
    P[who].adv += adv;
    if (pts)
    {
        s16 x, y;
        headPos(who, &x, &y);
        popAt(who, pts == 2 ? POP_P2 : (pts == 3 ? POP_P3 : POP_P4));
        refSignal(REF_POINTS, x, 70);
        crowdReact(FALSE);
        stageCheer(120);
    }
    else if (adv)
    {
        s16 x, y;
        headPos(who, &x, &y);
        popAt(who, POP_AV);
        refSignal(REF_ADV, x, 60);
    }
}

static void endMatch(s16 winner, const char* method, bool bySub, u16 ban);

static void penalty(u16 i, const char* why)
{
    s16 x, y;
    P[i].pen++;
    P[i].idle = 0;
    headPos(i, &x, &y);
    popAt(i, POP_PEN);
    refSignal(REF_PEN, x, 80);
    hudMessageWho(i, why, 0);
    sfx(200, 14);
    if (P[i].pen == 2) P[OPP(i)].adv++;
    if (P[i].pen == 3) P[OPP(i)].points += 2;
    if (P[i].pen >= 4) endMatch(OPP(i), "DISQUALIFICATION", FALSE, BAN_DESCLASSIFICADO);
}

static void stallCheck(u16 i)
{
    if (++P[i].idle < STALL_FRAMES) return;
    penalty(i, "PENALITE : PASSIVITE");
}

static void acted(u16 i)
{
    P[i].idle = 0;
    P[i].attempts++;
}

/* ------------------------------------------------------------------ aides */
static void standHints(void)
{
    static const char* const h[3] = {
        "@A DOUBLE-LEG  ^@A JUDO   v@A SINGLE-LEG",
        "@B PRISES (TENIR)    v@B TIRER LA GARDE",
        "@C SPRAWL      ^@C BLOQUER LA HANCHE" };
    if (!helpOn || !P[0].human || numHumans != 1) { hudClearHints(); return; }
    hudHints(h, 3);
}

static const Move* moveFor(u16 who, u16 d)
{
    const Move* m = (who == top) ? &topMoves[pos][d] : &botMoves[pos][d];
    return m->name ? m : NULL;
}

static const SubMove* subFor(u16 who, u16 d)
{
    const SubMove* s = (who == top) ? &topSubs[pos][d] : &botSubs[pos][d];
    return s->name ? s : NULL;
}

static void groundHints(void)
{
    static char lines[3][41];
    static const char* ptr[3];
    u16 d, n = 0;
    char b[41];

    b[0] = 0;
    scat(b, P[top].label, 40);
    scat(b, " DESSUS - ", 40);
    scat(b, posName[pos], 40);
    clearRow(6);
    wtxtC(b, 6);
    if (!helpOn || !P[0].human || numHumans != 1) { hudClearHints(); return; }
    for (d = 0; d < 3; d++)
    {
        const Move* m = moveFor(0, d);
        const SubMove* s = subFor(0, d);
        char* l = lines[n];
        u16 k;
        if (!m && !s) continue;
        for (k = 0; k < 40; k++) l[k] = ' ';
        l[40] = 0;
        l[0] = dirChar[d];
        if (m) { l[1] = '@'; l[2] = 'A'; for (k = 0; m->name[k] && k < 16; k++) l[4 + k] = m->name[k]; }
        if (s) { l[21] = '@'; l[22] = 'B'; for (k = 0; s->name[k] && k < 16; k++) l[24 + k] = s->name[k]; }
        ptr[n] = l;
        n++;
    }
    if (n) hudHints(ptr, n);
    else hudClearHints();
}

/* ------------------------------------------------------------------ fin de combat */
static void endMatch(s16 winner, const char* method, bool bySub, u16 ban)
{
    if (phase == PH_DECIDE || phase == PH_OVER) return;
    resWinner = winner;
    scopy(resMethod, method, 31);
    resBySub = bySub;
    resTimeLeft = (s16)(matchFrames / 60);
    phase = PH_DECIDE;
    phaseTimer = 150;
    pend = FALSE;
    hudSubClear();
    hudClearHints();
    clearRow(6);
    fxBanner(ban, 150);
    crowdReact(TRUE);
    stageCheer(300);
    refSignal(REF_STOP, (P[0].x + P[1].x) / 2, 100);
}

static void timeUp(void)
{
    char b[24];
    s16 w = -1;
    const char* how = "DECISION DES ARBITRES";
    if (P[0].points != P[1].points)
    {
        char n0[4], n1[4];
        w = (P[0].points > P[1].points) ? 0 : 1;
        numStr(P[w].points, 2, n0);
        numStr(P[OPP(w)].points, 2, n1);
        b[0] = 0;
        scat(b, "AUX POINTS ", 23);
        scat(b, n0, 23);
        scat(b, "-", 23);
        scat(b, n1, 23);
        how = b;
    }
    else if (P[0].adv != P[1].adv) { w = (P[0].adv > P[1].adv) ? 0 : 1; how = "AUX AVANTAGES"; }
    else if (P[0].pen != P[1].pen) { w = (P[0].pen < P[1].pen) ? 0 : 1; how = "AUX PENALITES"; }
    else if (P[0].attempts != P[1].attempts) w = (P[0].attempts > P[1].attempts) ? 0 : 1;
    else how = "MATCH NUL";
    endMatch(w, how, FALSE, BAN_TEMPO);
}

/* passage a l'image finale : vainqueur debout, arbitre qui leve son bras */
static void startOutro(void)
{
    u16 i;
    s16 cx;
    if (phase != PH_STAND && phase != PH_INTRO)
    {
        s16 x0, y0, x1, y1;
        actorsPairWorldPos(0, &x0, &y0);
        actorsPairWorldPos(1, &x1, &y1);
        cx = (x0 + x1) / 2;
    }
    else cx = (P[0].x + P[1].x) / 2;
    if (cx < MAT_MIN + 60) cx = MAT_MIN + 60;
    if (cx > MAT_MAX - 60) cx = MAT_MAX - 60;
    phase = PH_OVER;
    phaseTimer = 260;
    actorsSoloMode();
    P[0].x = cx - 30;
    P[1].x = cx + 30;
    P[0].dir = 1;
    P[1].dir = -1;
    for (i = 0; i < 2; i++)
    {
        P[i].animRev = FALSE;
        if (resWinner < 0) actorsSoloSet(i, SOLO_IDLE, TRUE);
        else actorsSoloSet(i, (s16)i == resWinner ? SOLO_VICTORY : SOLO_DEFEAT, TRUE);
    }
    if (resWinner >= 0)
    {
        refSignal(REF_WINNER, P[resWinner].x, 250);
        fxBanner(BAN_VITORIA, 200);
        if (P[resWinner].human || numHumans == 0) playTune(tuneWin, 6);
        else playTune(tuneLose, 4);
    }
    else
    {
        fxBanner(BAN_EMPATE, 200);
        playTune(tuneLose, 4);
    }
    refFollow(cx);
    stageSetCamera(cx);
}

/* ------------------------------------------------------------------ sequences en paire */
static void playSeq(u16 seq, u16 roleA, bool rev, u16 nx)
{
    actorsPairStart(seq, anchor, flip, roleA, rev);
    phase = PH_ANIM;
    next = nx;
    hudClearHints();
}

static void enterGround(void)
{
    u16 i;
    phase = PH_GROUND;
    pend = FALSE;
    actorsPairStart(SEQ_G_GUARD + pos, anchor, flip, top, FALSE);
    for (i = 0; i < 2; i++)
    {
        groundCd[i] = 30;
        P[i].aiWant = 0;
        P[i].aiDenyAt = 0;
        P[i].aiTimer = 50 + (random() % 50);
        P[i].grip = 0;
    }
    groundHints();
    refFollow(anchor);
}

static void standFromPair(void)
{
    s16 x, y;
    u16 a = actorsPairFlip() ? 1 : 0;
    u16 roleA = top;
    (void)a;
    actorsPairWorldPos(0, &x, &y);
    P[roleA].x = x;
    actorsPairWorldPos(1, &x, &y);
    P[OPP(roleA)].x = x;
}

static void enterStand(void)
{
    u16 i;
    s16 cx = (P[0].x + P[1].x) / 2;
    phase = PH_STAND;
    pend = FALSE;
    actorsSoloMode();
    if (dist() < START_GAP - 30)
    {
        s16 l = (P[0].x < P[1].x) ? 0 : 1;
        P[l].x = cx - 36;
        P[OPP(l)].x = cx + 36;
    }
    for (i = 0; i < 2; i++)
    {
        if (P[i].x < MAT_MIN) P[i].x = MAT_MIN;
        if (P[i].x > MAT_MAX) P[i].x = MAT_MAX;
        P[i].act = ACT_IDLE;
        P[i].cooldown = 30;
        P[i].grip = 0;
        P[i].aiWant = 0;
        P[i].aiTimer = 30;
        P[i].animRev = FALSE;
        actorsSoloSet(i, SOLO_IDLE, TRUE);
    }
    P[0].dir = (P[0].x < P[1].x) ? 1 : -1;
    P[1].dir = -P[0].dir;
    standHints();
    clearRow(6);
}

static void seqDone(void)
{
    const PairSeq* s = &pairSeqs[actorsPairSeq()];
    bool rev = FALSE;
    (void)rev;
    switch (next)
    {
        case NX_GROUND:
            if (s->endFlip) flip = !flip;
            if (s->swap) top = OPP(top);
            pos = nextPos;
            enterGround();
            break;
        case NX_STAND:
            standFromPair();
            enterStand();
            break;
        case NX_FIGHT:
            standFromPair();
            enterStand();
            fxBanner(BAN_COMBATE, 90);
            refSignal(REF_START, (P[0].x + P[1].x) / 2, 40);
            sfx(880, 10);
            break;
        case NX_SUB:
            phase = PH_SUB;
            actorsPairStart(subM->seq + 1, anchor, flip, top, FALSE);
            break;
        case NX_ESCAPE:
            enterGround();
            break;
    }
}

/* ==================================================================== DEBOUT */
static void moveFighter(u16 i, s16 dx)
{
    s16 nx = P[i].x + dx;
    s16 ox = P[OPP(i)].x;
    s16 dn = nx - ox, d0 = P[i].x - ox;
    if (dn < 0) dn = -dn;
    if (d0 < 0) d0 = -d0;
    if (dn < MIN_GAP && dn < d0) return;
    if ((P[i].x < ox) != (nx < ox)) return;
    if (nx < MAT_MIN || nx > MAT_MAX)
    {
        /* recule hors de la zone : fuite */
        if (dn > d0)
        {
            P[i].idle += 25;
            if (P[i].idle >= STALL_FRAMES)
            {
                penalty(i, "PENALITE : SORTIE DE ZONE");
                P[0].x = WORLD_W / 2 - START_GAP / 2;
                P[1].x = WORLD_W / 2 + START_GAP / 2;
            }
        }
        return;
    }
    P[i].x = nx;
}

static void soloFor(u16 i, u16 anim, bool rev)
{
    P[i].animRev = rev;
    actorsSoloSet(i, anim, FALSE);
}

static void startShoot(u16 i, u16 type)
{
    if (type == TD_JUDO)
    {
        if (P[i].grip < 40 || dist() > REACH_GRIP)
        {
            hudMessageWho(i, "PAS ASSEZ DE PRISES !", 0);
            P[i].cooldown = 20;
            return;
        }
        P[i].act = ACT_JUDO;
        P[i].actTimer = 18;
        popAt(i, POP_DANGER);
        soloFor(i, SOLO_GRIP_HOLD, FALSE);
    }
    else
    {
        P[i].act = ACT_SHOOT;
        soloFor(i, SOLO_SHOOT, FALSE);
        actorsSoloSet(i, SOLO_SHOOT, TRUE);
    }
    P[i].tdType = type;
    spend(i, 10);
    acted(i);
    sfx(220, 6);
}

static void startDefend(u16 i, u16 kind)
{
    P[i].act = ACT_DEFEND;
    P[i].defKind = kind;
    P[i].defTimer = 26;
    spend(i, 4);
    soloFor(i, kind == DEF_HIP ? SOLO_HIPBLOCK : SOLO_SPRAWL, FALSE);
    actorsSoloSet(i, kind == DEF_HIP ? SOLO_HIPBLOCK : SOLO_SPRAWL, TRUE);
}

/* point de depart d'une sequence de projection : ancre et miroir */
static void setupStandingPair(u16 att, u16 seq, bool fromShoot)
{
    const PairFrame* f0 = &pairSeqs[seq].f[0];
    s16 ax;
    flip = P[att].x < P[OPP(att)].x;            /* canonique : l'attaquant est a droite */
    if (fromShoot)
    {
        const SoloAnim* a = &soloAnims[SOLO_SHOOT];
        ax = P[att].x + a->f[a->n - 1].dx * P[att].dir;
        anchor = flip ? ax + (f0->ax - PAIR_AX) : ax - (f0->ax - PAIR_AX);
    }
    else anchor = (P[0].x + P[1].x) / 2;
    if (anchor < 100) anchor = 100;
    if (anchor > WORLD_W - 100) anchor = WORLD_W - 100;
    top = att;
}

static void resolveShoot(u16 att)
{
    u16 def = OPP(att);
    u16 type = P[att].tdType;
    s16 chance;
    bool ok;
    u16 seqOk, seqKo;

    if (type != TD_JUDO && dist() > REACH_SHOOT)
    {
        hudMessageWho(att, "TROP LOIN !", 0);
        P[att].act = ACT_STUMBLE;
        P[att].cooldown = 40;
        soloFor(att, SOLO_STUMBLE, FALSE);
        actorsSoloSet(att, SOLO_STUMBLE, TRUE);
        return;
    }
    seqOk = (type == TD_DOUBLE) ? SEQ_TD_DOUBLE : (type == TD_SINGLE ? SEQ_TD_SINGLE : SEQ_TD_JUDO);
    seqKo = (type == TD_DOUBLE) ? SEQ_TD_DOUBLE_SPRAWL : (type == TD_SINGLE ? SEQ_TD_SINGLE_SPRAWL : SEQ_TD_JUDO_BLOCK);

    if (P[def].act == ACT_DEFEND && P[def].defKind == (type == TD_JUDO ? DEF_HIP : DEF_SPRAWL))
    {
        hudMessageWho(def, type == TD_JUDO ? "HANCHE BLOQUEE ! CONTRE" : "SPRAWL ! CONTRE", 0);
        spend(att, 12);
        crowdReact(FALSE);
        stageCheer(60);
        ok = FALSE;
    }
    else
    {
        chance = 45 + (STAT(att, ST_LUTTE) - STAT(def, ST_LUTTE)) * 6 + (P[att].souffle - P[def].souffle) / 4;
        if (type == TD_DOUBLE) chance += 5;
        if (type == TD_JUDO) chance += P[att].grip / 3 - 5;
        if (P[def].act == ACT_DEFEND) chance += 15;
        if (chance < 10) chance = 10;
        if (chance > 90) chance = 90;
        ok = (s16)(random() % 100) < chance;
        if (!ok) hudMessageWho(def, "DEFEND LA PROJECTION", 0);
    }
    setupStandingPair(att, ok ? seqOk : seqKo, type != TD_JUDO);
    if (ok)
    {
        hudMessageWho(att, tdName[type], 2);
        P[att].points += 2;
        popAt(att, POP_P2);
        refSignal(REF_POINTS, P[att].x, 80);
        nextPos = (type == TD_DOUBLE) ? POS_GUARD : (type == TD_SINGLE ? POS_HALF : POS_SIDE);
        playSeq(seqOk, att, FALSE, NX_GROUND);
    }
    else playSeq(seqKo, att, FALSE, NX_STAND);
}

static void tryPullGuard(u16 i)
{
    if (dist() > REACH_GRIP) return;
    if (P[i].grip < 25) { hudMessageWho(i, "IL FAUT DES PRISES", 0); P[i].cooldown = 15; return; }
    acted(i);
    hudMessageWho(i, "TIRE LA GARDE", 0);
    setupStandingPair(i, SEQ_PULL_GUARD, FALSE);
    nextPos = POS_GUARD;
    playSeq(SEQ_PULL_GUARD, i, FALSE, NX_GROUND);
}

/* ---------------- IA debout */
static void aiStand(u16 i)
{
    Player* me = &P[i];
    Player* op = &P[OPP(i)];
    s16 d = dist();
    s16 r;
    u16 toward = (op->x > me->x) ? BUTTON_RIGHT : BUTTON_LEFT;
    u16 away = (op->x > me->x) ? BUTTON_LEFT : BUTTON_RIGHT;

    if ((op->act == ACT_SHOOT && op->animFrame == 0 && op->animTick == 1) ||
        (op->act == ACT_JUDO && op->actTimer == 17))
    {
        u16 chance = 25 + difficulty * 20 + me->def->stat[ST_LUTTE] * 2;
        if ((random() % 100) < chance)
        {
            bool read = (random() % 100) < (u16)(55 + difficulty * 15);
            u16 kind = (op->tdType == TD_JUDO) ? DEF_HIP : DEF_SPRAWL;
            if (!read) kind = 1 - kind;
            me->aiDenyAt = 2 + (random() % 6);
            me->aiDenyBtn = BUTTON_C | (kind == DEF_HIP ? BUTTON_UP : 0);
        }
    }
    if (me->aiDenyAt > 0 && --me->aiDenyAt == 0) me->aiWant = me->aiDenyBtn;
    if (me->aiWant) return;
    if (me->act != ACT_IDLE || me->cooldown > 0) return;

    if (--me->aiTimer <= 0)
    {
        me->aiTimer = 18 + (random() % 30) - difficulty * 5;
        r = random() % 100;
        me->aiMoveDir = 0;
        if (d <= REACH_SHOOT - 6)
        {
            if (me->grip >= 45 && d <= REACH_GRIP && me->def->stat[ST_LUTTE] >= 6 && r < 30) { me->aiWant = BUTTON_A | BUTTON_UP; return; }
            if (me->grip >= 25 && d <= REACH_GRIP && me->def->stat[ST_GARDE] >= 7 && r >= 85) { me->aiWant = BUTTON_B | BUTTON_DOWN; return; }
            if (r < 26 + difficulty * 8) { me->aiWant = BUTTON_A | ((r & 1) ? BUTTON_DOWN : 0); return; }
            me->aiMoveDir = (r > 80) ? -1 : 0;
        }
        else me->aiMoveDir = (r < 85) ? 1 : 0;
    }
    if (me->aiMoveDir > 0) me->pad |= toward;
    else if (me->aiMoveDir < 0) me->pad |= away;
    if (d <= REACH_GRIP && (frameCount & 63) < (u16)(30 + difficulty * 10)) me->pad |= BUTTON_B;
}

static bool standAction(u16 i)
{
    Player* f = &P[i];
    if (f->act == ACT_SHOOT)
    {
        if (actorsSoloDone(i))
        {
            f->act = ACT_IDLE;
            resolveShoot(i);
        }
        return TRUE;
    }
    if (f->act == ACT_JUDO)
    {
        if (--f->actTimer <= 0)
        {
            f->act = ACT_IDLE;
            resolveShoot(i);
        }
        return TRUE;
    }
    if (f->act == ACT_DEFEND)
    {
        if (--f->defTimer <= 0) { f->act = ACT_IDLE; f->cooldown = 16; soloFor(i, SOLO_IDLE, FALSE); }
        return TRUE;
    }
    if (f->act == ACT_STUMBLE)
    {
        if (actorsSoloDone(i)) { f->act = ACT_IDLE; soloFor(i, SOLO_IDLE, FALSE); }
        return TRUE;
    }
    if (f->cooldown > 0) { f->cooldown--; }
    return FALSE;
}

static void standInput(u16 i)
{
    Player* f = &P[i];
    bool moving = FALSE, gripping = FALSE;
    s16 sp = (f->souffle < 25 && (frameCount & 1)) ? 0 : 1;

    if (f->cooldown <= 0)
    {
        if (pressed(i, BUTTON_A))
        {
            u16 t = TD_DOUBLE;
            if (held(i, BUTTON_UP)) t = TD_JUDO;
            else if (held(i, BUTTON_DOWN)) t = TD_SINGLE;
            startShoot(i, t);
            return;
        }
        if (pressed(i, BUTTON_B) && held(i, BUTTON_DOWN)) { tryPullGuard(i); if (phase != PH_STAND) return; }
        if (pressed(i, BUTTON_C)) { startDefend(i, held(i, BUTTON_UP) ? DEF_HIP : DEF_SPRAWL); return; }
    }
    if (held(i, BUTTON_LEFT))  { moveFighter(i, -sp); moving = TRUE; }
    if (held(i, BUTTON_RIGHT)) { moveFighter(i, sp); moving = TRUE; }

    if (held(i, BUTTON_B) && !held(i, BUTTON_DOWN) && dist() <= REACH_GRIP)
    {
        gripping = TRUE;
        if (f->grip < 100) { f->grip += 2; if (f->grip > 100) f->grip = 100; }
        if (P[OPP(i)].grip > 0) P[OPP(i)].grip--;
        if ((frameCount % 20) == 0) spend(i, 1);
        /* en tenant, on se rapproche a distance de prise */
        if (dist() > GAP_GRIP + 2) moveFighter(i, f->dir);
    }
    if (!gripping && f->grip > 0 && (frameCount & 1)) f->grip--;
    if (dist() > REACH_GRIP + 14) f->grip = 0;

    if (gripping) soloFor(i, SOLO_GRIP_HOLD, FALSE);
    else if (moving)
    {
        bool fwd = (held(i, BUTTON_RIGHT) && f->dir > 0) || (held(i, BUTTON_LEFT) && f->dir < 0);
        soloFor(i, SOLO_WALK, !fwd);
    }
    else soloFor(i, f->souffle < 30 ? SOLO_TIRED : SOLO_IDLE, FALSE);
}

static void updateStand(void)
{
    u16 i;
    for (i = 0; i < 2; i++) if (!P[i].human) aiStand(i);
    for (i = 0; i < 2 && phase == PH_STAND; i++)
        if (!standAction(i)) standInput(i);
    if (phase != PH_STAND) return;
    P[0].dir = (P[0].x < P[1].x) ? 1 : -1;
    P[1].dir = -P[0].dir;
    for (i = 0; i < 2; i++)
    {
        if (P[i].act == ACT_IDLE) regen(i, 6);
        stallCheck(i);
    }
    actorsSoloUpdate();
    stageSetCamera((P[0].x + P[1].x) / 2);
    refFollow((P[0].x + P[1].x) / 2);
}

/* ==================================================================== SOL */
static void startPend(u16 who, u16 kind, u16 dir)
{
    s16 st = (kind == PEND_SUB) ? STAT(who, ST_SOUM) : (who == top ? STAT(who, ST_PASSAGE) : STAT(who, ST_GARDE));
    pend = TRUE;
    pendWho = who;
    pendKind = kind;
    pendDir = dir;
    pendTimer = ((kind == PEND_SUB) ? 22 : 24) - (st - 5);
    pendDenyUsed = FALSE;
    spend(who, (kind == PEND_SUB) ? 8 : 6);
    acted(who);
    popAt(who, POP_DANGER);
    sfx(kind == PEND_SUB ? 700 : 440, 6);
    {
        u16 d = OPP(who);
        if (!P[d].human)
        {
            u16 stat = (who == top) ? P[d].def->stat[ST_GARDE] : P[d].def->stat[ST_PASSAGE];
            if ((random() % 100) < (u16)(cpuDeny[difficulty] + stat * 2))
            {
                u16 guess = dir;
                if ((random() % 100) >= cpuRead[difficulty]) guess = (dir + 1 + (random() & 1)) % 3;
                P[d].aiDenyAt = 5 + (random() % 9);
                if (P[d].aiDenyAt >= pendTimer) P[d].aiDenyAt = pendTimer - 1;
                P[d].aiDenyBtn = BUTTON_C | (guess == DIR_DOWN ? BUTTON_DOWN : guess == DIR_SIDE ? BUTTON_RIGHT : BUTTON_UP);
            }
        }
    }
}

static void startSub(u16 who, const SubMove* s);

static void applyMove(u16 who, const Move* m)
{
    if (m->target == STANDUP)
    {
        hudMessageWho(who, m->name, 0);
        playSeq(m->seq, top, FALSE, NX_STAND);
        return;
    }
    nextPos = m->target;
    if (m->points) hudMessageWho(who, m->name, m->points);
    else if (m->adv) hudMessageWho(who, "AVANTAGE ! GARDE OUVERTE", 0);
    else hudMessageWho(who, m->name, 0);
    award(who, m->points, m->adv);
    playSeq(m->seq, top, FALSE, NX_GROUND);
}

static void resolvePend(void)
{
    u16 who = pendWho, d = OPP(who);
    pend = FALSE;
    groundCd[who] = 40;
    groundCd[d] = 12;
    if (pendKind == PEND_SUB)
    {
        startSub(who, subFor(who, pendDir));
        return;
    }
    {
        const Move* m = moveFor(who, pendDir);
        s16 sa = (who == top) ? STAT(who, ST_PASSAGE) : STAT(who, ST_GARDE);
        s16 sd = (who == top) ? STAT(d, ST_GARDE) : STAT(d, ST_PASSAGE);
        s16 chance = 58 + (sa - sd) * 5 + (P[who].souffle - P[d].souffle) / 4 - m->diff * 6;
        if (chance < 15) chance = 15;
        if (chance > 95) chance = 95;
        if ((s16)(random() % 100) < chance) applyMove(who, m);
        else { hudMessageWho(d, "RESISTE", 0); sfx(160, 6); actorsShake(10); }
    }
}

static void denied(u16 d)
{
    u16 who = pendWho;
    pend = FALSE;
    spend(who, 12);
    groundCd[who] = 30;
    groundCd[d] = 6;
    P[d].idle = 0;
    hudMessageWho(d, "BLOQUE ! BONNE LECTURE", 0);
    sfx(180, 10);
    actorsShake(14);
    crowdReact(FALSE);
    stageCheer(50);
}

static void aiGround(u16 i)
{
    Player* me = &P[i];
    s16 best = -1000, sc;
    u16 d, bestKind = 0, bestDir = 0;

    if (me->aiDenyAt > 0)
    {
        if (!pend || pendWho == i) me->aiDenyAt = 0;
        else if (--me->aiDenyAt == 0) me->aiWant = me->aiDenyBtn;
    }
    if (pend || groundCd[i] > 0 || me->aiWant) return;
    if (--me->aiTimer > 0) return;
    me->aiTimer = 110 - difficulty * 25 + (random() % 60);
    for (d = 0; d < 3; d++)
    {
        const Move* m = moveFor(i, d);
        const SubMove* s = subFor(i, d);
        if (m)
        {
            sc = m->points * 10 + m->adv * 6 + 20 - m->diff * 6 + (random() % 30);
            if (m->target == STANDUP) sc -= 25;
            if (i != top && m->target != STANDUP) sc += 15;
            if (sc > best) { best = sc; bestKind = PEND_MOVE; bestDir = d; }
        }
        if (s)
        {
            sc = 10 + me->def->stat[ST_SOUM] * 3 - s->diff * 6 + (random() % 30);
            if (s->family == me->def->special) sc += 15;
            if (P[OPP(i)].souffle < 40) sc += 20;
            if (sc > best) { best = sc; bestKind = PEND_SUB; bestDir = d; }
        }
    }
    if (best <= -1000) return;
    me->aiWant = ((bestKind == PEND_SUB) ? BUTTON_B : BUTTON_A) |
                 (bestDir == DIR_DOWN ? BUTTON_DOWN : bestDir == DIR_SIDE ? BUTTON_RIGHT : 0);
}

static void updateGround(void)
{
    u16 i;
    for (i = 0; i < 2; i++) if (!P[i].human) aiGround(i);
    if (pend)
    {
        u16 d = OPP(pendWho);
        if (!pendDenyUsed && pressed(d, BUTTON_C))
        {
            pendDenyUsed = TRUE;
            if (dirOf(d) == pendDir) { denied(d); return; }
        }
        if (--pendTimer <= 0) resolvePend();
        return;
    }
    for (i = 0; i < 2; i++)
    {
        if (groundCd[i] > 0) { groundCd[i]--; continue; }
        if (pressed(i, BUTTON_A) && moveFor(i, dirOf(i))) { startPend(i, PEND_MOVE, dirOf(i)); return; }
        if (pressed(i, BUTTON_B) && subFor(i, dirOf(i))) { startPend(i, PEND_SUB, dirOf(i)); return; }
    }
    regen(top, 5);
    regen(OPP(top), (pos >= POS_SIDE) ? 14 : 7);
    stallCheck(0);
    stallCheck(1);
}

/* ==================================================================== SOUMISSION */
static void subLayout(void)
{
    s16 maxA = (BAR_CELLS - lenA) * 16, maxD = (BAR_CELLS - lenD) * 16;
    P[subWho].cur = maxA / 2;
    P[OPP(subWho)].cur = (random() & 1) ? 0 : maxD;
}

static void startSub(u16 who, const SubMove* s)
{
    u16 d = OPP(who);
    subWho = who;
    subM = s;
    gate = 0;
    tug = 0;
    lenA = 3 + STAT(who, ST_SOUM) / 3 + (P[who].def->special == s->family ? 1 : 0) - s->diff - (P[who].souffle < 30 ? 1 : 0);
    if (lenA < 3) lenA = 3;
    if (lenA > 8) lenA = 8;
    lenD = 3 + (STAT(d, ST_SOUM) + STAT(d, ST_CARDIO)) / 6 - (P[d].souffle < 30 ? 1 : 0);
    if (lenD < 2) lenD = 2;
    subLayout();
    P[0].aiTimer = P[1].aiTimer = 0;
    hudClearHints();
    clearRow(6);
    hudMessageWho(who, s->name, 0);
    sfx(880, 10);
    crowdReact(FALSE);
    stageCheer(100);
    playSeq(s->seq, top, FALSE, NX_SUB);
}

static void aiSub(u16 i)
{
    Player* me = &P[i];
    s16 len = (i == subWho) ? lenA : lenD;
    s16 maxPos = (BAR_CELLS - len) * 16;
    if (--me->aiTimer <= 0)
    {
        me->aiTimer = ((i == subWho) ? cpuReact[difficulty] : cpuEscape[difficulty]) + (random() % 5);
        if (i == subWho)
        {
            s16 dc = P[OPP(i)].cur + lenD * 8;
            s16 noise = 49 - difficulty * 8;
            me->aiTarget = dc - lenA * 8 + ((s16)(random() % noise) - noise / 2);
        }
        else
        {
            s16 ac = P[OPP(i)].cur + lenA * 8;
            me->aiTarget = (ac > BAR_CELLS * 8) ? 0 : maxPos;
            if ((random() % 100) < 20) me->aiTarget = maxPos - me->aiTarget;
        }
        if (me->aiTarget < 0) me->aiTarget = 0;
        if (me->aiTarget > maxPos) me->aiTarget = maxPos;
    }
    if (me->cur < me->aiTarget - 8) me->pad |= BUTTON_RIGHT;
    else if (me->cur > me->aiTarget + 8) me->pad |= BUTTON_LEFT;
}

static void updateSub(void)
{
    u16 i, d = OPP(subWho);
    s16 a0, a1, d0, d1, ov;
    for (i = 0; i < 2; i++)
    {
        s16 len = (i == subWho) ? lenA : lenD;
        s16 maxPos = (BAR_CELLS - len) * 16;
        s16 sp;
        if (!P[i].human) aiSub(i);
        sp = (i == subWho) ? 5 + STAT(i, ST_SOUM) / 3 : 4 + STAT(i, ST_CARDIO) / 3;
        if (P[i].souffle < 40) sp--;
        if (held(i, BUTTON_LEFT))  P[i].cur -= sp;
        if (held(i, BUTTON_RIGHT)) P[i].cur += sp;
        if (P[i].cur < 0) P[i].cur = 0;
        if (P[i].cur > maxPos) P[i].cur = maxPos;
        if ((frameCount % 10) == 0) spend(i, 1);
    }
    a0 = P[subWho].cur; a1 = a0 + lenA * 16;
    d0 = P[d].cur;      d1 = d0 + lenD * 16;
    ov = ((a1 < d1) ? a1 : d1) - ((a0 > d0) ? a0 : d0);
    if (ov * 2 >= lenD * 16) tug += (P[subWho].def->special == subM->family) ? 3 : 2;
    else tug -= 2;
    if ((frameCount & 1) == 0)
        hudSubGauge(subM->name, gate, P[subWho].cur / 16, lenA, P[d].cur / 16, lenD, tug, BAR_CELLS);

    if (tug >= 100)
    {
        gate++;
        sfx(1000 + gate * 200, 8);
        crowdReact(FALSE);
        stageCheer(80);
        if (gate >= 3)
        {
            char b[32];
            b[0] = 0;
            scat(b, "SOUMISSION : ", 31);
            scat(b, subM->name, 31);
            hudSubClear();
            phase = PH_TAP;
            phaseTimer = 110;
            actorsPairStart(subM->seq + 2, anchor, flip, top, FALSE);
            resWinner = subWho;
            scopy(resMethod, b, 31);
            fxBanner(BAN_FINALIZOU, 110);
            return;
        }
        tug = 0;
        subLayout();
    }
    else if (tug <= -100)
    {
        if (gate >= 1) { P[subWho].adv++; hudMessageWho(d, "S'ECHAPPE ! (AVANTAGE)", 0); popAt(subWho, POP_AV); }
        else hudMessageWho(d, "S'ECHAPPE !", 0);
        spend(subWho, 20);
        sfx(160, 10);
        hudSubClear();
        groundCd[subWho] = 120;
        groundCd[d] = 10;
        playSeq(subM->seq, top, TRUE, NX_ESCAPE);
    }
}

/* ==================================================================== boucle */
void match_enter(void)
{
    u16 i;
    gameMode = MODE_MATCH;
    VDP_clearPlane(BG_A, TRUE);
    stageLoad(stageIdx);
    applyFighterPalettes();
    actorsInit();
    fxInit();
    for (i = 0; i < 2; i++)
    {
        P[i].souffle = P[i].cardio = 100;
        P[i].points = P[i].adv = P[i].pen = 0;
        P[i].attempts = 0;
        P[i].idle = 0;
        P[i].aiDenyAt = 0;
        P[i].aiMoveDir = 0;
        P[i].act = ACT_IDLE;
        P[i].grip = 0;
        P[i].pad = P[i].prev = 0xFFFF;
        P[i].animRev = FALSE;
    }
    P[0].x = WORLD_W / 2 - START_GAP / 2;
    P[1].x = WORLD_W / 2 + START_GAP / 2;
    P[0].dir = 1;
    P[1].dir = -1;
    matchFrames = (gameType == GAME_DEMO) ? 60L * 60 : (s32)matchMinutes * 60 * 60;
    resWinner = -1;
    resBySub = FALSE;
    hudInit();
    hudDraw(matchFrames, TRUE);
    crowdStart();
    refInit(WORLD_W / 2 + 90);
    /* salut d'avant combat */
    top = 1;
    flip = FALSE;
    anchor = WORLD_W / 2;
    actorsPairStart(SEQ_SHAKE, anchor, flip, 1, FALSE);
    phase = PH_ANIM;
    next = NX_FIGHT;
    fxBanner(BAN_OSS, 80);
    stageCheer(120);
    if (gameType == GAME_DEMO) wtxtC("DEMO - APPUIE SUR START", 27);
    actorsDraw();
    SPR_update();
    fadeInScreen(16);
}

void matchLeave(void)
{
    fxRelease();
    actorsRelease();
    crowdStop();
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_B, 0);
}

void match_update(void)
{
    u16 i;
    for (i = 0; i < 2; i++)
    {
        if (!P[i].human && P[i].aiWant)
        {
            if (P[i].prev & P[i].aiWant & BTN_ACTS) continue;
            P[i].pad |= P[i].aiWant;
            P[i].aiWant = 0;
        }
    }

    switch (phase)
    {
        case PH_STAND:
            updateStand();
            break;
        case PH_ANIM:
            if (actorsPairUpdate()) seqDone();
            stageSetCamera(anchor);
            break;
        case PH_GROUND:
            actorsPairUpdate();
            updateGround();
            stageSetCamera(anchor);
            break;
        case PH_SUB:
            actorsPairUpdate();
            updateSub();
            break;
        case PH_TAP:
            actorsPairUpdate();
            if ((phaseTimer & 15) == 0) sfxNoise(4);
            if (--phaseTimer <= 0)
            {
                phase = PH_ANIM;     /* evite le double appel */
                endMatch(resWinner, resMethod, TRUE, BAN_FINALIZOU);
            }
            break;
        case PH_DECIDE:
            actorsPairUpdate();
            actorsSoloUpdate();
            if (--phaseTimer <= 0) startOutro();
            break;
        case PH_OVER:
            actorsSoloUpdate();
            if (--phaseTimer <= 0) { matchLeave(); result_enter(); return; }
            break;
    }

    if (phase == PH_STAND || phase == PH_GROUND || phase == PH_SUB || phase == PH_ANIM)
    {
        if (phase != PH_ANIM || next != NX_FIGHT)
        {
            if (--matchFrames <= 0) { matchFrames = 0; timeUp(); }
        }
    }
    hudDraw(matchFrames, FALSE);
    hudUpdate();
    crowdUpdate(pend || phase == PH_SUB);
    stageUpdate();
    fxUpdate();
    actorsDraw();
}
