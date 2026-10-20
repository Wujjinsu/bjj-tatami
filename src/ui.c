/*
 * Outils communs : textes, palettes, manettes, sons, musique, public, sauvegarde du nom.
 */
#include "game.h"

/* ------------------------------------------------------------------ chaines et textes */
u16 slen(const char* s)
{
    u16 n = 0;
    while (s[n]) n++;
    return n;
}

void scopy(char* dst, const char* src, u16 max)
{
    u16 i = 0;
    while (src[i] && i < max) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

void txt(const char* s, u16 x, u16 y, u16 pal)
{
    if (x >= 40 || y >= 28) return;
    VDP_setTextPalette(pal);
    VDP_drawText(s, x, y);
}

void txtCenter(const char* s, u16 y, u16 pal)
{
    u16 n = slen(s);
    VDP_clearTextLine(y);
    txt(s, (n >= 40) ? 0 : (40 - n) / 2, y, pal);
}

void clearRow(u16 y) { VDP_clearTextLine(y); }

void clearRows(u16 y0, u16 y1)
{
    u16 y;
    for (y = y0; y <= y1; y++) VDP_clearTextLine(y);
}

void numStr(s16 v, u16 digits, char* out)
{
    u16 i;
    if (v < 0) v = 0;
    for (i = 0; i < digits; i++)
    {
        out[digits - 1 - i] = '0' + (v % 10);
        v /= 10;
    }
    out[digits] = 0;
}

void drawNum(s16 v, u16 digits, u16 x, u16 y, u16 pal)
{
    char b[6];
    numStr(v, digits, b);
    txt(b, x, y, pal);
}

void statBar(u8 v, char* out)
{
    u16 i;
    for (i = 0; i < 9; i++) out[i] = (i < v) ? '#' : '-';
    out[9] = 0;
}

/* ------------------------------------------------------------------ decors et palettes */
#define C(rgb) RGB24_TO_VDPCOLOR(rgb)

static const u16 skinCol[3][2] = { { C(0xFCB490), C(0xD87E5A) }, { C(0xC68A5A), C(0x905A36) }, { C(0x7A4A2A), C(0x4A2A12) } };
static const u16 hairCol[3]    = { C(0x121212), C(0x5A3612), C(0xD8B030) };
static const u16 giCol[3][2]   = { { C(0xECECEC), C(0xA2AAC6) }, { C(0x245AD8), C(0x123690) }, { C(0x3A3A46), C(0x1E1E26) } };

#define COL_OUTLINE C(0x121224)
#define COL_BELT    C(0x101010)
#define COL_REDBAR  C(0xD82424)

static u16 palGround[16];

void showBackground(const Image* img)
{
    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawImageEx(BG_B, img, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, DMA);
}

void buildFighterPalette(u16* pal, const FighterDef* f, u8 gi, u16 textColor)
{
    u16 i;
    for (i = 0; i < 16; i++) pal[i] = 0;
    pal[1] = COL_OUTLINE;
    pal[2] = skinCol[f->skin][0];
    pal[3] = skinCol[f->skin][1];
    pal[4] = (f->hair == HAIR_BALD) ? skinCol[f->skin][1] : hairCol[f->hair];
    pal[5] = giCol[gi][0];
    pal[6] = giCol[gi][1];
    pal[7] = COL_BELT;
    pal[11] = COL_REDBAR;
    pal[15] = textColor;
}

void applyPlayerPalettes(void)
{
    buildFighterPalette(P[0].pal, P[0].def, P[0].gi, C(0x121236));   /* PAL1 : texte fonce */
    buildFighterPalette(P[1].pal, P[1].def, P[1].gi, C(0xC81818));   /* PAL2 : texte rouge */
    PAL_setPalette(PAL1, P[0].pal, CPU);
    PAL_setPalette(PAL2, P[1].pal, CPU);
    setGroundPalette(0);
}

/* sprite "ground" : groupe A (2,3,4 + 5,6,7) = dessus, groupe B (12,13,14 + 8,9,10) = dessous */
void setGroundPalette(u16 top)
{
    const u16* a = P[top].pal;
    const u16* b = P[1 - top].pal;
    u16 i;

    for (i = 0; i < 16; i++) palGround[i] = 0;
    palGround[1] = COL_OUTLINE;
    palGround[2] = a[2]; palGround[3] = a[3]; palGround[4] = a[4];
    palGround[5] = a[5]; palGround[6] = a[6]; palGround[7] = a[7];
    palGround[12] = b[2]; palGround[13] = b[3]; palGround[14] = b[4];
    palGround[8] = b[5]; palGround[9] = b[6]; palGround[10] = b[7];
    palGround[11] = COL_REDBAR;
    palGround[15] = C(0x0A6A0A);                                     /* PAL3 : texte vert */
    PAL_setPalette(PAL3, palGround, CPU);
}

void hideAllSprites(void)
{
    SPR_setVisibility(P[0].spr, HIDDEN);
    SPR_setVisibility(P[1].spr, HIDDEN);
    SPR_setVisibility(groundSpr, HIDDEN);
}

/* ------------------------------------------------------------------ manettes */
void readPads(void)
{
    u16 i;
    for (i = 0; i < 2; i++)
    {
        P[i].prev = P[i].pad;
        P[i].pad = P[i].human ? JOY_readJoypad(P[i].joy) : 0;
    }
}

static u16 mPad, mPrev;
void readMenuPad(void)
{
    mPrev = mPad;
    mPad = JOY_readJoypad(JOY_1);
}
bool mpressed(u16 b) { return (mPad & b) && !(mPrev & b); }
bool mheld(u16 b)    { return (mPad & b) != 0; }

bool pressed(u16 i, u16 b) { return (P[i].pad & b) && !(P[i].prev & b); }
bool held(u16 i, u16 b)    { return (P[i].pad & b) != 0; }

u16 dirOf(u16 i)
{
    u16 p = P[i].pad;
    if (p & BUTTON_DOWN) return DIR_DOWN;
    if (p & (BUTTON_LEFT | BUTTON_RIGHT)) return DIR_SIDE;
    return DIR_UP;                      /* haut ou neutre */
}

/* ------------------------------------------------------------------ sons PSG */
const u16 tuneWin[6]   = { 523, 659, 784, 1046, 0, 1046 };
const u16 tuneLose[4]  = { 392, 330, 262, 196 };
const u16 tuneStart[3] = { 784, 0, 784 };

static s16 sfxTimer;
static const u16* tunePtr;
static u16 tuneLen, tuneIdx;
static s16 tuneTimer;

void sfx(u16 freq, s16 frames)
{
    PSG_setFrequency(0, freq);
    PSG_setEnvelope(0, 2);
    sfxTimer = frames;
}

void sfxSlam(void)
{
    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK2);
    PSG_setEnvelope(3, 2);
    sfxTimer = 10;
}

