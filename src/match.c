/*
 * BJJ TATAMI - le combat, version "arcade reactive".
 *
 * Principe : chaque attaque SE VOIT VENIR (le combattant clignote, une icone de bouton apparait
 * au-dessus du defenseur). Le defenseur contre avec le bon bouton pendant la preparation ;
 * au dernier moment c'est un CONTRE PARFAIT (arret sur image, eclair, "CONTRA!", ouverture).
 *
 * DEBOUT  : A = double-leg, BAS+A = single-leg, HAUT+A = judo (il faut des prises : B tenu),
 *           BAS+B = tirer la garde.  Defense : C contre les amenees au sol, HAUT+C contre le judo.
 * AU SOL  : A + direction = technique, B + direction = soumission ; pendant la preparation,
 *           rappuyer A/B dans une autre direction = FEINTE.  Defense : C + la fleche affichee.
 * SOUMISSION : bras de fer - l'attaquant martele A, le defenseur martele C ; une fleche
 *           apparait parfois au-dessus d'un combattant : l'appuyer vite = gros bonus.
 * SUPER   : la jauge (sous les points) se remplit avec les techniques et les contres ;
 *           pleine, B+C ensemble = technique signature (cinematique + enchainement automatique).
 * PENALITES : passivite et sortie de zone ; 2e = avantage adverse, 3e = 2 points, 4e = disqualification.
 */
#include "game.h"

/* ------------------------------------------------------------------ reglages */
#define MIN_GAP         34
#define REACH_GRIP      58
#define REACH_SHOOT     72
#define GAP_GRIP        46
#define START_GAP       104
#define STALL_FRAMES    1500
#define PERFECT_WIN     7           /* dernieres images de la preparation = contre parfait */
#define SUB_MAX_FRAMES  660

enum { POS_GUARD, POS_HALF, POS_SIDE, POS_KNEE, POS_MOUNT, POS_BACK, POS_COUNT };
enum { PH_INTRO, PH_STAND, PH_ANIM, PH_GROUND, PH_SUB, PH_TAP, PH_DECIDE, PH_OVER, PH_SUPER };
enum { ACT_IDLE, ACT_WIND, ACT_SHOOT, ACT_DEFEND, ACT_STUMBLE };
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
static const char* const sigName[5] = { "CLE DE BRAS ECLAIR", "TRIANGLE VOLANT", "PIEGE KIMURA",
                                        "ETRANGLEMENT FATAL", "BERIMBOLO CHEVILLE" };
static const char dirChar[3] = { '^', '>', 'v' };

/* IA : facile / normal / difficile */
static const u16 cpuCounter[3] = { 28, 48, 68 };   /* % de reaction a une attaque debout */
static const u16 cpuDeny[3]    = { 22, 42, 62 };   /* % de reaction au sol */
static const u16 cpuRead[3]    = { 65, 82, 94 };   /* % de bonne lecture quand elle reagit */
static const u16 cpuMash[3]    = { 11, 8, 6 };     /* images entre deux appuis (soumission) */
static const u16 cpuDodge[3]   = { 35, 55, 75 };   /* % de reussite sur les fleches */
static const u16 cpuFeint[3]   = { 0, 12, 28 };    /* % de feinte au sol */

/* ------------------------------------------------------------------ etat */
static u16 phase;
static s32 matchFrames;
static s16 phaseTimer;
static u16 pos, top;
static bool flip;
static s16 anchor;
static u16 next, nextPos;
static s16 freeze;                  /* arret sur image */

/* attaque au sol */
static bool pend, pendSure, pendDenyUsed, pendFeinted;
static u16 pendWho, pendKind, pendDir;
static s16 pendTimer, pendMax;
static s16 groundCd[2];
static s16 feintAt;
static u16 feintBtn;

/* soumission */
static u16 subWho;
static const SubMove* subM;
static s16 tug, subTime, subPeak, subBonus;
static s16 prWho, prDir, prTimer, prNext;

/* super technique */
static u16 superWho, superFrom;
static bool comboOn;
static u16 comboWho, comboSteps;
static s16 comboDelay;

#define STAT(i, s)  ((s16)P[i].def->stat[s])
#define OPP(i)      (1 - (i))

static const SubMove* subFor(u16 who, u16 d);
static const Move* moveFor(u16 who, u16 d);
static void startSub(u16 who, const SubMove* s, s16 startTug);

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

static void gainSuper(u16 i, s16 n)
{
    bool was = P[i].superG >= 100;
    P[i].superG += n;
    if (P[i].superG > 100) P[i].superG = 100;
    if (!was && P[i].superG >= 100)
    {
        hudMessageWho(i, "SUPER PRETE : B+C !", 0);
        sfx(1568, 12);
    }
}

static u16 dirOf(u16 i)
{
    u16 p = P[i].pad;
    if (p & BUTTON_DOWN) return DIR_DOWN;
    if (p & (BUTTON_LEFT | BUTTON_RIGHT)) return DIR_SIDE;
    return DIR_UP;
}

static u16 dirBtn(u16 d) { return d == DIR_DOWN ? BUTTON_DOWN : (d == DIR_SIDE ? BUTTON_RIGHT : BUTTON_UP); }

