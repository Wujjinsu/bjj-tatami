/*
 * BJJ TATAMI - le combat, facon "UFC" en 16 bits.
 *
 * DEBOUT
 *   B tenu (pres de l'adversaire) : prendre les prises du kimono (jauge PRISE)
 *   A        : double-leg        A + HAUT : projection judo (il faut des prises)
 *   A + BAS  : single-leg        B + BAS  : tirer la garde (il faut des prises)
 *   C        : sprawl (contre double / single)
 *   C + HAUT : bloquer la hanche (contre la projection judo)
 *
 * AU SOL : chaque position propose jusqu'a 3 transitions (A + direction) et 3 soumissions
 *   (B + direction). Directions : HAUT (ou rien), COTE (gauche/droite), BAS.
 *   L'attaque s'annonce ("ATTAQUE !") sans dire laquelle : le defenseur peut la bloquer
 *   en appuyant sur C + LA MEME direction avant qu'elle parte. C'est un jeu de lecture.
 *
 * SOUMISSION : 3 portes. Chacun deplace son curseur avec la croix ; l'attaquant doit
 *   couvrir le curseur du defenseur pour remplir la porte, le defenseur doit lui echapper.
 *
 * SOUFFLE / CARDIO : chaque effort coute du souffle ; le souffle ne remonte jamais
 *   au-dessus du cardio, qui baisse doucement pendant tout le combat.
 */
#include "game.h"

/* ------------------------------------------------------------------ constantes */
#define FEET_Y          200
#define STAND_Y         (FEET_Y - 46)
#define GROUND_SPR_X    128
#define GROUND_SPR_Y    (FEET_Y - 38)
#define MIN_X           36
#define MAX_X           284
#define MIN_GAP         26
#define REACH_SHOOT     46
#define REACH_PULL      52
#define REACH_GRIP      44

#define ROW_HUD1        1
#define ROW_HUD2        2
#define ROW_HUD3        3
#define ROW_INFO        9
#define ROW_GAUGE       10
#define ROW_MSG         11
#define ROW_HINT        12

#define BAR_CELLS       28
#define STALL_FRAMES    1800

enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT, ANIM_SPRAWL };
enum { POS_GUARD, POS_HALF, POS_SIDE, POS_KNEE, POS_MOUNT, POS_BACK, POS_COUNT };
enum { PH_STAND, PH_GROUND, PH_SUB, PH_OVER };
enum { ACT_IDLE, ACT_SHOOT, ACT_DEFEND, ACT_STUMBLE };
enum { TD_DOUBLE, TD_JUDO, TD_SINGLE };
enum { DEF_SPRAWL, DEF_HIP };
enum { PEND_MOVE, PEND_SUB };

#define STANDUP (-1)

typedef struct { const char* name; s8 target; u8 swap; u8 points; u8 adv; s8 diff; } Move;
typedef struct { const char* name; u8 family; s8 diff; } SubMove;

#define MV(n, t, s, p, a, d)    { n, t, s, p, a, d }
#define NOMV                    { NULL, 0, 0, 0, 0, 0 }
#define SB(n, f, d)             { n, f, d }
#define NOSB                    { NULL, 0, 0 }

/* [position][direction HAUT, COTE, BAS] */
static const Move topMoves[POS_COUNT][3] =
{
    { MV("OUVRIR LA GARDE", POS_HALF, 0, 0, 1, 0),   MV("PASSAGE TOREANDO", POS_SIDE, 0, 3, 0, 3),  MV("SE RELEVER", STANDUP, 0, 0, 0, -1) },
    { MV("PASSAGE", POS_SIDE, 0, 3, 0, 1),           MV("PASSAGE MONTEE", POS_MOUNT, 0, 7, 0, 4),   MV("SE RELEVER", STANDUP, 0, 0, 0, 0) },
    { MV("MONTEE", POS_MOUNT, 0, 4, 0, 1),           MV("GENOU AU VENTRE", POS_KNEE, 0, 2, 0, 0),   MV("PRISE DE DOS", POS_BACK, 0, 4, 0, 3) },
    { MV("MONTEE", POS_MOUNT, 0, 4, 0, 0),           NOMV,                                          MV("RETOUR 100 KILOS", POS_SIDE, 0, 0, 0, -2) },
    { MV("PRISE DE DOS", POS_BACK, 0, 4, 0, 1),      NOMV,                                          NOMV },
    { NOMV,                                          NOMV,                                          NOMV },
};
static const Move botMoves[POS_COUNT][3] =
{
    { MV("BALAYAGE CISEAUX", POS_HALF, 1, 2, 0, 1),  MV("PRISE DE DOS", POS_BACK, 1, 4, 0, 4),      MV("SE RELEVER", STANDUP, 0, 0, 0, 0) },
    { MV("REPRENDRE GARDE", POS_GUARD, 0, 0, 0, -1), MV("BALAYAGE", POS_HALF, 1, 2, 0, 2),          MV("SE RELEVER", STANDUP, 0, 0, 0, 1) },
    { MV("SORTIE DE HANCHE", POS_HALF, 0, 0, 0, 0),  MV("REPRENDRE GARDE", POS_GUARD, 0, 0, 0, 2),  NOMV },
    { MV("SORTIE", POS_HALF, 0, 0, 0, 0),            NOMV,                                          NOMV },
    { MV("UPA", POS_GUARD, 1, 0, 0, 1),              MV("COUDE-GENOU", POS_HALF, 0, 0, 0, 0),       NOMV },
    { MV("SE RETOURNER", POS_GUARD, 1, 0, 0, 1),     MV("GLISSER AU SOL", POS_HALF, 0, 0, 0, 0),    NOMV },
};
static const SubMove topSubs[POS_COUNT][3] =
{
    { NOSB,                               NOSB,                                 SB("CLE DE CHEVILLE", SUBF_LEG, 2) },
    { SB("KIMURA", SUBF_KIM, 1),          SB("ETRANGL. D'ARCE", SUBF_CHOKE, 2), NOSB },
    { SB("KIMURA", SUBF_KIM, 0),          SB("AMERICANA", SUBF_ARM, 1),         NOSB },
    { SB("CLE DE BRAS", SUBF_ARM, 0),     NOSB,                                 NOSB },
    { SB("CLE DE BRAS", SUBF_ARM, -1),    SB("ETRANGL. CROISE", SUBF_CHOKE, 0), NOSB },
    { SB("MATA LEAO", SUBF_CHOKE, -2),    SB("ARC ET FLECHE", SUBF_CHOKE, -1),  NOSB },
};
static const SubMove botSubs[POS_COUNT][3] =
{
    { SB("TRIANGLE", SUBF_TRI, 1),        SB("CLE DE BRAS", SUBF_ARM, 1),       SB("GUILLOTINE", SUBF_CHOKE, 2) },
    { SB("KIMURA", SUBF_KIM, 2),          NOSB,                                 NOSB },
    { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB }, { NOSB, NOSB, NOSB },
};

