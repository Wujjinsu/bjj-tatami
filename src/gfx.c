/*
 * Graphismes generaux : palettes (avec fondus), images d'ecran, portraits.
 */
#include "game.h"
#include "gen/pal3.h"

#define C(rgb) RGB24_TO_VDPCOLOR(rgb)

static u16 curPal[64];
static bool palHold;
static u16 tintNow[2];
static u16 screenEnd = BG_BASE;

static void palApply(u16 index, u16 count)
{
    if (!palHold) PAL_setColors(index, curPal + index, count, CPU);
}

void setPalette(u16 index, const u16* cols, u16 n)
{
    memcpy(curPal + index, cols, n * 2);
    palApply(index, n);
}

void setPal3(void)
{
    memcpy(curPal + 48, pal3Colors, 32);
    palApply(48, 16);
}

void gfxInit(void)
{
    u16 i;
    for (i = 0; i < 64; i++) curPal[i] = 0;
    VDP_setScreenWidth320();
    /* pad 6 boutons accepte (X Y Z = super technique ; Xbox Y / LB / RB sous RetroArch) */
    JOY_setSupport(PORT_1, JOY_SUPPORT_6BTN);
    JOY_setSupport(PORT_2, JOY_SUPPORT_6BTN);
    VDP_setPlaneSize(64, 32, TRUE);
    SPR_initEx(SPR_VRAM);
    VDP_loadFont(&font_gold, DMA);
    VDP_loadTileSet(&font_white, WFONT_BASE, DMA);
    VDP_loadTileSet(&hud_tiles, HUD_BASE, DMA);
    VDP_setTextPlane(BG_A);
    VDP_setTextPalette(PAL3);
    VDP_setTextPriority(1);
    VDP_setBackgroundColor(0);
    setPal3();
}

/* ------------------------------------------------------------------ fondus */
void fadeOut(u16 frames)
{
    PAL_fadeOutAll(frames, FALSE);
    palHold = TRUE;
}

void fadeInScreen(u16 frames)
{
    palHold = FALSE;
    PAL_fadeInAll(curPal, frames, FALSE);
}

/* ------------------------------------------------------------------ images */
void loadScreenImage(const Image* img, bool pal0Only)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    VDP_drawImageEx(BG_B, img, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, BG_BASE), 0, 0, FALSE, DMA);
    screenEnd = BG_BASE + img->tileset->numTile;
    memcpy(curPal, img->palette->data, 32);
    palApply(0, 16);
    (void)pal0Only;
    setPal3();
}

u16 screenTilesEnd(void) { return screenEnd; }

/* ------------------------------------------------------------------ combattants */
static const u16 skinCol[3][3] = { { C(0xFCC8A0), C(0xDC966E), C(0xA06040) },
                                   { C(0xD69664), C(0xB06E44), C(0x784628) },
                                   { C(0x96603C), C(0x6C4024), C(0x462814) } };
static const u16 hairCol[3][2] = { { C(0x463A34), C(0x1C1616) }, { C(0x8C5C30), C(0x543418) },
                                   { C(0xECC460), C(0xB07E34) } };
static const u16 giCol[3][4] = { { C(0xFCFCFC), C(0xCCD6EA), C(0x909EC4), C(0x545E88) },
                                 { C(0x5490FC), C(0x2C64E0), C(0x1A3EA4), C(0x0E2062) },
                                 { C(0x606070), C(0x40404E), C(0x282832), C(0x16161C) } };
static const u16 accentCol[8] = { C(0xE6C428), C(0xD82424), C(0x24B648), C(0xFCFCFC),
                                  C(0x24C8E6), C(0xE67E24), C(0xB648D8), C(0x2464DA) };

void buildFighterPalette(u16* pal, const FighterDef* f, u8 gi)
{
    u16 i;
    pal[0] = 0;
    pal[1] = C(0x101028);
    for (i = 0; i < 3; i++) pal[2 + i] = skinCol[f->skin][i];
    if (f->hair == HAIR_BALD)
    {
        pal[5] = skinCol[f->skin][1];
        pal[6] = skinCol[f->skin][2];
    }
    else
    {
        pal[5] = hairCol[f->hair][0];
        pal[6] = hairCol[f->hair][1];
    }
    for (i = 0; i < 4; i++) pal[7 + i] = giCol[gi][i];
    pal[11] = C(0x1A1A1A);
    pal[12] = f->beard ? ((f->hair == HAIR_BALD) ? hairCol[0][1] : pal[6]) : skinCol[f->skin][1];
    pal[13] = C(0xC81E1E);
    pal[14] = C(0xFAFAFA);
    pal[15] = accentCol[f->accent & 7];
}