static s16 dist(void) { s16 d = P[1].x - P[0].x; return d < 0 ? -d : d; }

static void headPos(u16 i, s16* x, s16* y)
{
    if (phase == PH_STAND || phase == PH_INTRO || phase == PH_OVER || (phase == PH_SUPER && superFrom == PH_STAND))
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
        voiceSay(pts == 2 ? VO_P2 : (pts == 3 ? VO_P3 : VO_P4));
        crowdReact(FALSE);
        stageCheer(120);
        gainSuper(who, pts * 4);
        gainSuper(OPP(who), 6);
    }
    else if (adv)
    {
        s16 x, y;
        headPos(who, &x, &y);
        popAt(who, POP_AV);
        refSignal(REF_ADV, x, 60);
        voiceSay(VO_VANTAGEM);
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
    voiceSay(VO_PUNICAO);
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

static bool superPressed(u16 i)
{
    if (P[i].superG < 100) return FALSE;
    return (pressed(i, BUTTON_B) && held(i, BUTTON_C)) || (pressed(i, BUTTON_C) && held(i, BUTTON_B));
}

/* ------------------------------------------------------------------ aides */
static void standHints(void)
{
    static const char* const h[3] = {
        "@A DOUBLE-LEG ^@A JUDO  v@A SINGLE-LEG",
        "@B TENIR = PRISES    v@B TIRER LA GARDE",
        "@C CONTRER L'ICONE   @B+@C SUPER (PLEINE)" };
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

static void subHints(void)
{
    static const char* const att[2] = { "MARTELE @A POUR SERRER !",
                                         "FLECHE AU-DESSUS DE TOI : APPUIE VITE" };
    static const char* const def[2] = { "MARTELE @C POUR T'ECHAPPER !",
                                         "FLECHE AU-DESSUS DE TOI : APPUIE VITE" };
    if (!helpOn || !P[0].human || numHumans != 1) { hudClearHints(); return; }
    hudHints(subWho == 0 ? att : def, 2);
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
    comboOn = FALSE;
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
        P[i].animHold = FALSE;
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
        if (groundCd[i] < 24) groundCd[i] = 24;
        P[i].aiWant = 0;
        P[i].aiDenyAt = 0;
        P[i].aiTimer = 40 + (random() % 50);
        P[i].grip = 0;
    }
    feintAt = 0;
    if (comboOn) comboDelay = 16;
    groundHints();
    refFollow(anchor);
}

static void standFromPair(void)
{
    s16 x, y;
    u16 roleA = top;
    actorsPairWorldPos(0, &x, &y);
    P[roleA].x = x;
    actorsPairWorldPos(1, &x, &y);
    P[OPP(roleA)].x = x;
}

static void soloFor(u16 i, u16 anim, bool rev)
{
    P[i].animRev = rev;
    actorsSoloSet(i, anim, FALSE);
}

static void stumble(u16 i, s16 frames, const char* msg)
{
    P[i].act = ACT_STUMBLE;
    P[i].stun = frames;
    P[i].animRev = FALSE;
    P[i].animHold = FALSE;
    actorsSoloSet(i, SOLO_STUMBLE, TRUE);
    if (msg) hudMessageWho(i, msg, 0);
}

static void enterStand(void)
{
    u16 i;
    s16 cx = (P[0].x + P[1].x) / 2;
    phase = PH_STAND;
    pend = FALSE;
    comboOn = FALSE;
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
        P[i].cooldown = 20;
        P[i].grip = 0;
        P[i].aiWant = 0;
        P[i].aiDenyAt = 0;
        P[i].aiTimer = 30;
        P[i].animRev = FALSE;
        P[i].animHold = FALSE;
        actorsSoloSet(i, SOLO_IDLE, TRUE);
        if (P[i].stunNext > 0)
        {
            stumble(i, P[i].stunNext, NULL);
            actorsSoloSet(i, SOLO_TIRED, TRUE);
            P[i].stunNext = 0;
        }
    }
    P[0].dir = (P[0].x < P[1].x) ? 1 : -1;
    P[1].dir = -P[0].dir;
    standHints();
    clearRow(6);
}

static void comboStep(void);