static const char* const posName[POS_COUNT] =
{ "GARDE FERMEE", "DEMI-GARDE", "100 KILOS", "GENOU AU VENTRE", "MONTEE", "DOS" };
static const char* const tdName[3] = { "DOUBLE-LEG", "PROJECTION JUDO", "SINGLE-LEG" };
static const char dirChar[3] = { '^', '>', 'v' };

/* ------------------------------------------------------------------ etat */
Sprite* groundSpr;

static u16 phase;
static s32 matchFrames;
static u16 pos, top;
static s16 msgTimer, overTimer;
static bool hudDirty;

/* attaque en preparation (sol) */
static bool pend;
static u16 pendWho, pendKind, pendDir;
static s16 pendTimer;
static bool pendDenyUsed;

/* soumission */
static u16 subWho, subFam;
static s8 subDiff;
static const char* subNameStr;
static u16 gate;
static s16 tug;
static s16 lenA, lenD;

#ifdef SIM_STATS
long statSubStart[2], statSubWin[2], statMove[2], statMoveOk[2], statDeny[2];
#define STATINC(x) (x)++
#else
#define STATINC(x)
#endif

static const u16 cpuDeny[3]   = { 22, 40, 58 };
static const u16 cpuRead[3]   = { 50, 62, 75 };
static const u16 cpuReact[3]  = { 16, 11, 7 };      /* CPU qui attaque (soumission) */
static const u16 cpuEscape[3] = { 10, 7, 5 };       /* CPU qui defend (soumission) */

#define STAT(i, s)  ((s16)P[i].def->stat[s])
#define OPP(i)      (1 - (i))

/* ------------------------------------------------------------------ messages */
static void message(const char* s)
{
    txtCenter(s, ROW_MSG, TXT_DARK);
    msgTimer = 120;
}

static void msgWho(u16 who, const char* s)
{
    char b[41];
    u16 n = 0;
    const char* p = P[who].label;
    while (*p && n < 38) b[n++] = *p++;
    b[n++] = ' '; b[n++] = ':'; b[n++] = ' ';
    while (*s && n < 40) b[n++] = *s++;
    b[n] = 0;
    message(b);
}

static void msgWhoPts(u16 who, const char* s, u16 pts)
{
    char b[32];
    u16 n = 0;
    while (*s && n < 24) b[n++] = *s++;
    if (pts)
    {
        b[n++] = ' '; b[n++] = '+';
        if (pts >= 10) b[n++] = '0' + pts / 10;
        b[n++] = '0' + pts % 10;
    }
    b[n] = 0;
    msgWho(who, b);
}

/* ------------------------------------------------------------------ souffle / cardio */
static void spend(u16 i, s16 amount)
{
    s16 c = P[i].def->stat[ST_CARDIO];
    P[i].souffle -= amount;
    if (P[i].souffle < 0) P[i].souffle = 0;
    /* le cardio baisse un peu a chaque effort, moins pour les gros cardios */
    P[i].cardio -= (amount * (11 - c)) / 40 + 0;
    if (P[i].cardio < 30) P[i].cardio = 30;
    hudDirty = TRUE;
}

static void regen(u16 i, u16 every)
{
    if ((frameCount % every) == 0 && P[i].souffle < P[i].cardio)
    {
        P[i].souffle++;
        if ((P[i].souffle % 10) == 0) hudDirty = TRUE;
    }
}

