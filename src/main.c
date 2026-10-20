/*
 * BJJ TATAMI - prototype de jeu de jiu-jitsu bresilien pour Mega Drive (SGDK 2.11)
 *
 * Debout  : A = projection (double leg), B = tirer la garde, C = sprawl (defense)
 * Au sol  : A = progresser / s'echapper / balayer, B = soumission
 * Lutte   : marteler A, B ou C pour pousser la jauge
 * Points IBJJF : projection 2, balayage 2, passage 3, montee 4, dos 4.
 */
#include <genesis.h>
#include "resources.h"

/* ------------------------------------------------------------------ reglages */
#define P_PLAYER        0
#define P_CPU           1

#define FEET_Y          200
#define STAND_Y         (FEET_Y - 46)
#define GROUND_SPR_X    128
#define GROUND_SPR_Y    (FEET_Y - 38)
#define MIN_X           36
#define MAX_X           284
#define MIN_GAP         26
#define REACH_SHOOT     46
#define REACH_PULL      54

#define MATCH_SECONDS   120
#define GAUGE_MAX       400

#define ROW_HUD1        1
#define ROW_HUD2        2
#define ROW_BANNER      6
#define ROW_POS         9
#define ROW_GAUGE       10
#define ROW_MSG         11
#define ROW_HINT1       13
#define ROW_HINT2       14

enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT, ANIM_SPRAWL };
enum { POS_GUARD, POS_HALF, POS_SIDE, POS_MOUNT, POS_BACK };
enum { MODE_TITLE, MODE_STAND, MODE_GROUND, MODE_END };
enum { ACT_IDLE, ACT_SHOOT, ACT_SPRAWL, ACT_STUMBLE };
enum { MOVE_ADVANCE, MOVE_SUB };

typedef struct
{
    s16 x;
    s16 stamina;        /* souffle 0..100 */
    s16 points;
    s16 adv;            /* avantages */
    u16 act;
    s16 actTimer;
    s16 cooldown;
    u16 walkTick;
    bool walking;
    Sprite* spr;
} Fighter;

static Fighter F[2];
static u16 mode;
static u16 difficulty = 1;          /* 0 facile, 1 normal, 2 difficile */
static s16 matchFrames;
static u16 frameCount;

/* sol */
static u16 pos;
static u16 top;                     /* combattant au-dessus (ou dans le dos) */
static Sprite* groundSpr;
static s16 groundCooldown;
static s16 cpuThink;
static s16 cpuMoveTimer;
static s16 cpuMoveDir;
static s16 cpuSprawlDelay;

/* lutte */
static bool sActive;
static u16 sWho;
static u16 sMove;
static s16 sGauge;
static s16 sTime;

/* entrees, palettes, sons, textes */
static u16 pad, padPrev;
static u16 palP[16], palC[16], palG[16];
static s16 msgTimer;
static s16 sfxTimer;
static const u16* tunePtr;
static u16 tuneLen, tuneIdx;
static s16 tuneTimer;
static u16 musicStage;               /* 0 rien, 1 intro, 2 boucle */

#define COL_WHITE       RGB24_TO_VDPCOLOR(0xECECEC)
#define COL_WHITE_SH    RGB24_TO_VDPCOLOR(0xA2AAC6)
#define COL_BLACK_BELT  RGB24_TO_VDPCOLOR(0x181818)
#define COL_RED_BAR     RGB24_TO_VDPCOLOR(0xD82424)
#define COL_BLUE        RGB24_TO_VDPCOLOR(0x245AD8)
#define COL_BLUE_SH     RGB24_TO_VDPCOLOR(0x123690)
#define COL_PURPLE      RGB24_TO_VDPCOLOR(0x9036B4)
#define COL_TEXT_DARK   RGB24_TO_VDPCOLOR(0x121236)

static const u16 cpuPressRate[3]  = { 18, 31, 40 };   /* sur 256, par image */
static const s16 cpuSprawlPct[3]  = { 25, 45, 65 };
static const s16 cpuShootBonus[3] = { -10, 0, 10 };

static const char* const posName[5] =
{ "GARDE FERMEE", "DEMI-GARDE", "CONTROLE LATERAL", "MONTEE", "DOS" };

static const u16 tuneWin[]  = { 523, 659, 784, 1046, 0, 1046 };
static const u16 tuneLose[] = { 392, 330, 262, 196 };
static const u16 tuneStart[] = { 784, 0, 784 };