static void seqDone(void)
{
    const PairSeq* s = &pairSeqs[actorsPairSeq()];
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
            subHints();
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

static s16 windupFor(u16 i, u16 type)
{
    s16 w = (type == TD_JUDO) ? 26 : (type == TD_DOUBLE ? 22 : 20);
    w -= (STAT(i, ST_LUTTE) - 5) / 2;
    if (!P[i].human) w -= difficulty * 2;
    if (P[i].souffle < 30) w += 5;
    if (w < 10) w = 10;
    return w;
}

/* l'IA decide si (et quand) elle contre une attaque debout */
static void aiPlanCounter(u16 d)
{
    Player* me = &P[d];
    Player* op = &P[OPP(d)];
    u16 ch;
    me->aiDenyAt = 0;
    if (op->sure || me->act != ACT_IDLE) return;
    ch = cpuCounter[difficulty] + me->def->stat[ST_LUTTE] * 2;
    if ((random() % 100) < ch)
    {
        bool read = (random() % 100) < cpuRead[difficulty];
        u16 kind = (op->tdType == TD_JUDO) ? DEF_HIP : DEF_SPRAWL;
        s16 span = op->windMax - 5;
        if (!read) kind = 1 - kind;
        if (span < 1) span = 1;
        me->aiDenyAt = 4 + (random() % span);
        me->aiDenyBtn = BUTTON_C | (kind == DEF_HIP ? BUTTON_UP : 0);
    }
}

static void startAttack(u16 i, u16 type)
{
    Player* f = &P[i];
    if (type == TD_JUDO && (f->grip < 40 || dist() > REACH_GRIP))
    {
        hudMessageWho(i, "JUDO : IL FAUT DES PRISES (B)", 0);
        f->cooldown = 20;
        return;
    }
    f->tdType = type;
    f->act = ACT_WIND;
    f->sure = f->opening > 0;
    f->windMax = f->sure ? 5 : windupFor(i, type);
    f->wind = f->windMax;
    f->opening = 0;
    f->animRev = FALSE;
    actorsSoloSet(i, type == TD_JUDO ? SOLO_GRIP_HOLD : SOLO_SHOOT, TRUE);
    f->animHold = TRUE;
    popAt(i, POP_DANGER);
    spend(i, 8);
    acted(i);
    sfx(type == TD_JUDO ? 330 : 220, 6);
    if (f->sure) hudMessageWho(i, "OUVERTURE ! IMPARABLE", 0);
    if (!P[OPP(i)].human) aiPlanCounter(OPP(i));
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

static void counterStand(u16 d, bool perfect)
{
    u16 a = OPP(d);
    u16 type = P[a].tdType;
    u16 seq = (type == TD_DOUBLE) ? SEQ_TD_DOUBLE_SPRAWL : (type == TD_SINGLE ? SEQ_TD_SINGLE_SPRAWL : SEQ_TD_JUDO_BLOCK);
    P[a].act = ACT_IDLE;
    P[a].animHold = FALSE;
    P[d].act = ACT_IDLE;
    P[d].aiDenyAt = 0;
    spend(a, perfect ? 18 : 12);
    P[a].stunNext = perfect ? 70 : 40;
    P[d].opening = perfect ? 150 : 80;
    P[d].idle = 0;
    gainSuper(d, perfect ? 40 : 22);
    gainSuper(a, 5);
    if (perfect)
    {
        award(d, 0, 1);
        fxBanner(BAN_CONTRA, 70);
        hudMessageWho(d, "CONTRE PARFAIT ! OUVERTURE", 0);
        freeze = 16;
        gfxFlash(6);
        stageShake(10);
    }
    else
    {
        popAt(d, POP_BLOC);
        hudMessageWho(d, type == TD_JUDO ? "HANCHE BLOQUEE ! CONTRE" : "SPRAWL ! CONTRE", 0);
        freeze = 8;
    }
    crowdReact(perfect);
    stageCheer(perfect ? 160 : 70);
    sfx(perfect ? 1200 : 900, 8);
    setupStandingPair(a, seq, FALSE);
    playSeq(seq, a, FALSE, NX_STAND);
}

static void landTakedown(u16 a)
{
    u16 type = P[a].tdType;
    u16 seq = (type == TD_DOUBLE) ? SEQ_TD_DOUBLE : (type == TD_SINGLE ? SEQ_TD_SINGLE : SEQ_TD_JUDO);
    P[a].act = ACT_IDLE;
    P[a].animHold = FALSE;
    setupStandingPair(a, seq, type != TD_JUDO);
    hudMessageWho(a, tdName[type], 2);
    award(a, 2, 0);
    gainSuper(a, 6);
    nextPos = (type == TD_DOUBLE) ? POS_GUARD : (type == TD_SINGLE ? POS_HALF : POS_SIDE);
    playSeq(seq, a, FALSE, NX_GROUND);
}

static void tryPullGuard(u16 i)
{
    if (dist() > REACH_GRIP) return;
    if (P[i].grip < 25) { hudMessageWho(i, "IL FAUT DES PRISES (B)", 0); P[i].cooldown = 15; return; }
    acted(i);
    hudMessageWho(i, "TIRE LA GARDE", 0);
    setupStandingPair(i, SEQ_PULL_GUARD, FALSE);
    nextPos = POS_GUARD;
    playSeq(SEQ_PULL_GUARD, i, FALSE, NX_GROUND);
}

static void startSuper(u16 i);

/* ---------------- IA debout */
static void aiStand(u16 i)
{
    Player* me = &P[i];
    Player* op = &P[OPP(i)];
    s16 d = dist();
    s16 r;
    u16 toward = (op->x > me->x) ? BUTTON_RIGHT : BUTTON_LEFT;
    u16 away = (op->x > me->x) ? BUTTON_LEFT : BUTTON_RIGHT;

    if (me->aiDenyAt > 0)
    {
        if (op->act != ACT_WIND) me->aiDenyAt = 0;
        else if (--me->aiDenyAt == 0) me->aiWant = me->aiDenyBtn;
    }
    if (me->aiWant) return;
    if (me->act != ACT_IDLE || me->cooldown > 0) return;

    if (me->superG >= 100 && op->act != ACT_WIND && (random() % 100) < 2) { me->aiWant = BUTTON_B | BUTTON_C; return; }
    if (me->opening > 0 && d <= REACH_SHOOT - 4) { me->aiWant = BUTTON_A; return; }

    if (--me->aiTimer <= 0)
    {
        me->aiTimer = 18 + (random() % 30) - difficulty * 5;
        r = random() % 100;
        me->aiMoveDir = 0;
        if (op->act == ACT_STUMBLE && d <= REACH_SHOOT) { me->aiWant = BUTTON_A; return; }
        if (d <= REACH_SHOOT - 6)
        {
            if (me->grip >= 45 && d <= REACH_GRIP && me->def->stat[ST_LUTTE] >= 6 && r < 30) { me->aiWant = BUTTON_A | BUTTON_UP; return; }
            if (me->grip >= 25 && d <= REACH_GRIP && me->def->stat[ST_GARDE] >= 7 && r >= 88) { me->aiWant = BUTTON_B | BUTTON_DOWN; return; }
            if (r < 22 + difficulty * 8) { me->aiWant = BUTTON_A | ((r & 1) ? BUTTON_DOWN : 0); return; }
            me->aiMoveDir = (r > 80) ? -1 : 0;
        }
        else me->aiMoveDir = (r < 85) ? 1 : 0;
    }
    if (me->aiMoveDir > 0) me->pad |= toward;
    else if (me->aiMoveDir < 0) me->pad |= away;
    if (!(me->pad & BUTTON_C) && d <= REACH_GRIP && (frameCount & 63) < (u16)(30 + difficulty * 10)) me->pad |= BUTTON_B;
}

static bool standAction(u16 i)
{
    Player* f = &P[i];
    switch (f->act)
    {
        case ACT_WIND:
            if (--f->wind <= 0)
            {
                f->animHold = FALSE;
                if (f->tdType == TD_JUDO) landTakedown(i);
                else f->act = ACT_SHOOT;
            }
            return TRUE;
        case ACT_SHOOT:
            if (actorsSoloDone(i))
            {
                f->act = ACT_IDLE;
                if (dist() > REACH_SHOOT) stumble(i, 40, "TROP LOIN !");
                else landTakedown(i);
            }
            return TRUE;
        case ACT_DEFEND:
            if (--f->defTimer <= 0) { f->act = ACT_IDLE; f->cooldown = 14; soloFor(i, SOLO_IDLE, FALSE); }
            return TRUE;
        case ACT_STUMBLE:
            if (f->anim == SOLO_STUMBLE && actorsSoloDone(i)) soloFor(i, SOLO_TIRED, FALSE);
            if (--f->stun <= 0) { f->act = ACT_IDLE; f->cooldown = 6; soloFor(i, SOLO_IDLE, FALSE); }
            return TRUE;
    }
    if (f->cooldown > 0) f->cooldown--;
    return FALSE;
}

static void standInput(u16 i)
{
    Player* f = &P[i];
    Player* op = &P[OPP(i)];
    bool moving = FALSE, gripping = FALSE;
    s16 sp = (f->souffle < 25 && (frameCount & 1)) ? 0 : 1;

    if (superPressed(i)) { startSuper(i); return; }

    /* defense : toujours disponible quand on est libre (pas de temps de recharge) */
    if (pressed(i, BUTTON_C))
    {
        u16 kind = held(i, BUTTON_UP) ? DEF_HIP : DEF_SPRAWL;
        if (op->act == ACT_WIND && !op->sure)
        {
            u16 need = (op->tdType == TD_JUDO) ? DEF_HIP : DEF_SPRAWL;
            if (kind == need) { counterStand(i, op->wind <= PERFECT_WIN); return; }
            hudMessageWho(i, "MAUVAISE DEFENSE !", 0);
        }
        startDefend(i, kind);
        return;
    }
    if (f->cooldown <= 0)
    {
        if (pressed(i, BUTTON_A))
        {
            u16 t = TD_DOUBLE;
            if (held(i, BUTTON_UP)) t = TD_JUDO;
            else if (held(i, BUTTON_DOWN)) t = TD_SINGLE;
            startAttack(i, t);
            return;
        }
        if (pressed(i, BUTTON_B) && held(i, BUTTON_DOWN)) { tryPullGuard(i); if (phase != PH_STAND) return; }
    }
    if (held(i, BUTTON_LEFT))  { moveFighter(i, -sp); moving = TRUE; }
    if (held(i, BUTTON_RIGHT)) { moveFighter(i, sp); moving = TRUE; }

    if (held(i, BUTTON_B) && !held(i, BUTTON_DOWN) && dist() <= REACH_GRIP)
    {
        gripping = TRUE;
        if (f->grip < 100) { f->grip += 2; if (f->grip > 100) f->grip = 100; }
        if (op->grip > 0) op->grip--;
        if ((frameCount % 20) == 0) spend(i, 1);
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
        if (P[i].opening > 0) P[i].opening--;
        stallCheck(i);
    }
    actorsSoloUpdate();
    stageSetCamera((P[0].x + P[1].x) / 2);
    refFollow((P[0].x + P[1].x) / 2);
}

/* ==================================================================== SOL */
static void planDeny(void)
{
    u16 d = OPP(pendWho);
    P[d].aiDenyAt = 0;
    if (pendSure || P[d].human) return;
    {
        u16 stat = (pendWho == top) ? P[d].def->stat[ST_GARDE] : P[d].def->stat[ST_PASSAGE];
        if ((random() % 100) < (u16)(cpuDeny[difficulty] + stat * 2))
        {
            u16 guess = pendDir;
            s16 span = pendMax - 5;
            if ((random() % 100) >= cpuRead[difficulty]) guess = (pendDir + 1 + (random() & 1)) % 3;
            if (span < 1) span = 1;
            P[d].aiDenyAt = 4 + (random() % span);
            P[d].aiDenyBtn = BUTTON_C | dirBtn(guess);
        }
    }
}

static void startPend(u16 who, u16 kind, u16 dir, bool feint)
{
    s16 st = (kind == PEND_SUB) ? STAT(who, ST_SOUM) : (who == top ? STAT(who, ST_PASSAGE) : STAT(who, ST_GARDE));
    s16 diff = (kind == PEND_SUB) ? subFor(who, dir)->diff : moveFor(who, dir)->diff;
    pend = TRUE;
    pendWho = who;
    pendKind = kind;
    pendDir = dir;
    if (!feint)
    {
        pendSure = P[who].opening > 0;
        P[who].opening = 0;
        pendDenyUsed = FALSE;
        pendFeinted = FALSE;
    }
    else pendFeinted = TRUE;
    if (pendSure) pendMax = 5;
    else if (feint) pendMax = 12;
    else
    {
        pendMax = ((kind == PEND_SUB) ? 26 : 28) - (st - 5) + diff * 2;
        if (!P[who].human) pendMax -= difficulty * 2;
        if (P[who].souffle < 30) pendMax += 5;
        if (pendMax < 11) pendMax = 11;
        if (pendMax > 38) pendMax = 38;
    }
    pendTimer = pendMax;
    spend(who, feint ? 6 : ((kind == PEND_SUB) ? 8 : 6));
    acted(who);
    popAt(who, POP_DANGER);
    sfx(kind == PEND_SUB ? 700 : 440, 6);
    if (pendSure) hudMessageWho(who, "OUVERTURE ! IMPARABLE", 0);
    else if (feint) hudMessageWho(who, "FEINTE !", 0);
    planDeny();
    /* l'IA attaquante peut feinter */
    feintAt = 0;
    if (!feint && !pendSure && !P[who].human && (random() % 100) < cpuFeint[difficulty])
    {
        u16 k;
        for (k = 1; k < 3; k++)
        {
            u16 nd = (dir + k) % 3;
            if (kind == PEND_SUB ? (subFor(who, nd) != NULL) : (moveFor(who, nd) != NULL))
            {
                feintAt = 6 + (random() % 8);
                if (feintAt >= pendMax - 2) feintAt = pendMax - 3;
                feintBtn = (kind == PEND_SUB ? BUTTON_B : BUTTON_A) | dirBtn(nd);
                break;
            }
        }
    }
}

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
    gainSuper(who, 8);
    playSeq(m->seq, top, FALSE, NX_GROUND);
}

static void resolvePend(void)
{
    u16 who = pendWho, d = OPP(who);
    pend = FALSE;
    groundCd[who] = 36;
    groundCd[d] = 12;
    if (pendKind == PEND_SUB) startSub(who, subFor(who, pendDir), 0);
    else applyMove(who, moveFor(who, pendDir));
}

static void blocked(u16 d, bool perfect)
{
    u16 who = pendWho;
    pend = FALSE;
    feintAt = 0;
    spend(who, perfect ? 16 : 10);
    groundCd[who] = perfect ? 70 : 45;
    groundCd[d] = 4;
    P[d].idle = 0;
    gainSuper(d, perfect ? 38 : 20);
    gainSuper(who, 4);
    if (perfect)
    {
        P[d].opening = 120;
        award(d, 0, 1);
        fxBanner(BAN_CONTRA, 70);
        hudMessageWho(d, "CONTRE PARFAIT ! OUVERTURE", 0);
        freeze = 14;
        gfxFlash(5);
    }
    else
    {
        popAt(d, POP_BLOC);
        hudMessageWho(d, "BLOQUE !", 0);
        freeze = 6;
    }
    sfx(perfect ? 1200 : 180, 10);
    actorsShake(14);
    crowdReact(perfect);
    stageCheer(perfect ? 140 : 50);
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
    if (me->superG >= 100 && (random() % 100) < 3) { me->aiWant = BUTTON_B | BUTTON_C; return; }
    if (me->opening > 0) me->aiTimer = 0;
    if (--me->aiTimer > 0) return;
    me->aiTimer = 100 - difficulty * 25 + (random() % 60);
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
    me->aiWant = ((bestKind == PEND_SUB) ? BUTTON_B : BUTTON_A) | dirBtn(bestDir);
}

static void updateGround(void)
{
    u16 i;
    if (comboOn)
    {
        if (--comboDelay <= 0) comboStep();
        return;
    }
    for (i = 0; i < 2; i++) if (!P[i].human) aiGround(i);
    if (pend)
    {
        u16 w = pendWho, d = OPP(w);
        /* feinte : l'attaquant change de direction pendant la preparation */
        if (!pendFeinted && !pendSure)
        {
            if (feintAt > 0 && --feintAt == 0) P[w].aiWant = feintBtn;
            if ((pressed(w, BUTTON_A) || pressed(w, BUTTON_B)) && dirOf(w) != pendDir)
            {
                u16 kind = pressed(w, BUTTON_B) ? PEND_SUB : PEND_MOVE;
                u16 nd = dirOf(w);
                if (kind == PEND_SUB ? (subFor(w, nd) != NULL) : (moveFor(w, nd) != NULL))
                {
                    startPend(w, kind, nd, TRUE);
                    return;
                }
            }
        }
        if (!pendSure && !pendDenyUsed && pressed(d, BUTTON_C))
        {
            pendDenyUsed = TRUE;
            if (dirOf(d) == pendDir) { blocked(d, pendTimer <= PERFECT_WIN); return; }
            hudMessageWho(d, "RATE !", 0);
            if (pendTimer > 6) pendTimer = 6;
        }
        if (--pendTimer <= 0) resolvePend();
        return;
    }
    for (i = 0; i < 2; i++)
    {
        if (superPressed(i)) { startSuper(i); return; }
        if (P[i].opening > 0) P[i].opening--;
        if (groundCd[i] > 0) { groundCd[i]--; continue; }
        if (pressed(i, BUTTON_A) && moveFor(i, dirOf(i))) { startPend(i, PEND_MOVE, dirOf(i), FALSE); return; }
        if (pressed(i, BUTTON_B) && !held(i, BUTTON_C) && subFor(i, dirOf(i))) { startPend(i, PEND_SUB, dirOf(i), FALSE); return; }
    }
    regen(top, 5);
    regen(OPP(top), (pos >= POS_SIDE) ? 14 : 7);
    stallCheck(0);
    stallCheck(1);
}

/* ==================================================================== SOUMISSION (bras de fer) */
static void startSub(u16 who, const SubMove* s, s16 startTug)
{
    subWho = who;
    subM = s;
    tug = startTug;
    subPeak = startTug;
    subTime = 0;
    subBonus = startTug > 0 ? 2 : 0;
    prWho = -1;
    prNext = 40 + (random() % 30);
    P[0].aiTimer = P[1].aiTimer = 10;
    P[0].aiDenyAt = P[1].aiDenyAt = 0;
    hudClearHints();
    clearRow(6);
    hudMessageWho(who, s->name, 0);
    sfx(880, 10);
    crowdReact(FALSE);
    stageCheer(100);
    playSeq(s->seq, top, FALSE, NX_SUB);
}

static void endPrompt(void)
{
    prWho = -1;
    prNext = 45 + (random() % 35);
}

static void updateSub(void)
{
    u16 a = subWho, d = OPP(a), i;
    s16 pa, pd;
    subTime++;

    /* l'IA martele */
    for (i = 0; i < 2; i++)
    {
        if (P[i].human) continue;
        if (P[i].aiDenyAt > 0 && --P[i].aiDenyAt == 0) P[i].aiWant = P[i].aiDenyBtn;
        if (--P[i].aiTimer <= 0)
        {
            P[i].aiTimer = cpuMash[difficulty] + (random() % 3);
            if (!P[i].aiWant) P[i].aiWant = (i == a) ? BUTTON_A : BUTTON_C;
        }
    }

    pa = 4 + STAT(a, ST_SOUM) / 2 + (P[a].def->special == subM->family ? 2 : 0) - subM->diff + subBonus
         - (P[a].souffle < 30 ? 2 : 0);
    pd = 4 + (STAT(d, ST_CARDIO) + STAT(d, ST_SOUM)) / 4 - (P[d].souffle < 30 ? 2 : 0);
    if (pa < 2) pa = 2;
    if (pd < 2) pd = 2;
    if (pressed(a, BUTTON_A)) { tug += pa; if ((subTime & 3) == 0) spend(a, 1); }
    if (pressed(d, BUTTON_C)) { tug -= pd; if ((subTime & 3) == 0) spend(d, 1); }
    if ((subTime & 1) == 0) tug--;

    /* fleches : bonus de reflexe */
    if (prWho < 0)
    {
        if (--prNext <= 0)
        {
            prWho = (random() & 1) ? a : d;
            prDir = random() % 4;
            prTimer = 28;
            if (!P[prWho].human)
            {
                static const u16 arrowBtn[4] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT };
                bool ok = (random() % 100) < cpuDodge[difficulty];
                P[prWho].aiDenyAt = 7 + (random() % 12);
                P[prWho].aiDenyBtn = arrowBtn[ok ? prDir : (prDir + 1) % 4];
            }
            sfx(1046, 4);
        }
    }
    else
    {
        static const u16 arrowBtn[4] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT };
        u16 w = prWho, k;
        s16 hit = -1;
        for (k = 0; k < 4; k++) if (pressed(w, arrowBtn[k])) hit = k;
        if (hit >= 0)
        {
            bool good = (hit == prDir);
            s16 amount = good ? 24 : -8;
            tug += (w == a) ? amount : -amount;
            if (good)
            {
                sfx(1568, 6);
                actorsShake(8);
                crowdReact(FALSE);
                hudMessageWho(w, w == a ? "SERRE !" : "ESQUIVE !", 0);
            }
            else sfx(150, 6);
            endPrompt();
        }
        else if (--prTimer <= 0) endPrompt();
    }

    if (tug > subPeak) subPeak = tug;
    if ((frameCount & 1) == 0) hudSubGauge(subM->name, tug);

    if (tug >= 100)
    {
        char b[32];
        b[0] = 0;
        scat(b, "SOUMISSION : ", 31);
        scat(b, subM->name, 31);
        prWho = -1;
        hudSubClear();
        hudClearHints();
        phase = PH_TAP;
        phaseTimer = 110;
        freeze = 18;
        gfxFlash(6);
        stageShake(12);
        actorsPairStart(subM->seq + 2, anchor, flip, top, FALSE);
        resWinner = a;
        scopy(resMethod, b, 31);
        fxBanner(BAN_FINALIZOU, 110);
        return;
    }
    if (tug <= -100 || subTime > SUB_MAX_FRAMES)
    {
        prWho = -1;
        if (subPeak >= 60) { P[a].adv++; hudMessageWho(d, "S'ECHAPPE ! (AVANTAGE)", 0); popAt(a, POP_AV); }
        else hudMessageWho(d, "S'ECHAPPE !", 0);
        spend(a, 20);
        gainSuper(d, 25);
        sfx(160, 10);
        hudSubClear();
        groundCd[a] = 110;
        groundCd[d] = 10;
        freeze = 6;
        playSeq(subM->seq, top, TRUE, NX_ESCAPE);
    }
}

