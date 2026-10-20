/*
 * HUD facon borne d'arcade : drapeaux, noms, chrono geant, jauges de souffle,
 * tableau de score IBJJF (points / avantages / penalites), messages, aides, mini-jeu.
 */
#include "game.h"

#define ROW_MSG     5
#define TUG_BASE    1040                /* jauge du mini-jeu (20 tuiles dynamiques) */
#define TUG_TILES   20
#define BAR_TILES   16
#define BAR_PX      (BAR_TILES * 8)

static u32 barData[2][BAR_TILES][8];
static s16 lastS[2] = { -1, -1 }, lastC[2] = { -1, -1 }, lastBlink = -1;
static s16 lastPts[2] = { -1, -1 }, lastAdv[2] = { -1, -1 }, lastPen[2] = { -1, -1 }, lastGrip[2] = { -1, -1 };
static s16 lastSecs = -1;
static s16 msgTimer;

enum { K = 1, WH = 2, LG = 3, DG = 4, RD = 5, DR = 6, GR = 7, DGR = 8, YE = 9, OR = 10, BL = 11, DBL = 12, NV = 15 };

static void bigDigit(u16 base, u16 ch, u16 x, u16 y)
{
    putTile(x, y, base + ch * 2);
    putTile(x, y + 1, base + ch * 2 + 1);
}

static void buildBar(u16 p)
{
    s16 v = (P[p].souffle * BAR_PX) / 100;
    s16 c = (P[p].cardio * BAR_PX) / 100;
    bool low = P[p].souffle < 30;
    bool blink = low && ((frameCount >> 3) & 1);
    u16 t, row, px;

    for (t = 0; t < BAR_TILES; t++)
    {
        for (row = 0; row < 8; row++)
        {
            u32 w = 0;
            for (px = 0; px < 8; px++)
            {
                s16 x = t * 8 + px;
                s16 xx = (p == 0) ? x : (BAR_PX - 1 - x);
                u16 col;
                if (row == 0 || row == 7 || x == 0 || x == BAR_PX - 1) col = K;
                else if (xx < v)
                {
                    if (low) col = blink ? WH : ((row < 3) ? OR : RD);
                    else col = (row == 1) ? WH : ((row < 5) ? YE : OR);
                }
                else if (xx < c) col = ((x + row) & 1) ? DR : K;
                else col = NV;
                w = (w << 4) | col;
            }
            barData[p][t][row] = w;
        }
    }
    VDP_loadTileData((const u32*)barData[p], BAR_BASE + p * BAR_TILES, BAR_TILES, DMA_QUEUE);
}

static void drawScore(u16 p)
{
    u16 x = (p == 0) ? 1 : 37;
    u16 ax = (p == 0) ? 4 : 34;
    s16 pts = P[p].points > 99 ? 99 : P[p].points;
    bigDigit(HT_BIG_PTS, pts / 10, x, 3);
    bigDigit(HT_BIG_PTS, pts % 10, x + 1, 3);
    putTile(ax, 3, HT_ADV_LAB);
    putTile(ax + 1, 3, HT_ADV_DIG + (P[p].adv > 9 ? 9 : P[p].adv));
    putTile(ax, 4, HT_PEN_LAB);
    putTile(ax + 1, 4, HT_PEN_DIG + (P[p].pen > 9 ? 9 : P[p].pen));
}