/* ------------------------------------------------------------------ outils */
static s16 iabs(s16 v) { return (v < 0) ? -v : v; }

static bool pressed(u16 b) { return (pad & b) && !(padPrev & b); }

static void addStamina(u16 i, s16 d)
{
    s16 s = F[i].stamina + d;
    if (s < 0) s = 0;
    if (s > 100) s = 100;
    F[i].stamina = s;
}

static u16 slen(const char* s) { u16 n = 0; while (s[n]) n++; return n; }

static void textDark(const char* s, u16 x, u16 y)
{
    VDP_setTextPalette(PAL1);
    VDP_drawText(s, x, y);
}

static void textLight(const char* s, u16 x, u16 y)
{
    VDP_setTextPalette(PAL0);
    VDP_drawText(s, x, y);
}

static void textCenter(const char* s, u16 y)
{
    VDP_clearTextLine(y);
    textDark(s, (40 - slen(s)) / 2, y);
}

static void textCenterLight(const char* s, u16 y)
{
    VDP_clearTextLine(y);
    textLight(s, (40 - slen(s)) / 2, y);
}

static void message(const char* s)
{
    textCenter(s, ROW_MSG);
    msgTimer = 120;
}

/* message precede de qui agit : "TOI : ..." ou "CPU : ..." */
static void messageWho(u16 who, const char* s)
{
    char b[41];
    const char* p = (who == P_PLAYER) ? "TOI : " : "CPU : ";
    u16 n = 0;
    while (*p && n < 40) b[n++] = *p++;
    while (*s && n < 40) b[n++] = *s++;
    b[n] = 0;
    message(b);
}

/* nombre avec zeros devant (digits chiffres) */
static void putNum(s16 v, u16 digits, u16 x, u16 y)
{
    char b[6];
    u16 i;
    if (v < 0) v = 0;
    for (i = 0; i < digits; i++)
    {
        b[digits - 1 - i] = '0' + (v % 10);
        v /= 10;
    }
    b[digits] = 0;
    textDark(b, x, y);
}

/* ------------------------------------------------------------------ sons (PSG) */
static void sfx(u16 freq, s16 frames)
{
    PSG_setFrequency(0, freq);
    PSG_setEnvelope(0, 2);
    sfxTimer = frames;
}

static void sfxSlam(void)
{
    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK2);
    PSG_setEnvelope(3, 2);
    sfxTimer = 10;
}

static void playTune(const u16* t, u16 len)
{
    tunePtr = t;
    tuneLen = len;
    tuneIdx = 0;
    tuneTimer = 0;
}

static void soundUpdate(void)
{
    if (tunePtr)
    {
        if (tuneTimer <= 0)
        {
            if (tuneIdx >= tuneLen)
            {
                tunePtr = NULL;
                PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
            }
            else
            {
                u16 f = tunePtr[tuneIdx++];
                if (f)
                {
                    PSG_setFrequency(1, f);
                    PSG_setEnvelope(1, 3);
                }
                else PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
                tuneTimer = 9;
            }
        }
        tuneTimer--;
    }

    if (sfxTimer > 0)
    {
        sfxTimer--;
        if (sfxTimer == 0)
        {
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
        }
    }
}

/* ------------------------------------------------------------------ public */
/* Driver PCM4 pendant le match : voie 1 = ambiance en boucle, voie 2 = acclamations */
static bool crowdOn;
static s16 crowdBoost;
static u8 crowdVol;

static void crowdStart(void)
{
    SND_PCM4_startPlay(crowd_loop, sizeof(crowd_loop), SOUND_PCM_CH1, TRUE);
    crowdVol = 6;
    SND_PCM4_setVolume(SOUND_PCM_CH1, crowdVol);
    crowdBoost = 0;
    crowdOn = TRUE;
}

static void crowdStop(void)
{
    if (!crowdOn) return;
    SND_PCM4_stopPlay(SOUND_PCM_CH1);
    SND_PCM4_stopPlay(SOUND_PCM_CH2);
    crowdOn = FALSE;
}

static void crowdReact(bool big)
{
    if (!crowdOn) return;
    if (big) SND_PCM4_startPlay(cheer_big, sizeof(cheer_big), SOUND_PCM_CH2, FALSE);
    else SND_PCM4_startPlay(cheer_small, sizeof(cheer_small), SOUND_PCM_CH2, FALSE);
    SND_PCM4_setVolume(SOUND_PCM_CH2, 15);
    crowdBoost = big ? 240 : 90;
}