/* ==================================================================== SUPER TECHNIQUE */
static void startSuper(u16 i)
{
    u16 k;
    superWho = i;
    superFrom = phase;
    P[i].superG = 0;
    pend = FALSE;
    feintAt = 0;
    for (k = 0; k < 2; k++)
    {
        P[k].act = ACT_IDLE;
        P[k].animHold = FALSE;
        P[k].aiWant = 0;
        P[k].aiDenyAt = 0;
    }
    phase = PH_SUPER;
    phaseTimer = 110;
    hudClearHints();
    clearRow(5);
    clearRow(6);
    stageDim(TRUE);
    fighterTint(OPP(i), 4);
    fighterTint(i, 0);
    fxBanner(BAN_ESPECIAL, 100);
    crowdReact(TRUE);
    stageCheer(240);
    sfx(1400, 20);
    freeze = 4;
}

static void drawCutIn(u16 row)
{
    u16 i = superWho;
    if (row == 13) { u16 x; for (x = 0; x < 40; x++) putTile(x, 13, HT_PANEL_TOP); }
    else if (row == 23) { u16 x; for (x = 0; x < 40; x++) putTile(x, 23, HT_PANEL_BOT); }
    else fillPanel(0, row, 40, 1);
    if (row == 22)
    {
        drawPortrait(i, P[i].gid, 3, 14, PORTRAIT_TILE);
        gtxt("TECHNIQUE SPECIALE !", 14, 15);
        wtxt(sigName[P[i].def->special], 14, 17);
        gtxt(P[i].def->name, 14, 19);
        wtxt(P[i].def->nick, 14, 20);
    }
}