void playTune(const u16* t, u16 len)
{
    tunePtr = t;
    tuneLen = len;
    tuneIdx = 0;
    tuneTimer = 0;
}

void soundUpdate(void)
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
                if (f) { PSG_setFrequency(1, f); PSG_setEnvelope(1, 3); }
                else PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
                tuneTimer = 9;
            }
        }
        tuneTimer--;
    }
    if (sfxTimer > 0 && --sfxTimer == 0)
    {
        PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
        PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
    }
}

/* ------------------------------------------------------------------ musique (driver PCM) */
static u16 musicStage;      /* 0 rien, 1 intro, 2 boucle */

void musicTitleStart(void)
{
    if (musicStage) return;                 /* deja en cours : on ne la relance pas */
    crowdStop();
    SND_PCM_startPlay(title_intro, sizeof(title_intro), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, FALSE);
    musicStage = 1;
}

void musicUpdate(void)
{
    if (musicStage == 1 && !SND_PCM_isPlaying())
    {
        SND_PCM_startPlay(title_loop, sizeof(title_loop), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, TRUE);
        musicStage = 2;
    }
}

void musicStop(void)
{
    if (!musicStage) return;
    SND_PCM_stopPlay();
    musicStage = 0;
}

/* ------------------------------------------------------------------ public (driver PCM4) */
static bool crowdOn;
static s16 crowdBoost;
static u8 crowdVol;