/* le public monte pendant les luttes et apres chaque action */
static void crowdUpdate(void)
{
    u8 v = 6;
    if (!crowdOn) return;
    if (crowdBoost > 0) crowdBoost--;
    if (sActive) v += 3;
    if (crowdBoost > 0) v += 4;
    if (v > 15) v = 15;
    if (v != crowdVol)
    {
        crowdVol = v;
        SND_PCM4_setVolume(SOUND_PCM_CH1, v);
    }
}

/* ------------------------------------------------------------------ palettes */
static void setupPalettes(void)
{
    memcpy(palP, spr_fighter.palette->data, 32);
    memcpy(palC, spr_fighter.palette->data, 32);
    memcpy(palG, spr_ground.palette->data, 32);

    palP[15] = COL_TEXT_DARK;          /* PAL1 sert aussi de palette de texte fonce */

    palC[5] = COL_BLUE;
    palC[6] = COL_BLUE_SH;
    palC[7] = COL_PURPLE;
    palC[11] = COL_PURPLE;

    PAL_setPalette(PAL1, palP, CPU);
    PAL_setPalette(PAL2, palC, CPU);
}

/* groupe A (5,6,7) = dessus, groupe B (8,9,10) = dessous */
static void setGroundPalette(void)
{
    u16 w = (top == P_PLAYER) ? 5 : 8;
    u16 b = (top == P_PLAYER) ? 8 : 5;

    palG[w]     = COL_WHITE;
    palG[w + 1] = COL_WHITE_SH;
    palG[w + 2] = COL_BLACK_BELT;
    palG[b]     = COL_BLUE;
    palG[b + 1] = COL_BLUE_SH;
    palG[b + 2] = COL_PURPLE;
    palG[11] = (top == P_PLAYER) ? COL_RED_BAR : COL_PURPLE;
    PAL_setPalette(PAL3, palG, CPU);
}

/* ------------------------------------------------------------------ HUD */
static void drawBar(s16 v, u16 x, u16 y)
{
    char b[13];
    u16 i;
    s16 n = (v + 9) / 10;
    b[0] = '[';
    for (i = 0; i < 10; i++) b[1 + i] = (i < n) ? '#' : '-';
    b[11] = ']';
    b[12] = 0;
    textDark(b, x, y);
}

static void drawHud(void)
{
    s16 secs = (matchFrames + 59) / 60;

    textDark("TOI", 1, ROW_HUD1);
    putNum(F[0].points, 2, 5, ROW_HUD1);
    textDark("AV", 8, ROW_HUD1);
    putNum(F[0].adv, 1, 10, ROW_HUD1);

    putNum(secs / 60, 1, 18, ROW_HUD1);
    textDark(":", 19, ROW_HUD1);
    putNum(secs % 60, 2, 20, ROW_HUD1);

    textDark("AV", 27, ROW_HUD1);
    putNum(F[1].adv, 1, 29, ROW_HUD1);
    putNum(F[1].points, 2, 31, ROW_HUD1);
    textDark("CPU", 36, ROW_HUD1);

    drawBar(F[0].stamina, 1, ROW_HUD2);
    drawBar(F[1].stamina, 27, ROW_HUD2);
}

static void drawGauge(void)
{
    char b[32];
    u16 i;
    s16 pg = (sWho == P_PLAYER) ? sGauge : -sGauge;   /* vers la droite = le joueur gagne */
    s16 idx = (pg + GAUGE_MAX) * 20 / (GAUGE_MAX * 2);

    if (idx < 0) idx = 0;
    if (idx > 20) idx = 20;

    b[0] = 'C'; b[1] = 'P'; b[2] = 'U'; b[3] = ' '; b[4] = '[';
    for (i = 0; i < 21; i++) b[5 + i] = (i == 10) ? '|' : '-';
    b[5 + idx] = 'O';
    b[26] = ']'; b[27] = ' '; b[28] = 'T'; b[29] = 'O'; b[30] = 'I'; b[31] = 0;
    textDark(b, 4, ROW_GAUGE);
}

/* ------------------------------------------------------------------ sprites debout */
static s16 dirTo(u16 i) { return (F[1 - i].x > F[i].x) ? 1 : -1; }
static s16 dist(void) { return iabs(F[1].x - F[0].x); }

