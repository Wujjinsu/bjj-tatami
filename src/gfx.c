/*
 * Graphismes generaux : palettes (avec fondus), images d'ecran, portraits.
 */
#include "game.h"
#include "gen/pal3.h"

#define C(rgb) RGB24_TO_VDPCOLOR(rgb)

static u16 curPal[64];
static bool palHold;
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
    palApply(16, 32);
}

/* portrait (plan A, palette du joueur) */
void drawPortrait(u16 who, u16 gid, u16 x, u16 y, u16 tileBase)
{
    VDP_drawImageEx(BG_A, portraits[gid], TILE_ATTR_FULL(who ? PAL2 : PAL1, TRUE, FALSE, FALSE, tileBase),
                    x, y, FALSE, DMA);
}