void crowdStart(void)
{
    musicStop();
    SND_PCM4_startPlay(crowd_loop, sizeof(crowd_loop), SOUND_PCM_CH1, TRUE);
    crowdVol = 6;
    SND_PCM4_setVolume(SOUND_PCM_CH1, crowdVol);
    crowdBoost = 0;
    crowdOn = TRUE;
}

void crowdStop(void)
{
    if (!crowdOn) return;
    SND_PCM4_stopPlay(SOUND_PCM_CH1);
    SND_PCM4_stopPlay(SOUND_PCM_CH2);
    crowdOn = FALSE;
}

void crowdReact(bool big)
{
    if (!crowdOn) return;
    if (big) SND_PCM4_startPlay(cheer_big, sizeof(cheer_big), SOUND_PCM_CH2, FALSE);
    else SND_PCM4_startPlay(cheer_small, sizeof(cheer_small), SOUND_PCM_CH2, FALSE);
    SND_PCM4_setVolume(SOUND_PCM_CH2, 15);
    crowdBoost = big ? 240 : 90;
}

void crowdUpdate(bool tense)
{
    u8 v = 6;
    if (!crowdOn) return;
    if (crowdBoost > 0) crowdBoost--;
    if (tense) v += 3;
    if (crowdBoost > 0) v += 4;
    if (v > 15) v = 15;
    if (v != crowdVol)
    {
        crowdVol = v;
        SND_PCM4_setVolume(SOUND_PCM_CH1, v);
    }
}

/* ------------------------------------------------------------------ nom du joueur 1 en SRAM */
#define SRAM_MAGIC0 'B'
#define SRAM_MAGIC1 'J'
#define SRAM_VER    1

void nameLoad(void)
{
    u16 i;
    u8 sum = 0, c;
    char tmp[NAME_LEN + 1];
    bool ok;

    SRAM_enableRO();
    ok = SRAM_readByte(0) == SRAM_MAGIC0 && SRAM_readByte(1) == SRAM_MAGIC1 && SRAM_readByte(2) == SRAM_VER;
    for (i = 0; i < NAME_LEN; i++)
    {
        c = SRAM_readByte(3 + i);
        tmp[i] = c;
        sum += c;
    }
    ok = ok && (SRAM_readByte(3 + NAME_LEN) == sum);
    SRAM_disable();

    tmp[NAME_LEN] = 0;
    for (i = 0; i < NAME_LEN && ok; i++)
        if (tmp[i] != 0 && (tmp[i] < 32 || tmp[i] > 95)) ok = FALSE;
    if (ok && tmp[0] != 0) scopy(playerName, tmp, NAME_LEN);
    else scopy(playerName, "JOUEUR 1", NAME_LEN);
}

void nameSave(void)
{
    u16 i;
    u8 sum = 0, c;

    SRAM_enable();
    SRAM_writeByte(0, SRAM_MAGIC0);
    SRAM_writeByte(1, SRAM_MAGIC1);
    SRAM_writeByte(2, SRAM_VER);
    for (i = 0; i < NAME_LEN; i++)
    {
        c = (i < slen(playerName)) ? (u8)playerName[i] : 0;
        SRAM_writeByte(3 + i, c);
        sum += c;
    }
    SRAM_writeByte(3 + NAME_LEN, sum);
    SRAM_disable();
}