static void showStanding(bool show)
{
    SPR_setVisibility(F[0].spr, show ? VISIBLE : HIDDEN);
    SPR_setVisibility(F[1].spr, show ? VISIBLE : HIDDEN);
}

static void updateFighterSprite(u16 i)
{
    Fighter* f = &F[i];
    s16 anim = ANIM_IDLE, frame = (frameCount / 30) & 1;

    switch (f->act)
    {
        case ACT_SHOOT:   anim = ANIM_SHOOT;  frame = (f->actTimer < 12) ? 0 : 1; break;
        case ACT_SPRAWL:  anim = ANIM_SPRAWL; frame = (f->actTimer / 8) & 1;      break;
        case ACT_STUMBLE: anim = ANIM_SHOOT;  frame = 0;                          break;
        default:
            if (f->walking) { anim = ANIM_WALK; frame = (f->walkTick / 8) & 1; }
            break;
    }
    SPR_setAnimAndFrame(f->spr, anim, frame);
    SPR_setHFlip(f->spr, dirTo(i) < 0);
    SPR_setPosition(f->spr, f->x - 16, STAND_Y);
}

/* ------------------------------------------------------------------ sol */
static bool canAdvance(u16 who) { return (who == top) ? (pos != POS_BACK) : TRUE; }
static bool canSub(u16 who)
{
    if (who == top) return pos != POS_HALF;
    return pos == POS_GUARD;
}

static const char* advName(u16 who)
{
    if (who == top)
    {
        switch (pos)
        {
            case POS_GUARD: return "OUVRIR LA GARDE";
            case POS_HALF:  return "PASSER LA GARDE";
            case POS_SIDE:  return "MONTER";
            case POS_MOUNT: return "PRENDRE LE DOS";
            default:        return "-";
        }
    }
    switch (pos)
    {
        case POS_GUARD: return "BALAYER";
        case POS_HALF:  return "REPRENDRE LA GARDE";
        case POS_SIDE:  return "S'ECHAPPER";
        case POS_MOUNT: return "UPA (RENVERSER)";
        default:        return "SE RETOURNER";
    }
}

static const char* subName(u16 who)
{
    if (who == top)
    {
        switch (pos)
        {
            case POS_GUARD: return "CLE DE CHEVILLE";
            case POS_SIDE:  return "KIMURA";
            case POS_MOUNT: return "CLE DE BRAS";
            case POS_BACK:  return "ETRANGLEMENT";
            default:        return "-";
        }
    }
    return (pos == POS_GUARD) ? "TRIANGLE" : "-";
}

/* force de l'attaquant (en %) selon l'action et la position */
static s16 movePower(u16 who, u16 move)
{
    static const s16 subTop[5] = { 55, 0, 70, 80, 95 };
    static const s16 escBottom[5] = { 85, 95, 80, 70, 70 };

    if (move == MOVE_SUB) return (who == top) ? subTop[pos] : 75;
    return (who == top) ? 100 : escBottom[pos];
}

static void drawGroundInfo(void)
{
    char line[40];
    const char* role = (top == P_PLAYER) ? "TOI DESSUS - " : "TOI DESSOUS - ";
    u16 n = 0, i;

    for (i = 0; role[i]; i++) line[n++] = role[i];
    for (i = 0; posName[pos][i]; i++) line[n++] = posName[pos][i];
    line[n] = 0;
    textCenter(line, ROW_POS);

    VDP_clearTextLine(ROW_GAUGE);
    VDP_clearTextLine(ROW_HINT1);
    VDP_clearTextLine(ROW_HINT2);
    if (canAdvance(P_PLAYER)) { textDark("A:", 4, ROW_HINT1); textDark(advName(P_PLAYER), 7, ROW_HINT1); }
    if (canSub(P_PLAYER))     { textDark("B:", 4, ROW_HINT2); textDark(subName(P_PLAYER), 7, ROW_HINT2); }
}

static void goGround(u16 p, u16 newTop)
{
    mode = MODE_GROUND;
    pos = p;
    top = newTop;
    sActive = FALSE;
    groundCooldown = 30;
    cpuThink = 90;

    showStanding(FALSE);
    setGroundPalette();
    SPR_setAnimAndFrame(groundSpr, pos, 0);
    SPR_setVisibility(groundSpr, VISIBLE);
    drawGroundInfo();
}

static void endMatch(s16 winner, bool bySub);