/* ------------------------------------------------------------------ HUD */
static void bar10(s16 souffle, s16 cardio, char* out)
{
    u16 k;
    out[0] = '[';
    for (k = 0; k < 10; k++)
    {
        s16 lim = (k + 1) * 10;
        out[1 + k] = (souffle >= lim - 5) ? '#' : (cardio >= lim - 5) ? '-' : ' ';
    }
    out[11] = ']';
    out[12] = 0;
}

static void put(char* line, u16 x, const char* s)
{
    while (*s && x < 40) line[x++] = *s++;
}

/* le HUD est reecrit en entier (sans effacer avant) : pas de clignotement */
static void drawHud(void)
{
    char line[41], b[16];
    s32 secs = (matchFrames + 59) / 60;
    u16 k;

    for (k = 0; k < 40; k++) line[k] = ' ';
    line[40] = 0;
    put(line, 1, P[0].label);
    numStr(P[0].points, 2, b); put(line, 9, b);          /* chiffres deja la : pas de trou */
    numStr(P[1].points, 2, b); put(line, 29, b);
    put(line, 12, "A"); numStr(P[0].adv, 1, b); put(line, 13, b);
    put(line, 15, "P"); numStr(P[0].pen, 1, b); put(line, 16, b);
    numStr((s16)(secs / 60), 1, b); put(line, 18, b);
    put(line, 19, ":");
    numStr((s16)(secs % 60), 2, b); put(line, 20, b);
    put(line, 23, "P"); numStr(P[1].pen, 1, b); put(line, 24, b);
    put(line, 26, "A"); numStr(P[1].adv, 1, b); put(line, 27, b);
    put(line, 39 - slen(P[1].label), P[1].label);
    txt(line, 0, ROW_HUD1, TXT_DARK);
    drawNum(P[0].points, 2, 9, ROW_HUD1, TXT_RED);
    drawNum(P[1].points, 2, 29, ROW_HUD1, TXT_RED);

    for (k = 0; k < 40; k++) line[k] = ' ';
    bar10(P[0].souffle, P[0].cardio, b); put(line, 1, b);
    bar10(P[1].souffle, P[1].cardio, b); put(line, 27, b);
    txt(line, 0, ROW_HUD2, TXT_DARK);

    /* jauges de prises (debout) : "PR #####" a gauche et a droite, hors de la banniere */
    for (k = 0; k < 40; k++) line[k] = ' ';
    txt(line, 0, ROW_HUD3, TXT_DARK);
    if (phase == PH_STAND)
    {
        u16 i;
        for (i = 0; i < 2; i++)
        {
            if (P[i].grip < 10) continue;
            b[0] = 'P'; b[1] = 'R'; b[2] = ' ';
            for (k = 0; k < 5; k++) b[3 + k] = (P[i].grip >= (k + 1) * 20 - 10) ? '#' : '-';
            b[8] = 0;
            txt(b, (i == 0) ? 1 : 31, ROW_HUD3, (P[i].grip >= 50) ? TXT_RED : TXT_DARK);
        }
    }
    hudDirty = FALSE;
}

/* ------------------------------------------------------------------ sprites */
static s16 dirTo(u16 i) { return (P[OPP(i)].x > P[i].x) ? 1 : -1; }
static s16 dist(void)   { s16 d = P[1].x - P[0].x; return (d < 0) ? -d : d; }

void updateFighterSprite(u16 i, s16 y)
{
    Player* f = &P[i];
    s16 anim = ANIM_IDLE, frame = (frameCount / 30) & 1;

    switch (f->act)
    {
        case ACT_SHOOT:   anim = ANIM_SHOOT;  frame = (f->actTimer < 12) ? 0 : 1; break;
        case ACT_DEFEND:  anim = ANIM_SPRAWL; frame = (f->actTimer / 8) & 1;      break;
        case ACT_STUMBLE: anim = ANIM_SHOOT;  frame = 0;                          break;
        default:
            if (f->walking) { anim = ANIM_WALK; frame = (f->walkTick / 8) & 1; }
            break;
    }
    SPR_setAnimAndFrame(f->spr, anim, frame);
    SPR_setHFlip(f->spr, dirTo(i) < 0);
    SPR_setPosition(f->spr, f->x - 16, y);
}

static void showStanding(bool show)
{
    SPR_setVisibility(P[0].spr, show ? VISIBLE : HIDDEN);
    SPR_setVisibility(P[1].spr, show ? VISIBLE : HIDDEN);
    SPR_setVisibility(groundSpr, show ? HIDDEN : VISIBLE);
}

/* ------------------------------------------------------------------ fin du combat */
static void endMatch(s16 winner, const char* method)
{
    phase = PH_OVER;
    overTimer = 210;
    pend = FALSE;
    resWinner = winner;
    scopy(resMethod, method, 31);
    clearRows(ROW_INFO, ROW_HINT + 2);
    if (winner >= 0)
    {
        char b[41];
        u16 n = 0;
        const char* p = P[winner].label;
        while (*p && n < 30) b[n++] = *p++;
        p = " GAGNE !";
        while (*p) b[n++] = *p++;
        b[n] = 0;
        txtCenter(b, ROW_INFO + 1, TXT_RED);
    }
    else txtCenter("EGALITE !", ROW_INFO + 1, TXT_RED);
    txtCenter(method, ROW_INFO + 2, TXT_DARK);
    crowdReact(TRUE);
    if (winner >= 0 && (P[winner].human || numHumans == 0)) playTune(tuneWin, 6);
    else playTune(tuneLose, 4);
    hudDirty = TRUE;
}