static void comboLaunch(void)
{
    u16 i = superWho;
    VDP_clearTextArea(0, 13, 40, 11);
    stageDim(FALSE);
    fighterTint(OPP(i), 0);
    comboOn = TRUE;
    comboWho = i;
    comboSteps = 0;
    if (superFrom == PH_STAND)
    {
        u16 d = OPP(i);
        s16 dir = (P[i].x < P[d].x) ? 1 : -1;
        P[i].x = P[d].x - dir * GAP_GRIP;
        if (P[i].x < MAT_MIN) { P[i].x = MAT_MIN; P[d].x = MAT_MIN + GAP_GRIP; }
        if (P[i].x > MAT_MAX) { P[i].x = MAT_MAX; P[d].x = MAT_MAX - GAP_GRIP; }
        P[i].tdType = TD_JUDO;
        freeze = 6;
        landTakedown(i);
    }
    else
    {
        phase = PH_GROUND;
        comboDelay = 6;
    }
}

static void updateSuper(void)
{
    u16 t = 110 - phaseTimer;
    if (t >= 8 && t <= 18) drawCutIn(13 + (t - 8));
    if (--phaseTimer <= 0) comboLaunch();
}

/* enchainement automatique de la super technique */
static void comboStep(void)
{
    u16 w = comboWho, d;
    const SubMove* best = NULL;
    const Move* bm = NULL;
    for (d = 0; d < 3; d++)
    {
        const SubMove* s = subFor(w, d);
        if (s && s->family == P[w].def->special) best = s;
    }
    if (!best && comboSteps >= 2)
        for (d = 0; d < 3; d++)
        {
            const SubMove* s = subFor(w, d);
            if (s && (!best || s->diff < best->diff)) best = s;
        }
    if (best)
    {
        comboOn = FALSE;
        hudMessage("FINITION !", 90);
        startSub(w, best, 50);
        return;
    }
    if (comboSteps < 4)
        for (d = 0; d < 3; d++)
        {
            const Move* m = moveFor(w, d);
            if (m && m->target != STANDUP && (!bm || m->points > bm->points || (m->swap && !bm->swap && w != top))) bm = m;
        }
    if (bm)
    {
        char b[24], n[3];
        comboSteps++;
        applyMove(w, bm);
        if (comboSteps >= 2)
        {
            numStr(comboSteps, 1, n);
            b[0] = 0;
            scat(b, "ENCHAINEMENT x", 23);
            scat(b, n, 23);
            scat(b, " !", 23);
            hudMessage(b, 80);
        }
        return;
    }
    for (d = 0; d < 3; d++)
    {
        const SubMove* s = subFor(w, d);
        if (s) { comboOn = FALSE; startSub(w, s, 50); return; }
    }
    comboOn = FALSE;
}