void hudInit(void)
{
    u16 p, t;
    char b[20];
    VDP_clearPlane(BG_A, TRUE);
    for (p = 0; p < 2; p++)
    {
        u16 fx = (p == 0) ? 1 : 37;
        u16 f = HT_FLAG_BRA + P[p].def->flag * 4;
        putTile(fx, 0, f);
        putTile(fx + 1, 0, f + 1);
        putTile(fx, 1, f + 2);
        putTile(fx + 1, 1, f + 3);
        if (p == 0)
        {
            gtxt(P[0].label, 4, 0);
            wtxt(P[0].def->name, 4, 1);
        }
        else
        {
            gtxt(P[1].label, 36 - slen(P[1].label), 0);
            wtxt(P[1].def->name, 36 - slen(P[1].def->name), 1);
        }
        for (t = 0; t < BAR_TILES; t++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, BAR_BASE + p * BAR_TILES + t),
                             (p == 0 ? 1 : 23) + t, 2);
        lastS[p] = lastC[p] = lastPts[p] = lastAdv[p] = lastPen[p] = lastGrip[p] = -1;
    }
    /* cadre du chrono */
    putTile(17, 0, HT_BOX_TIME_TOP);
    putTile(17, 1, HT_BOX_TIME_BOT);
    putTile(22, 0, HT_BOX_TIME_TOP);
    putTile(22, 1, HT_BOX_TIME_BOT);
    scopy(b, roster[fighterSel[0] / FIGHTERS_PER_CAT].name, 19);
    wtxt(b, 20 - slen(b) / 2, 3);
    lastSecs = -1;
    msgTimer = 0;
}

void hudDraw(s32 framesLeft, bool full)
{
    u16 p;
    s16 secs = (framesLeft + 59) / 60;
    if (full) lastSecs = -1;
    if (secs != lastSecs)
    {
        s16 m = secs / 60;
        s16 s = secs % 60;
        bigDigit(HT_BIG_TIME, m > 9 ? 9 : m, 18, 0);
        bigDigit(HT_BIG_TIME, 10, 19, 0);
        bigDigit(HT_BIG_TIME, s / 10, 20, 0);
        bigDigit(HT_BIG_TIME, s % 10, 21, 0);
        lastSecs = secs;
    }
    for (p = 0; p < 2; p++)
    {
        s16 blink = (P[p].souffle < 30) ? ((frameCount >> 3) & 1) : 0;
        if (full || P[p].souffle != lastS[p] || P[p].cardio != lastC[p] || (P[p].souffle < 30 && blink != lastBlink))
        {
            buildBar(p);
            lastS[p] = P[p].souffle;
            lastC[p] = P[p].cardio;
        }
        if (full || P[p].points != lastPts[p] || P[p].adv != lastAdv[p] || P[p].pen != lastPen[p])
        {
            drawScore(p);
            lastPts[p] = P[p].points;
            lastAdv[p] = P[p].adv;
            lastPen[p] = P[p].pen;
        }
        if (full || P[p].grip / 20 != lastGrip[p])
        {
            u16 k, n = P[p].grip / 20;
            for (k = 0; k < 5; k++)
            {
                u16 x = (p == 0) ? 7 + k : 32 - k;
                if (P[p].grip > 0) putTile(x, 4, k < n ? HT_SUB_ATT : HT_SUB_EMPTY);
                else VDP_setTileMapXY(BG_A, 0, x, 4);
            }
            lastGrip[p] = n;
        }
    }
    lastBlink = (frameCount >> 3) & 1;
}

/* ------------------------------------------------------------------ messages */
void hudMessage(const char* s, u16 frames)
{
    gtxtC(s, ROW_MSG);
    msgTimer = frames;
}

void hudMessageWho(u16 who, const char* s, u16 pts)
{
    char b[41];
    b[0] = 0;
    scat(b, P[who].label, 40);
    scat(b, " : ", 40);
    scat(b, s, 40);
    if (pts)
    {
        char n[4];
        numStr(pts, pts >= 10 ? 2 : 1, n);
        scat(b, " +", 40);
        scat(b, n, 40);
    }
    hudMessage(b, 120);
}

void hudUpdate(void)
{
    if (msgTimer > 0 && --msgTimer == 0) clearRow(ROW_MSG);
}