static void timeUp(void)
{
    char b[24];
    s16 w = -1;
    const char* how = "DECISION DES ARBITRES";

    if (P[0].points != P[1].points)
    {
        char n0[4], n1[4];
        u16 k = 0;
        const char* q = "AUX POINTS ";
        w = (P[0].points > P[1].points) ? 0 : 1;
        numStr(P[w].points, 2, n0);
        numStr(P[OPP(w)].points, 2, n1);
        while (*q) b[k++] = *q++;
        b[k++] = n0[0]; b[k++] = n0[1]; b[k++] = '-'; b[k++] = n1[0]; b[k++] = n1[1]; b[k] = 0;
        how = b;
    }
    else if (P[0].adv != P[1].adv) { w = (P[0].adv > P[1].adv) ? 0 : 1; how = "AUX AVANTAGES"; }
    else if (P[0].pen != P[1].pen) { w = (P[0].pen < P[1].pen) ? 0 : 1; how = "AUX PENALITES"; }
    else if (P[0].attempts != P[1].attempts) w = (P[0].attempts > P[1].attempts) ? 0 : 1;
    else how = "MATCH NUL";
    endMatch(w, how);
}

/* ------------------------------------------------------------------ penalites (passivite) */
static void stallCheck(u16 i)
{
    if (++P[i].idle < STALL_FRAMES) return;
    P[i].idle = 0;
    P[i].pen++;
    if (P[i].pen == 2) P[OPP(i)].adv++;
    if (P[i].pen >= 3) P[OPP(i)].points += 2;
    msgWho(i, "PENALITE (PASSIVITE)");
    sfx(200, 12);
    hudDirty = TRUE;
}

static void acted(u16 i)
{
    P[i].idle = 0;
    P[i].attempts++;
}

/* ==================================================================== DEBOUT */
static void standHints(void)
{
    clearRows(ROW_INFO, ROW_HINT + 2);
    if (!P[0].human || numHumans == 2) return;
    txt("A DOUBLE-LEG  ^A JUDO   vA SINGLE-LEG", 1, ROW_HINT, TXT_DARK);
    txt("B PRISES (TENIR)    vB TIRER LA GARDE", 1, ROW_HINT + 1, TXT_DARK);
    txt("C SPRAWL      ^C BLOQUER LA HANCHE", 1, ROW_HINT + 2, TXT_DARK);
}

static void enterStand(s16 cx)
{
    u16 i;
    phase = PH_STAND;
    pend = FALSE;
    if (cx < MIN_X + 44) cx = MIN_X + 44;
    if (cx > MAX_X - 44) cx = MAX_X - 44;
    P[0].x = cx - 44;
    P[1].x = cx + 44;
    for (i = 0; i < 2; i++)
    {
        P[i].act = ACT_IDLE;
        P[i].actTimer = 0;
        P[i].cooldown = 30;
        P[i].grip = 0;
        P[i].walking = FALSE;
        P[i].aiWant = 0;
        P[i].aiTimer = 30;
        updateFighterSprite(i, STAND_Y);
    }
    showStanding(TRUE);
    standHints();
    hudDirty = TRUE;
}

static void enterGround(u16 p, u16 newTop);

static void moveFighter(u16 i, s16 dx, s16 gap)
{
    s16 nx = P[i].x + dx;
    s16 ox = P[OPP(i)].x;
    s16 dn, d0;

    if (nx < MIN_X) nx = MIN_X;
    if (nx > MAX_X) nx = MAX_X;
    dn = nx - ox; if (dn < 0) dn = -dn;
    d0 = P[i].x - ox; if (d0 < 0) d0 = -d0;
    if (dn < gap && dn < d0) return;
    if ((P[i].x < ox) != (nx < ox)) return;
    if (nx == P[i].x) return;
    P[i].x = nx;
    P[i].walking = TRUE;
    P[i].walkTick++;
}

static void startShoot(u16 i, u16 type)
{
    if (type == TD_JUDO && P[i].grip < 40)
    {
        msgWho(i, "PAS ASSEZ DE PRISES !");
        P[i].cooldown = 20;
        return;
    }
    P[i].act = ACT_SHOOT;
    P[i].actTimer = 0;
    P[i].tdType = type;
    spend(i, 10);
    acted(i);
    sfx(220, 6);
}

static void startDefend(u16 i, u16 kind)
{
    P[i].act = ACT_DEFEND;
    P[i].actTimer = 0;
    P[i].defKind = kind;
    spend(i, 4);
}