/* ==================================================================== retours visuels */
static void updateTints(void)
{
    u16 i;
    if (phase == PH_SUPER)
    {
        if (phaseTimer < 92) fighterTint(superWho, ((frameCount >> 2) & 1) ? 3 : 0);
        return;
    }
    for (i = 0; i < 2; i++)
    {
        u16 mode = 0;
        if (phase == PH_STAND && P[i].act == ACT_WIND)
            mode = P[i].sure ? 3 : (((P[i].wind >> 1) & 1) ? 1 : 0);
        else if (phase == PH_GROUND && pend && pendWho == i)
            mode = pendSure ? 3 : (((pendTimer >> 1) & 1) ? 1 : 0);
        else if ((phase == PH_STAND || phase == PH_GROUND) && P[i].opening > 0)
            mode = ((frameCount >> 3) & 1) ? 3 : 0;
        else if (phase == PH_SUB && prWho == (s16)i && ((frameCount >> 2) & 1))
            mode = 1;
        fighterTint(i, mode);
    }
}

static void updatePrompts(void)
{
    u16 i;
    s16 x, y;
    for (i = 0; i < 2; i++)
    {
        s16 pr = -1;
        if (P[i].human && freeze == 0)
        {
            u16 o = OPP(i);
            if (phase == PH_STAND && P[o].act == ACT_WIND && !P[o].sure && P[i].act == ACT_IDLE)
                pr = (P[o].tdType == TD_JUDO) ? PR_C_UP : PR_C;
            else if (phase == PH_GROUND && pend && pendWho == o && !pendSure && !pendDenyUsed)
                pr = (pendDir == DIR_UP) ? PR_C_UP : (pendDir == DIR_SIDE ? PR_C_SIDE : PR_C_DOWN);
            else if (phase == PH_SUB)
            {
                if (prWho == (s16)i) pr = PR_UP + prDir;
                else if ((subTime >> 4) & 1) pr = (i == subWho) ? PR_A_MASH : PR_C_MASH;
            }
        }
        if (pr >= 0)
        {
            headPos(i, &x, &y);
            if (phase == PH_STAND) y = FLOOR_SCREEN - 118;
            else y -= 6;
            fxPrompt(i, pr, x, y);
        }
        else fxPrompt(i, -1, 0, 0);
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
        P[i].animHold = FALSE;
        P[i].superG = 0;
        P[i].opening = 0;
        P[i].stun = P[i].stunNext = 0;
        P[i].sure = FALSE;
        groundCd[i] = 0;
    }
    freeze = 0;
    pend = FALSE;
    comboOn = FALSE;
    prWho = -1;
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
    /* la finale du championnat est annoncee */
    fxBanner((gameType == GAME_ARCADE && ladderPos == LADDER_LEN - 1) ? BAN_FINAL : BAN_OSS, 80);
    stageCheer(120);
    if (gameType == GAME_DEMO) wtxtC("DEMO - APPUIE SUR START", 27);
    actorsDraw();
    SPR_update();
    fadeInScreen(16);
}

void matchLeave(void)
{
    fighterTint(0, 0);
    fighterTint(1, 0);
    stageDim(FALSE);
    fxRelease();
    actorsRelease();
    crowdStop();
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_B, 0);
}

static void frameCommon(void)
{
    hudDraw(matchFrames, FALSE);
    hudUpdate();
    crowdUpdate(pend || phase == PH_SUB || phase == PH_SUPER);
    stageUpdate();
    updateTints();
    updatePrompts();
    fxUpdate();
    gfxUpdate();
    actorsDraw();
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

    /* arret sur image : tout est fige sauf les effets */
    if (freeze > 0)
    {
        freeze--;
        frameCommon();
        return;
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
        case PH_SUPER:
            updateSuper();
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

    /* chute au sol : petit arret sur image et tremblement */
    if (actorsImpact() && freeze < 5) { freeze = 5; stageShake(8); }

    if (phase == PH_STAND || phase == PH_GROUND || phase == PH_SUB || phase == PH_ANIM)
    {
        if (phase != PH_ANIM || next != NX_FIGHT)
        {
            if (--matchFrames <= 0) { matchFrames = 0; timeUp(); }
        }
    }
    frameCommon();
}