/* ------------------------------------------------------------------ aides (bas de l'ecran) */
void hudHints(const char* const* lines, u16 n)
{
    u16 i;
    fillPanel(0, 25, 40, 3);
    for (i = 0; i < 40; i++) putTile(i, 24, HT_PANEL_TOP);
    for (i = 0; i < n && i < 3; i++)
    {
        const char* s = lines[i];
        u16 x = 1, k = 0;
        char seg[41];
        /* '@A' '@B' '@C' = icones des boutons */
        while (*s && x < 40)
        {
            if (*s == '@' && s[1])
            {
                if (k) { seg[k] = 0; wtxt(seg, x - k, 25 + i); k = 0; }
                putTile(x, 25 + i, s[1] == 'A' ? HT_BTN_A : (s[1] == 'B' ? HT_BTN_B : HT_BTN_C));
                x++;
                s += 2;
                continue;
            }
            seg[k++] = *s++;
            x++;
        }
        if (k) { seg[k] = 0; wtxt(seg, x - k, 25 + i); }
    }
}

void hudClearHints(void)
{
    VDP_clearTextArea(0, 24, 40, 4);
}

static u32 tugData[TUG_TILES][8];

static void buildTug(s16 tug)
{
    const s16 W = TUG_TILES * 8, mid = W / 2;
    s16 pos = mid + (tug * (mid - 3)) / 100;
    bool hot = (frameCount >> 2) & 1;
    u16 t, row, px;
    for (t = 0; t < TUG_TILES; t++)
        for (row = 0; row < 8; row++)
        {
            u32 w = 0;
            for (px = 0; px < 8; px++)
            {
                s16 x = t * 8 + px;
                u16 col;
                if (row == 0 || row == 7 || x == 0 || x == W - 1) col = K;
                else if (x >= pos - 1 && x <= pos + 1) col = (row < 2 || row > 5) ? K : WH;
                else if (x == mid) col = YE;
                else if (pos < mid && x > pos && x < mid) col = (row == 1) ? WH : ((row < 4) ? GR : DGR);
                else if (pos > mid && x < pos && x > mid)
                    col = (row == 1) ? (hot && tug > 70 ? WH : OR) : ((row < 4) ? RD : DR);
                else col = (x < mid) ? (((x + row) & 3) ? DGR : K) : (((x + row) & 3) ? DR : K);
                w = (w << 4) | col;
            }
            tugData[t][row] = w;
        }
    VDP_loadTileData((const u32*)tugData, TUG_BASE, TUG_TILES, DMA_QUEUE);
}

/* ------------------------------------------------------------------ mini-jeu de soumission */
void hudSubGauge(const char* name, u16 gate, s16 a0, s16 alen, s16 d0, s16 dlen, s16 tug, u16 cells)
{
    static u16 row[40];
    u16 k;
    s16 idx;
    char b[41];

    b[0] = 0;
    scat(b, name, 40);
    gtxtC(b, 8);
    for (k = 0; k < 3; k++) putTile(17 + k * 2, 9, k < gate ? HT_GATE_ON : HT_GATE_OFF);
    for (k = 0; k < cells; k++)
    {
        bool ina = (s16)k >= a0 && (s16)k < a0 + alen;
        bool ind = (s16)k >= d0 && (s16)k < d0 + dlen;
        u16 t = ina ? (ind ? HT_SUB_BOTH : HT_SUB_ATT) : (ind ? HT_SUB_DEF : HT_SUB_EMPTY);
        row[k] = TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, HUD_BASE + t);
    }
    VDP_setTileMapDataRow(BG_A, row, 10, (40 - cells) / 2, cells, CPU);
    /* jauge de la porte : ECHAPPE [=====|=====] TAPE, tuiles dessinees a la volee */
    buildTug(tug);
    for (k = 0; k < TUG_TILES; k++)
        row[k] = TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, TUG_BASE + k);
    VDP_setTileMapDataRow(BG_A, row, 11, 10, TUG_TILES, CPU);
    wtxt("ECHAPPE", 2, 11);
    wtxt("TAPE", 31, 11);
}

void hudSubClear(void)
{
    clearRows(8, 11);
}