static void resolveShoot(u16 att)
{
    u16 def = OPP(att);
    s16 chance;

    if (dist() > REACH_SHOOT + 4)
    {
        msgWho(att, "TROP LOIN !");
        P[att].act = ACT_STUMBLE; P[att].cooldown = 18;
        return;
    }
    if (P[def].act == ACT_DEFEND)
    {
        bool good = (P[att].tdType == TD_JUDO) ? (P[def].defKind == DEF_HIP) : (P[def].defKind == DEF_SPRAWL);
        if (good)
        {
            msgWho(def, (P[def].defKind == DEF_HIP) ? "HANCHE BLOQUEE ! CONTRE" : "SPRAWL ! CONTRE");
            spend(att, 12);
            P[att].act = ACT_STUMBLE; P[att].cooldown = 30;
            sfx(150, 8);
            crowdReact(FALSE);
            return;
        }
    }

    chance = 45 + (STAT(att, ST_LUTTE) - STAT(def, ST_LUTTE)) * 6 + (P[att].souffle - P[def].souffle) / 4;
    if (P[att].tdType == TD_DOUBLE) chance += 5;
    if (P[att].tdType == TD_JUDO) chance += P[att].grip / 3 - 5;
    if (P[def].act == ACT_DEFEND) chance += 15;          /* mauvaise defense : deseiquilibre */
    if (chance < 10) chance = 10;
    if (chance > 90) chance = 90;

    if ((s16)(random() % 100) < chance)
    {
        P[att].points += 2;
        sfxSlam();
        crowdReact(FALSE);
        msgWhoPts(att, tdName[P[att].tdType], 2);
        enterGround(P[att].tdType == TD_JUDO ? POS_SIDE : (P[att].tdType == TD_SINGLE ? POS_HALF : POS_GUARD), att);
    }
    else
    {
        msgWho(att, "PROJECTION RATEE");
        P[att].act = ACT_STUMBLE; P[att].cooldown = 22;
    }
}

static void tryPullGuard(u16 i)
{
    if (dist() > REACH_PULL) return;
    if (P[i].grip < 25) { msgWho(i, "IL FAUT DES PRISES"); P[i].cooldown = 15; return; }
    acted(i);
    msgWho(i, "TIRE LA GARDE");
    enterGround(POS_GUARD, OPP(i));
}

/* ---------------- IA debout */
static void aiStand(u16 i)
{
    Player* me = &P[i];
    Player* op = &P[OPP(i)];
    s16 d = dist();
    s16 r;
    u16 dirTowards = (dirTo(i) > 0) ? BUTTON_RIGHT : BUTTON_LEFT;
    u16 dirAway = (dirTo(i) > 0) ? BUTTON_LEFT : BUTTON_RIGHT;

    /* reaction a une projection adverse */
    if (op->act == ACT_SHOOT && op->actTimer == 1)
    {
        u16 chance = 25 + difficulty * 20 + me->def->stat[ST_LUTTE] * 2;
        if ((random() % 100) < chance)
        {
            bool read = (random() % 100) < (u16)(55 + difficulty * 15);
            u16 kind = (op->tdType == TD_JUDO) ? DEF_HIP : DEF_SPRAWL;
            if (!read) kind = 1 - kind;
            me->aiDenyAt = 3 + (random() % 7);
            me->aiDenyBtn = BUTTON_C | (kind == DEF_HIP ? BUTTON_UP : 0);
        }
    }
    if (me->aiDenyAt > 0)
    {
        if (--me->aiDenyAt == 0) me->aiWant = me->aiDenyBtn;
    }

    if (me->aiWant) return;
    if (me->act != ACT_IDLE || me->cooldown > 0) return;

    if (--me->aiTimer <= 0)
    {
        me->aiTimer = 18 + (random() % 30) - difficulty * 5;
        r = random() % 100;
        me->aiMoveDir = 0;
        if (d <= REACH_SHOOT)
        {
            if (me->grip >= 45 && me->def->stat[ST_LUTTE] >= 6 && r < 30) { me->aiWant = BUTTON_A | BUTTON_UP; return; }
            if (me->grip >= 25 && me->def->stat[ST_GARDE] >= 7 && r >= 85) { me->aiWant = BUTTON_B | BUTTON_DOWN; return; }
            if (r < 28 + difficulty * 8) { me->aiWant = BUTTON_A | ((r & 1) ? BUTTON_DOWN : 0); return; }
            me->aiMoveDir = (r > 80) ? -1 : 0;
        }
        else me->aiMoveDir = (r < 85) ? 1 : 0;
    }

    if (me->aiMoveDir > 0) me->pad |= dirTowards;
    else if (me->aiMoveDir < 0) me->pad |= dirAway;
    /* combat de prises quand on est a distance */
    if (d <= REACH_GRIP && (frameCount & 63) < (u16)(30 + difficulty * 10)) me->pad |= BUTTON_B;
}

/* ---------------- un combattant debout */
static bool standAction(u16 i)
{
    Player* f = &P[i];

    if (f->act == ACT_SHOOT)
    {
        f->actTimer++;
        if (f->actTimer >= 12 && f->actTimer < 18) moveFighter(i, dirTo(i) * 2, 14);
        if (f->actTimer == 18)
        {
            resolveShoot(i);
            if (phase != PH_STAND) return TRUE;
            if (f->act == ACT_SHOOT) { f->act = ACT_IDLE; f->cooldown = 16; }
        }
        return TRUE;
    }
    if (f->act == ACT_DEFEND)
    {
        if (++f->actTimer >= 22) { f->act = ACT_IDLE; f->cooldown = 18; }
        return TRUE;
    }
    if (f->act == ACT_STUMBLE)
    {
        if (--f->cooldown <= 0) { f->act = ACT_IDLE; f->cooldown = 0; }
        return TRUE;
    }
    if (f->cooldown > 0) { f->cooldown--; return TRUE; }
    return FALSE;
}