void applyFighterPalettes(void)
{
    buildFighterPalette(P[0].pal, P[0].def, P[0].gi);
    buildFighterPalette(P[1].pal, P[1].def, P[1].gi);
    memcpy(curPal + 16, P[0].pal, 32);
    memcpy(curPal + 32, P[1].pal, 32);
    tintNow[0] = tintNow[1] = 0;
    palApply(16, 32);
}

/* portrait (plan A, palette du joueur) */
void drawPortrait(u16 who, u16 gid, u16 x, u16 y, u16 tileBase)
{
    VDP_drawImageEx(BG_A, portraits[gid], TILE_ATTR_FULL(who ? PAL2 : PAL1, TRUE, FALSE, FALSE, tileBase),
                    x, y, FALSE, DMA);
}

/* ------------------------------------------------------------------ teintes de combat */
static u16 tintFighter[2][16];
static u16 tintStage[16];
static s16 flashT;

static u16 tintColor(u16 c, u16 mode)
{
    u16 r = (c >> 1) & 7, g = (c >> 5) & 7, b = (c >> 9) & 7;
    switch (mode)
    {
        case 1: r = r + 3 > 7 ? 7 : r + 3; g = g + 3 > 7 ? 7 : g + 3; b = b + 2 > 7 ? 7 : b + 2; break;
        case 2: r = g = b = 7; break;
        case 3: r = r + 4 > 7 ? 7 : r + 4; g = g + 3 > 7 ? 7 : g + 3; b = b >> 1; break;
        case 4: r >>= 1; g >>= 1; b >>= 1; break;
        default: break;
    }
    return (b << 9) | (g << 5) | (r << 1);
}

/* teinte d'un combattant (palette PAL1 / PAL2) ; 0 = palette normale */
void fighterTint(u16 who, u16 mode)
{
    u16 i, base = 16 + who * 16;
    if (palHold || tintNow[who] == mode) return;
    tintNow[who] = mode;
    {
        u16* buf = tintFighter[who];        /* un tampon par palette : le DMA lit en fin d'image */
        buf[0] = 0;
        for (i = 1; i < 16; i++) buf[i] = mode ? tintColor(curPal[base + i], mode) : curPal[base + i];
        if (mode == 2) buf[1] = curPal[base + 1];      /* garde le contour */
        PAL_setColors(base, buf, 16, DMA_QUEUE);
    }
}

/* decor assombri pendant une super technique */
void stageDim(bool on)
{
    u16 i;
    if (palHold) return;
    for (i = 0; i < 16; i++) tintStage[i] = on ? tintColor(curPal[i], 4) : curPal[i];
    PAL_setColors(0, tintStage, 16, DMA_QUEUE);
}

/* eclair blanc sur le decor (quelques images) */
void gfxFlash(u16 frames)
{
    u16 i;
    if (palHold) return;
    for (i = 0; i < 16; i++) tintStage[i] = tintColor(curPal[i], 1);
    PAL_setColors(0, tintStage, 16, DMA_QUEUE);
    flashT = frames;
}

void gfxUpdate(void)
{
    if (flashT > 0 && --flashT == 0 && !palHold) PAL_setColors(0, curPal, 16, DMA_QUEUE);
}

/* portrait opaque (fond = couleur de contour) pour la cinematique de super technique */
static u32 cutBuf[72 * 8];

void drawPortraitOpaque(u16 who, u16 gid, u16 x, u16 y, u16 tileBase)
{
    const Image* img = portraits[gid];
    const u32* src = img->tileset->tiles;
    u16 n = img->tileset->numTile, k, b;
    if (n > 72) n = 72;
    for (k = 0; k < n * 8; k++)
    {
        u32 v = src[k], o = 0;
        for (b = 0; b < 8; b++)
        {
            u32 nib = (v >> (b * 4)) & 15;
            if (!nib) nib = 1;
            o |= nib << (b * 4);
        }
        cutBuf[k] = o;
    }
    VDP_loadTileData(cutBuf, tileBase, n, DMA);
    VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(who ? PAL2 : PAL1, TRUE, FALSE, FALSE, tileBase),
                     x, y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
}