static void startStruggle(u16 who, u16 move)
{
    sActive = TRUE;
    sWho = who;
    sMove = move;
    sGauge = 0;
    sTime = (move == MOVE_SUB) ? 240 : 180;
    addStamina(who, (move == MOVE_SUB) ? -12 : -6);

    VDP_clearTextLine(ROW_HINT1);
    VDP_clearTextLine(ROW_HINT2);
    textCenter((move == MOVE_SUB) ? subName(who) : advName(who), ROW_HINT1);
    textCenter((who == P_PLAYER) ? "MARTELE A / B / C !" : "DEFENDS-TOI : MARTELE A / B / C !", ROW_HINT2);
    sfx((move == MOVE_SUB) ? 880 : 440, 8);
}

static void struggleSuccess(void)
{
    u16 who = sWho;

    if (sMove == MOVE_SUB)
    {
        endMatch(who, TRUE);
        return;
    }

    sfxSlam();
    crowdReact(FALSE);
    if (who == top)
    {
        switch (pos)
        {
            case POS_GUARD: pos = POS_HALF;  F[who].adv++;      messageWho(who, "GARDE OUVERTE ! AVANTAGE");  break;
            case POS_HALF:  pos = POS_SIDE;  F[who].points += 3; messageWho(who, "PASSAGE DE GARDE ! +3");    break;
            case POS_SIDE:  pos = POS_MOUNT; F[who].points += 4; messageWho(who, "MONTEE ! +4");              break;
            case POS_MOUNT: pos = POS_BACK;  F[who].points += 4; messageWho(who, "PRISE DE DOS ! +4");        break;
        }
    }
    else
    {
        switch (pos)
        {
            case POS_GUARD: top = who; pos = POS_HALF; F[who].points += 2; messageWho(who, "BALAYAGE ! +2");  break;
            case POS_HALF:  pos = POS_GUARD; messageWho(who, "GARDE RECUPEREE");                              break;
            case POS_SIDE:  pos = POS_HALF;  messageWho(who, "SORTIE EN DEMI-GARDE");                         break;
            case POS_MOUNT: top = who; pos = POS_GUARD; messageWho(who, "UPA ! RENVERSEMENT");                break;
            case POS_BACK:  top = who; pos = POS_GUARD; messageWho(who, "SE RETOURNE DANS LA GARDE");      break;
        }
    }
    setGroundPalette();
    SPR_setAnimAndFrame(groundSpr, pos, 0);
}

static void struggleEnd(void)
{
    sActive = FALSE;
    groundCooldown = 20;
    cpuThink = 50 + (random() % 80) - difficulty * 15;
    drawGroundInfo();
}

static void pushGauge(u16 i, s16 p)
{
    if (i == sWho) sGauge += p * movePower(sWho, sMove) / 100;
    else sGauge -= p;
}

static void updateStruggle(void)
{
    /* joueur : chaque nouvel appui pousse */
    if (pressed(BUTTON_A) || pressed(BUTTON_B) || pressed(BUTTON_C))
    {
        pushGauge(P_PLAYER, 20 + F[0].stamina / 4);
        if (frameCount & 1) addStamina(P_PLAYER, -1);
        sfx(300 + (frameCount & 7) * 20, 2);
    }
    /* CPU : appuis au hasard, plus frequents selon le niveau */
    if ((random() & 0xFF) < cpuPressRate[difficulty])
    {
        pushGauge(P_CPU, 20 + F[1].stamina / 4);
        if (frameCount & 1) addStamina(P_CPU, -1);
    }

    /* effet boule de neige : celui qui prend le dessus accelere */
    sGauge += sGauge / 24;

    if ((frameCount & 3) == 0) drawGauge();
    SPR_setFrame(groundSpr, (frameCount / 6) & 1);

    sTime--;
    if (sGauge >= GAUGE_MAX)
    {
        struggleSuccess();
        if (mode == MODE_GROUND) struggleEnd();
    }
    else if (sGauge <= -GAUGE_MAX)
    {
        if (sMove == MOVE_SUB) addStamina(sWho, -10);
        message((sWho == P_PLAYER) ? "LE CPU SE DEFEND" : "BIEN DEFENDU !");
        sfx(150, 10);
        struggleEnd();
    }
    else if (sTime <= 0)
    {
        if (sGauge >= GAUGE_MAX / 2)
        {
            F[sWho].adv++;
            messageWho(sWho, "PRESQUE ! AVANTAGE");
        }
        else message("BLOQUE");
        struggleEnd();
    }
}