static void standInput(u16 i)
{
    Player* f = &P[i];
    bool gripping = FALSE;
    s16 sp = (f->souffle < 25 && (frameCount & 1)) ? 0 : 1;

    if (pressed(i, BUTTON_A))
    {
        u16 t = TD_DOUBLE;
        if (held(i, BUTTON_UP)) t = TD_JUDO;
        else if (held(i, BUTTON_DOWN)) t = TD_SINGLE;
        startShoot(i, t);
        return;
    }
    if (pressed(i, BUTTON_B) && held(i, BUTTON_DOWN)) { tryPullGuard(i); return; }
    if (pressed(i, BUTTON_C)) { startDefend(i, held(i, BUTTON_UP) ? DEF_HIP : DEF_SPRAWL); return; }

    if (held(i, BUTTON_LEFT))  moveFighter(i, -sp, MIN_GAP);
    if (held(i, BUTTON_RIGHT)) moveFighter(i, sp, MIN_GAP);

    if (held(i, BUTTON_B) && !held(i, BUTTON_DOWN) && dist() <= REACH_GRIP)
    {
        gripping = TRUE;
        if (f->grip < 100) { f->grip += 2; if (f->grip > 100) f->grip = 100; }
        if (P[OPP(i)].grip > 0) P[OPP(i)].grip--;
        if ((frameCount % 20) == 0) spend(i, 1);
        if ((frameCount & 15) == 0) hudDirty = TRUE;
    }
    if (!gripping && f->grip > 0 && (frameCount & 1)) f->grip--;
    if (dist() > REACH_GRIP + 12) f->grip = 0;
}

static void updateStand(void)
{
    u16 i;

    P[0].walking = P[1].walking = FALSE;
    for (i = 0; i < 2; i++)
    {
        if (!P[i].human) aiStand(i);
    }
    for (i = 0; i < 2 && phase == PH_STAND; i++)
    {
        if (!standAction(i)) standInput(i);
    }
    if (phase != PH_STAND) return;

    for (i = 0; i < 2; i++)
    {
        if (P[i].act == ACT_IDLE) regen(i, 6);
        stallCheck(i);
        updateFighterSprite(i, STAND_Y);
    }
}

/* ==================================================================== SOL */
static const Move* moveFor(u16 who, u16 dir)
{
    const Move* m = (who == top) ? &topMoves[pos][dir] : &botMoves[pos][dir];
    return m->name ? m : NULL;
}

static const SubMove* subFor(u16 who, u16 dir)
{
    const SubMove* s = (who == top) ? &topSubs[pos][dir] : &botSubs[pos][dir];
    return s->name ? s : NULL;
}

static void groundInfo(void)
{
    char b[41];
    u16 n = 0, d;
    const char* p = P[top].label;

    while (*p && n < 16) b[n++] = *p++;
    p = " DESSUS - ";
    while (*p) b[n++] = *p++;
    p = posName[pos];
    while (*p && n < 40) b[n++] = *p++;
    b[n] = 0;
    txtCenter(b, ROW_INFO, TXT_DARK);
    clearRow(ROW_GAUGE);

    clearRows(ROW_HINT, ROW_HINT + 2);
    if (!P[0].human || numHumans == 2) return;
    for (d = 0; d < 3; d++)
    {
        const Move* m = moveFor(0, d);
        const SubMove* s = subFor(0, d);
        char a[2] = { dirChar[d], 0 };
        if (!m && !s) continue;
        txt(a, 0, ROW_HINT + d, TXT_RED);
        if (m) { txt("A", 2, ROW_HINT + d, TXT_RED); txt(m->name, 4, ROW_HINT + d, TXT_DARK); }
        if (s) { txt("B", 22, ROW_HINT + d, TXT_RED); txt(s->name, 24, ROW_HINT + d, TXT_DARK); }
    }
}

static void enterGround(u16 p, u16 newTop)
{
    u16 i;
    phase = PH_GROUND;
    pos = p;
    top = newTop;
    pend = FALSE;
    for (i = 0; i < 2; i++)
    {
        P[i].cooldown = 30;
        P[i].act = ACT_IDLE;
        P[i].grip = 0;
        P[i].aiWant = 0;
        P[i].aiDenyAt = 0;
        P[i].aiTimer = 40 + (random() % 40);
    }
    setGroundPalette(top);
    SPR_setAnimAndFrame(groundSpr, pos, 0);
    SPR_setPosition(groundSpr, GROUND_SPR_X, GROUND_SPR_Y);
    showStanding(FALSE);
    groundInfo();
    hudDirty = TRUE;
}

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
    txtCenter((who == 0) ? "<<<  ATTAQUE !" : "ATTAQUE !  >>>", ROW_GAUGE, TXT_RED);
    sfx(kind == PEND_SUB ? 700 : 440, 6);

    /* IA en defense : va-t-elle lire l'attaque ? */
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
        msgWho(who, m->name);
        enterStand(160);
        return;
    }
    if (m->swap) top = who;
    pos = m->target;
    P[who].points += m->points;
    P[who].adv += m->adv;
    if (m->adv && !m->points) msgWho(who, "AVANTAGE ! GARDE OUVERTE");
    else msgWhoPts(who, m->name, m->points);
    if (m->points || m->swap) { sfxSlam(); crowdReact(FALSE); }
    else sfx(520, 6);
    setGroundPalette(top);
    SPR_setAnimAndFrame(groundSpr, pos, 0);
    groundInfo();
    hudDirty = TRUE;
}

