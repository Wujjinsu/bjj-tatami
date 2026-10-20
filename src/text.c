/*
 * Texte (police doree = police systeme remplacee, police blanche = tuiles a part),
 * tuiles du HUD et manettes.
 */
#include "game.h"

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

void scat(char* dst, const char* src, u16 max)
{
    u16 n = slen(dst);
    u16 i = 0;
    while (src[i] && n < max) dst[n++] = src[i++];
    dst[n] = 0;
}

void numStr(s32 v, u16 digits, char* out)
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

void gtxt(const char* s, u16 x, u16 y)
{
    if (x >= 40 || y >= 28) return;
    VDP_drawText(s, x, y);
}

static u16 rowBuf[40];

void wtxt(const char* s, u16 x, u16 y)
{
    u16 n = 0;
    if (x >= 40 || y >= 28) return;
    while (s[n] && x + n < 40)
    {
        u16 c = (u8)s[n];
        if (c < 32 || c > 127) c = '?';
        rowBuf[n] = TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, WFONT_BASE + c - 32);
        n++;
    }
    if (n) VDP_setTileMapDataRow(BG_A, rowBuf, y, x, n, CPU);
}

void gtxtC(const char* s, u16 y)
{
    u16 n = slen(s);
    VDP_clearTextLine(y);
    gtxt(s, (n >= 40) ? 0 : (40 - n) / 2, y);
}

void wtxtC(const char* s, u16 y)
{
    u16 n = slen(s);
    VDP_clearTextLine(y);
    wtxt(s, (n >= 40) ? 0 : (40 - n) / 2, y);
}

void clearRow(u16 y) { VDP_clearTextLine(y); }

void clearRows(u16 y0, u16 y1)
{
    u16 y;
    for (y = y0; y <= y1 && y < 28; y++) VDP_clearTextLine(y);
}

void putTile(u16 x, u16 y, u16 tile)
{
    if (x >= 40 || y >= 28) return;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, HUD_BASE + tile), x, y);
}

void fillPanel(u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, HUD_BASE + HT_PANEL), x, y, w, h);
}

/* ------------------------------------------------------------------ manettes */
u16 padStyle = PAD_XBOX;

/*
 * Avec RetroArch + Genesis Plus GX (reglages par defaut), une manette Xbox donne :
 *   Xbox A -> MD B,  Xbox B -> MD C,  Xbox X -> MD A,  Xbox Y -> MD Y (pad 6 boutons), LB -> MD X, RB -> MD Z.
 * On remet les boutons a leur place "Xbox" :
 *   menus  : Xbox A = valider (A), Xbox B = retour (B)
 *   combat : Xbox A = attaque (A), Xbox X = prises/soumission (B), Xbox B = defense (C), Y/LB/RB = super
 */
#define ABC (BUTTON_A | BUTTON_B | BUTTON_C)

u16 padMenu(u16 raw)
{
    u16 o;
    if (padStyle != PAD_XBOX) return raw;
    o = raw & ~ABC;
    if (raw & BUTTON_B) o |= BUTTON_A;
    if (raw & BUTTON_C) o |= BUTTON_B;
    if (raw & BUTTON_A) o |= BUTTON_C;
    return o;
}

u16 padGame(u16 raw)
{
    u16 o;
    if (padStyle != PAD_XBOX) return raw;
    o = raw & ~ABC;
    if (raw & BUTTON_B) o |= BUTTON_A;
    if (raw & BUTTON_A) o |= BUTTON_B;
    if (raw & BUTTON_C) o |= BUTTON_C;
    return o;
}

u16 iconTile(char role)
{
    if (padStyle == PAD_XBOX)
        return role == 'A' ? HT_XB_A : (role == 'B' ? HT_XB_X : (role == 'C' ? HT_XB_B : HT_XB_Y));
    return role == 'A' ? HT_BTN_A : (role == 'B' ? HT_BTN_B : HT_BTN_C);
}
void readPads(void)
{
    u16 i;
    for (i = 0; i < 2; i++)
    {
        P[i].prev = P[i].pad;
        P[i].pad = P[i].human ? padGame(JOY_readJoypad(P[i].joy)) : 0;
    }
}

bool pressed(u16 i, u16 b) { return (P[i].pad & b) && !(P[i].prev & b); }
bool held(u16 i, u16 b)    { return (P[i].pad & b) != 0; }

static u16 mPad, mPrev;

void readMenuPad(void)
{
    mPrev = mPad;
    mPad = padMenu(JOY_readJoypad(JOY_1) | JOY_readJoypad(JOY_2));
}

bool mpressed(u16 b) { return (mPad & b) && !(mPrev & b); }
bool mheld(u16 b)    { return (mPad & b) != 0; }
bool anyMenuKey(void) { return mpressed(BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C); }