static void cpuGroundThink(void)
{
    s16 subChance;

    if (--cpuThink > 0) return;
    cpuThink = 40;

    subChance = 15 + (100 - F[0].stamina) / 3 + difficulty * 5;
    if (canSub(P_CPU) && (s16)(random() % 100) < subChance) startStruggle(P_CPU, MOVE_SUB);
    else if (canAdvance(P_CPU)) startStruggle(P_CPU, MOVE_ADVANCE);
}

static void updateGround(void)
{
    if (sActive)
    {
        updateStruggle();
        return;
    }

    SPR_setFrame(groundSpr, 0);

    /* recuperation : celui du dessous respire mal sous la pression */
    if ((frameCount % 5) == 0) addStamina(top, 1);
    if ((frameCount % ((pos >= POS_SIDE) ? 14 : 6)) == 0) addStamina(1 - top, 1);

    if (groundCooldown > 0) { groundCooldown--; return; }

    if (pressed(BUTTON_A) && canAdvance(P_PLAYER)) { startStruggle(P_PLAYER, MOVE_ADVANCE); return; }
    if (pressed(BUTTON_B) && canSub(P_PLAYER))     { startStruggle(P_PLAYER, MOVE_SUB);     return; }

    cpuGroundThink();
}

/* ------------------------------------------------------------------ debout */
static void startShoot(u16 i)
{
    F[i].act = ACT_SHOOT;
    F[i].actTimer = 0;
    addStamina(i, -8);
    sfx(220, 6);

    if (i == P_PLAYER && F[1].act == ACT_IDLE && (s16)(random() % 100) < cpuSprawlPct[difficulty])
        cpuSprawlDelay = 3 + (random() % 6);
}

static void startSprawl(u16 i)
{
    F[i].act = ACT_SPRAWL;
    F[i].actTimer = 0;
    addStamina(i, -4);
}

static void stumble(u16 i, s16 frames)
{
    F[i].act = ACT_STUMBLE;
    F[i].actTimer = 0;
    F[i].cooldown = frames;
}

static void resolveShoot(u16 att)
{
    u16 def = 1 - att;
    s16 chance;

    if (dist() > REACH_SHOOT)
    {
        messageWho(att, "TROP LOIN !");
        stumble(att, 16);
        return;
    }
    if (F[def].act == ACT_SPRAWL)
    {
        messageWho(def, "SPRAWL ! PROJECTION BLOQUEE");
        addStamina(att, -10);
        stumble(att, 30);
        sfx(150, 8);
        return;
    }

    chance = 55 + (F[att].stamina - F[def].stamina) / 3;
    if (att == P_CPU) chance += cpuShootBonus[difficulty];

    if ((s16)(random() % 100) < chance)
    {
        F[att].points += 2;
        sfxSlam();
        crowdReact(FALSE);
        messageWho(att, "QUEDA ! PROJECTION +2");
        goGround(POS_GUARD, att);
    }
    else
    {
        messageWho(att, "PROJECTION RATEE");
        stumble(att, 20);
    }
}

static void tryPullGuard(u16 i)
{
    if (dist() > REACH_PULL) return;
    message((i == P_PLAYER) ? "TU TIRES LA GARDE" : "LE CPU TIRE LA GARDE");
    goGround(POS_GUARD, 1 - i);
}

/* avance un combattant de dx en gardant l'ecart minimum */
static void moveFighter(u16 i, s16 dx, s16 gap)
{
    s16 nx = F[i].x + dx;
    s16 ox = F[1 - i].x;

    if (nx < MIN_X) nx = MIN_X;
    if (nx > MAX_X) nx = MAX_X;
    if (iabs(nx - ox) < gap && iabs(nx - ox) < iabs(F[i].x - ox)) return;
    if ((F[i].x < ox) != (nx < ox)) return;     /* on ne se traverse pas */
    F[i].x = nx;
    if (dx) { F[i].walking = TRUE; F[i].walkTick++; }
}