static void resolvePend(void)
{
    u16 who = pendWho, d = OPP(who);
    pend = FALSE;
    P[who].cooldown = 40;
    P[d].cooldown = 12;
    clearRow(ROW_GAUGE);

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
        STATINC(statMove[who]);
        if ((s16)(random() % 100) < chance) { STATINC(statMoveOk[who]); applyMove(who, m); }
        else { msgWho(d, "RESISTE"); sfx(160, 6); }
    }
}

static void denied(u16 d)
{
    u16 who = pendWho;
    STATINC(statDeny[d]);
    pend = FALSE;
    spend(who, 12);
    P[who].cooldown = 30;
    P[d].cooldown = 6;
    P[d].idle = 0;
    clearRow(ROW_GAUGE);
    msgWho(d, "BLOQUE ! BONNE LECTURE");
    sfx(180, 10);
    crowdReact(FALSE);
}

/* ---------------- IA au sol */
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
    if (pend || me->cooldown > 0 || me->aiWant) return;
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
            if (i != top && m->target != STANDUP) sc += 15;      /* dessous : sortir d'abord */
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
        SPR_setFrame(groundSpr, (frameCount / 3) & 1);
        if (!pendDenyUsed && pressed(d, BUTTON_C))
        {
            pendDenyUsed = TRUE;
            if (dirOf(d) == pendDir) { denied(d); return; }
        }
        if (--pendTimer <= 0) resolvePend();
        return;
    }

    SPR_setFrame(groundSpr, 0);
    for (i = 0; i < 2; i++)
    {
        if (P[i].cooldown > 0) { P[i].cooldown--; continue; }
        if (pressed(i, BUTTON_A))
        {
            u16 d = dirOf(i);
            if (moveFor(i, d)) { startPend(i, PEND_MOVE, d); return; }
        }
        if (pressed(i, BUTTON_B))
        {
            u16 d = dirOf(i);
            if (subFor(i, d)) { startPend(i, PEND_SUB, d); return; }
        }
    }

    /* recuperation : sous la pression, celui du dessous respire mal */
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
    phase = PH_SUB;
    subWho = who;
    subFam = s->family;
    subDiff = s->diff;
    subNameStr = s->name;
    gate = 0;
    tug = 0;
    STATINC(statSubStart[who]);

    lenA = 3 + STAT(who, ST_SOUM) / 3 + (P[who].def->special == subFam ? 1 : 0) - subDiff - (P[who].souffle < 30 ? 1 : 0);
    if (lenA < 3) lenA = 3;
    if (lenA > 8) lenA = 8;
    lenD = 3 + (STAT(d, ST_SOUM) + STAT(d, ST_CARDIO)) / 6 - (P[d].souffle < 30 ? 1 : 0);
    if (lenD < 2) lenD = 2;
    subLayout();
    P[0].aiTimer = P[1].aiTimer = 0;

    clearRows(ROW_INFO, ROW_HINT + 2);
    msgWho(who, s->name);
    sfx(880, 10);
    crowdReact(FALSE);
}

static void drawSub(void)
{
    char b[41];
    u16 k, n = 0;
    s16 a0 = P[subWho].cur / 16, d0 = P[OPP(subWho)].cur / 16;
    const char* p = subNameStr;

    while (*p) b[n++] = *p++;
    p = "  PORTE ";
    while (*p) b[n++] = *p++;
    b[n++] = '1' + gate; b[n++] = '/'; b[n++] = '3'; b[n] = 0;
    txtCenter(b, ROW_INFO, TXT_DARK);

    /* barre : '=' attaquant (rouge), '#' defenseur (vert), 'X' quand ils se couvrent.
       4 ecritures maximum : fond, defenseur (2 morceaux autour de l'attaquant), attaquant */
    {
        char base[BAR_CELLS + 3], seg[BAR_CELLS + 1];
        s16 a1 = a0 + lenA, d1 = d0 + lenD, x;
        for (k = 0; k < BAR_CELLS; k++) base[1 + k] = '-';
        base[0] = '['; base[BAR_CELLS + 1] = ']'; base[BAR_CELLS + 2] = 0;
        txt(base, 5, ROW_GAUGE, TXT_DARK);

        /* defenseur, a gauche de l'attaquant */
        n = 0;
        for (x = d0; x < d1 && x < a0; x++) seg[n++] = '#';
        seg[n] = 0;
        if (n) txt(seg, 6 + d0, ROW_GAUGE, TXT_GREEN);
        /* defenseur, a droite de l'attaquant */
        n = 0;
        for (x = (d0 > a1 ? d0 : a1); x < d1; x++) seg[n++] = '#';
        seg[n] = 0;
        if (n) txt(seg, 6 + (d0 > a1 ? d0 : a1), ROW_GAUGE, TXT_GREEN);
        /* attaquant, 'X' sur le defenseur */
        n = 0;
        for (x = a0; x < a1; x++) seg[n++] = (x >= d0 && x < d1) ? 'X' : '=';
        seg[n] = 0;
        txt(seg, 6 + a0, ROW_GAUGE, TXT_RED);
    }

    /* jauge de la porte */
    {
        s16 idx = (tug + 100) * 20 / 200;
        if (idx < 0) idx = 0;
        if (idx > 20) idx = 20;
        n = 0;
        p = "ECHAPPE [";
        while (*p) b[n++] = *p++;
        for (k = 0; k < 21; k++) b[n++] = ((s16)k == idx) ? 'O' : (k == 10 ? '|' : '-');
        p = "] TAPE";
        while (*p) b[n++] = *p++;
        b[n] = 0;
        txtCenter(b, ROW_MSG, TXT_DARK);
    }
}

static void subHints(void)
{
    clearRows(ROW_HINT, ROW_HINT + 2);
    if (numHumans == 0) return;
    txt("= ", 3, ROW_HINT, TXT_RED);
    txt(P[subWho].label, 5, ROW_HINT, TXT_DARK);
    txt(": COUVRE L'AUTRE CURSEUR", 5 + slen(P[subWho].label), ROW_HINT, TXT_DARK);
    txt("# ", 3, ROW_HINT + 1, TXT_GREEN);
    txt(P[OPP(subWho)].label, 5, ROW_HINT + 1, TXT_DARK);
    txt(": ECHAPPE-LUI (CROIX)", 5 + slen(P[OPP(subWho)].label), ROW_HINT + 1, TXT_DARK);
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
            s16 dc = P[OPP(i)].cur + lenD * 8;               /* centre du defenseur */
            s16 noise = 49 - difficulty * 8;
            me->aiTarget = dc - lenA * 8 + ((s16)(random() % noise) - noise / 2);
        }
        else
        {
            s16 ac = P[OPP(i)].cur + lenA * 8;               /* centre de l'attaquant */
            /* fuir vers le cote qui a le plus de place */
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
    /* il faut couvrir au moins la moitie du curseur adverse */
    if (ov * 2 >= lenD * 16) tug += (P[subWho].def->special == subFam) ? 3 : 2;
    else tug -= 2;

    SPR_setFrame(groundSpr, (frameCount / 4) & 1);
    if ((frameCount % 3) == 0) drawSub();

    if (tug >= 100)
    {
        gate++;
        sfx(1000 + gate * 200, 8);
        crowdReact(FALSE);
        if (gate >= 3)
        {
            char b[32];
            u16 n = 0;
            const char* p = "SOUMISSION : ";
            while (*p) b[n++] = *p++;
            p = subNameStr;
            while (*p && n < 31) b[n++] = *p++;
            b[n] = 0;
            SPR_setFrame(groundSpr, 0);
            STATINC(statSubWin[subWho]);
            endMatch(subWho, b);
            return;
        }
        tug = 0;
        subLayout();
    }
    else if (tug <= -100)
    {
        if (gate >= 1) { P[subWho].adv++; msgWho(d, "S'ECHAPPE ! (AVANTAGE)"); }
        else msgWho(d, "S'ECHAPPE !");
        spend(subWho, 20);
        sfx(160, 10);
        phase = PH_GROUND;
        P[subWho].cooldown = 120;            /* rater une soumission, ca se paie */
        P[d].cooldown = 10;
        groundInfo();
        hudDirty = TRUE;
    }
}

/* ==================================================================== boucle */
void match_enter(void)
{
    u16 i;

    gameMode = MODE_MATCH;
    showBackground(&bg_dojo);
    applyPlayerPalettes();
    for (i = 0; i < 2; i++)
    {
        P[i].souffle = 100;
        P[i].cardio = 100;
        P[i].points = P[i].adv = P[i].pen = 0;
        P[i].attempts = 0;
        P[i].idle = 0;
        P[i].aiDenyAt = 0;
        P[i].aiMoveDir = 0;
        P[i].pad = P[i].prev = 0xFFFF;          /* evite un appui fantome au demarrage */
    }
    matchFrames = (numHumans == 0) ? 60L * 60 : (s32)matchMinutes * 60 * 60;
    msgTimer = 0;
    crowdStart();
    enterStand(160);
    drawHud();
    message("COMBATE !");
    playTune(tuneStart, 3);
    if (numHumans == 0) txtCenter("DEMO - APPUIE SUR START", 23, TXT_WHITE);
}

void match_update(void)
{
    u16 i;

    /* les IA "appuient" sur leurs boutons (une pression = un front montant) */
    for (i = 0; i < 2; i++)
    {
        if (!P[i].human && P[i].aiWant)
        {
            if (P[i].prev & P[i].aiWant & BTN_ACTS) continue;   /* relacher d'abord */
            P[i].pad |= P[i].aiWant;
            P[i].aiWant = 0;
        }
    }

    switch (phase)
    {
        case PH_STAND:  updateStand();  break;
        case PH_GROUND: updateGround(); break;
        case PH_SUB:
            updateSub();
            if (phase == PH_SUB && (frameCount % 30) == 0) subHints();
            break;
        case PH_OVER:
            if (--overTimer <= 0) { result_enter(); return; }
            break;
    }

    if (phase != PH_OVER)
    {
        if (--matchFrames <= 0) { matchFrames = 0; timeUp(); }
    }
    if (hudDirty || (frameCount & 15) == 0) drawHud();
    if (msgTimer > 0 && --msgTimer == 0 && phase != PH_SUB) clearRow(ROW_MSG);
    crowdUpdate(pend || phase == PH_SUB);
}