/* fait avancer l'action en cours ; renvoie TRUE si le combattant est occupe */
static bool updateAction(u16 i)
{
    Fighter* f = &F[i];

    if (f->act == ACT_SHOOT)
    {
        f->actTimer++;
        if (f->actTimer >= 12 && f->actTimer < 18) moveFighter(i, dirTo(i) * 2, 14);
        if (f->actTimer == 18)
        {
            resolveShoot(i);
            if (mode != MODE_STAND) return TRUE;
            if (f->act == ACT_SHOOT) { f->act = ACT_IDLE; f->cooldown = 16; }
        }
        return TRUE;
    }
    if (f->act == ACT_SPRAWL)
    {
        if (++f->actTimer >= 24) { f->act = ACT_IDLE; f->cooldown = 20; }
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

static void cpuStand(void)
{
    s16 d = dist();
    s16 r;

    if (cpuSprawlDelay > 0)
    {
        if (--cpuSprawlDelay == 0) startSprawl(P_CPU);
        return;
    }

    if (--cpuMoveTimer <= 0)
    {
        r = random() % 100;
        cpuMoveTimer = 20 + (random() % 30) - difficulty * 5;

        if (d <= REACH_SHOOT && r < 30 + difficulty * 10) { startShoot(P_CPU); return; }
        if (d <= REACH_PULL && r >= 90 && F[1].stamina < 50) { tryPullGuard(P_CPU); return; }

        if (d > REACH_SHOOT - 6) cpuMoveDir = (r < 80) ? 1 : 0;
        else cpuMoveDir = (r < 50) ? -1 : 0;
    }
    if (cpuMoveDir) moveFighter(P_CPU, dirTo(P_CPU) * cpuMoveDir, MIN_GAP);
}

static void updateStand(void)
{
    u16 i;

    F[0].walking = F[1].walking = FALSE;

    if (!updateAction(P_PLAYER) && mode == MODE_STAND)
    {
        if (pad & BUTTON_LEFT)  moveFighter(P_PLAYER, -1, MIN_GAP);
        if (pad & BUTTON_RIGHT) moveFighter(P_PLAYER,  1, MIN_GAP);
        if (pressed(BUTTON_A))      startShoot(P_PLAYER);
        else if (pressed(BUTTON_B)) tryPullGuard(P_PLAYER);
        else if (pressed(BUTTON_C)) startSprawl(P_PLAYER);
    }
    if (mode != MODE_STAND) return;

    if (!updateAction(P_CPU) && mode == MODE_STAND) cpuStand();
    if (mode != MODE_STAND) return;

    for (i = 0; i < 2; i++)
    {
        if (F[i].act == ACT_IDLE && (frameCount % 6) == 0) addStamina(i, 1);
        updateFighterSprite(i);
    }
}

static void standStart(void)
{
    u16 i;

    mode = MODE_STAND;
    F[0].x = 116;
    F[1].x = 204;
    for (i = 0; i < 2; i++)
    {
        F[i].act = ACT_IDLE;
        F[i].actTimer = 0;
        F[i].cooldown = 30;
        F[i].walking = FALSE;
        updateFighterSprite(i);
    }
    cpuMoveTimer = 40;
    cpuMoveDir = 0;
    cpuSprawlDelay = 0;

    SPR_setVisibility(groundSpr, HIDDEN);
    showStanding(TRUE);

    VDP_clearTextLine(ROW_POS);
    VDP_clearTextLine(ROW_GAUGE);
    textCenter("A PROJETER  B TIRER LA GARDE  C SPRAWL", ROW_HINT1);
    VDP_clearTextLine(ROW_HINT2);
}

/* ------------------------------------------------------------------ match */
static void startMatch(void)
{
    u16 i;

    setRandomSeed(frameCount);
    for (i = 0; i < 2; i++)
    {
        F[i].points = 0;
        F[i].adv = 0;
        F[i].stamina = 100;
    }
    matchFrames = MATCH_SECONDS * 60;

    VDP_clearTextArea(0, 8, 40, 20);
    standStart();
    drawHud();
    message("COMBATE !");
    SND_PCM_stopPlay();
    musicStage = 0;
    crowdStart();
    playTune(tuneStart, 3);
}

static void endMatch(s16 winner, bool bySub)
{
    mode = MODE_END;
    sActive = FALSE;
    msgTimer = 0;

    if (winner < 0)
    {
        if (F[0].points != F[1].points) winner = (F[0].points > F[1].points) ? P_PLAYER : P_CPU;
        else if (F[0].adv != F[1].adv)  winner = (F[0].adv > F[1].adv) ? P_PLAYER : P_CPU;
    }

    VDP_clearTextArea(0, 8, 40, 9);
    if (bySub)
    {
        textCenter((winner == P_PLAYER) ? "LE CPU TAPE ! FINALISATION !" : "TU TAPES... SOUMISSION", ROW_POS);
        textCenter((winner == P_PLAYER) ? "TU GAGNES !" : "LE CPU GAGNE", ROW_MSG);
    }
    else
    {
        textCenter("FIN DU TEMPS !", ROW_POS);
        if (winner < 0) textCenter("EGALITE PARFAITE", ROW_MSG);
        else textCenter((winner == P_PLAYER) ? "TU GAGNES AUX POINTS !" : "LE CPU GAGNE AUX POINTS", ROW_MSG);
    }
    textCenter("START : REJOUER", ROW_HINT2);

    crowdReact(TRUE);
    if (winner == P_PLAYER) playTune(tuneWin, 6);
    else playTune(tuneLose, 4);
    drawHud();
}

/* ------------------------------------------------------------------ titre */
static void drawDifficulty(void)
{
    static const char* const names[3] = { "<  FACILE   >", "<  NORMAL   >", "< DIFFICILE >" };
    textCenterLight(names[difficulty], 15);
}

static void titleStart(void)
{
    mode = MODE_TITLE;
    VDP_clearTextArea(0, 0, 40, 28);

    /* musique : l'intro une fois, puis la boucle (voir updateTitle) */
    crowdStop();
    SND_PCM_startPlay(title_intro, sizeof(title_intro), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, FALSE);
    musicStage = 1;
    textLight("BJJ TATAMI", 15, ROW_BANNER);

    textCenter("LE JEU DE JIU-JITSU BRESILIEN", 9);
    textCenter("DEBOUT : A PROJETER  B GARDE  C SPRAWL", 11);
    textCenter("AU SOL : A PROGRESSER  B SOUMISSION", 12);
    textCenter("LUTTE : MARTELE LES BOUTONS", 13);
    drawDifficulty();

    F[0].x = 116;
    F[1].x = 204;
    F[0].act = F[1].act = ACT_IDLE;
    F[0].walking = F[1].walking = FALSE;
    SPR_setVisibility(groundSpr, HIDDEN);
    showStanding(TRUE);
}

static void updateTitle(void)
{
    if (musicStage == 1 && !SND_PCM_isPlaying())
    {
        SND_PCM_startPlay(title_loop, sizeof(title_loop), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, TRUE);
        musicStage = 2;
    }

    if (pressed(BUTTON_LEFT) && difficulty > 0)  { difficulty--; drawDifficulty(); sfx(600, 3); }
    if (pressed(BUTTON_RIGHT) && difficulty < 2) { difficulty++; drawDifficulty(); sfx(600, 3); }

    if ((frameCount & 31) == 0) textCenterLight("APPUIE SUR START", 17);
    if ((frameCount & 31) == 16) VDP_clearTextLine(17);

    updateFighterSprite(0);
    updateFighterSprite(1);

    if (pressed(BUTTON_START))
    {
        VDP_clearTextLine(15);
        VDP_clearTextLine(17);
        startMatch();
    }
}

/* ------------------------------------------------------------------ main */
int main(bool hardReset)
{
    VDP_setScreenWidth320();
    SPR_init();
    PSG_reset();

    VDP_drawImageEx(BG_B, &bg_dojo, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, DMA);
    setupPalettes();

    F[0].spr = SPR_addSprite(&spr_fighter, 100, STAND_Y, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    F[1].spr = SPR_addSprite(&spr_fighter, 188, STAND_Y, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    groundSpr = SPR_addSprite(&spr_ground, GROUND_SPR_X, GROUND_SPR_Y, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
    SPR_setVisibility(groundSpr, HIDDEN);

    titleStart();

    while (TRUE)
    {
        padPrev = pad;
        pad = JOY_readJoypad(JOY_1);
        frameCount++;

        switch (mode)
        {
            case MODE_TITLE:
                updateTitle();
                break;

            case MODE_STAND:
            case MODE_GROUND:
                if (mode == MODE_STAND) updateStand();
                else updateGround();

                if (mode != MODE_END)
                {
                    if (--matchFrames <= 0) { matchFrames = 0; endMatch(-1, FALSE); }
                    else if ((frameCount & 7) == 0) drawHud();
                }
                if (msgTimer > 0 && --msgTimer == 0) VDP_clearTextLine(ROW_MSG);
                break;

            case MODE_END:
                if (pressed(BUTTON_START))
                {
                    titleStart();
                }
                break;
        }

        soundUpdate();
        crowdUpdate();
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
