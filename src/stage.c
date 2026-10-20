/*
 * Arene de combat : decor de 512 px, camera, defilement parallaxe ligne par ligne,
 * public anime par palette (bras leves, flashs), tremblement d'ecran.
 */
#include "game.h"

static const Image* const stageImg[3] = { &stage_arena, &stage_academy, &stage_beach };
static const bool stageCrowd[3] = { TRUE, FALSE, TRUE };

static s16 hs[224];
static s16 hsA[224];
static u16 fac[224];        /* facteur de parallaxe x256 */
static s16 cam = CAM_MAX / 2, camTarget = CAM_MAX / 2;
static s16 lastCam = -1;
static s16 cheer, shake;
static u16 crowdBase, crowdArm;
static s16 flashT;
static u16 stageNow;

void stageLoad(u16 idx)
{
    u16 y;
    stageNow = idx;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_drawImageEx(BG_B, stageImg[idx], TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, BG_BASE), 0, 0, FALSE, DMA);
    setPalette(0, stageImg[idx]->palette->data, 16);
    setPal3();
    crowdBase = stageImg[idx]->palette->data[11];
    crowdArm = stageImg[idx]->palette->data[3];

    for (y = 0; y < 224; y++)
    {
        u16 f;
        /* parallaxe par bandes (un plan = un facteur, sinon les murs se deforment),
           le tapis seul a un degrade ligne par ligne pour la perspective */
        if (y < 56) f = 0;                                  /* bandeau du score */
        else if (y >= 144) f = 240 + (y - 144) * 16 / 80;   /* tapis */
        else if (idx == 1) f = 200;                         /* academie : mur plat */
        else if (idx == 2) f = (y < 96) ? 60 : (y < 112 ? 110 : (y < 136 ? 180 : 230));
        else f = (y < 120) ? 160 : 224;                     /* arene : public / panneaux */
        fac[y] = f;
        hsA[y] = 0;
    }
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    VDP_setHorizontalScrollLine(BG_A, 0, hsA, 224, DMA);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);
    cam = camTarget = CAM_MAX / 2;
    lastCam = -1;
    cheer = shake = 0;
    flashT = 0;
    stageUpdate();
}

s16 camX(void) { return cam; }

void stageSetCamera(s16 target)
{
    s16 t = target - SCREEN_W / 2;
    if (t < 0) t = 0;
    if (t > CAM_MAX) t = CAM_MAX;
    camTarget = t;
}

void stageCheer(s16 frames) { if (frames > cheer) cheer = frames; }
void stageShake(s16 frames) { if (frames > shake) shake = frames; }
s16 shakeY(void) { return (shake > 0) ? ((shake & 2) ? 2 : -2) : 0; }

void stageUpdate(void)
{
    u16 y;
    s16 d = camTarget - cam;

    /* camera : approche douce */
    if (d > 0) cam += (d > 24) ? 3 : (d > 6 ? 2 : 1);
    else if (d < 0) cam -= (d < -24) ? 3 : (d < -6 ? 2 : 1);

    if (cam != lastCam)
    {
        s16 c = cam - CAM_MAX / 2;
        for (y = 0; y < 224; y++)
            hs[y] = -((s16)((c * (s16)fac[y]) >> 8)) - CAM_MAX / 2;
        VDP_setHorizontalScrollLine(BG_B, 0, hs, 224, DMA_QUEUE);
        lastCam = cam;
    }

    if (shake > 0)
    {
        shake--;
        VDP_setVerticalScroll(BG_B, shakeY());
    }
    else if (shake == 0)
    {
        VDP_setVerticalScroll(BG_B, 0);
        shake = -1;
    }

    if (!stageCrowd[stageNow]) return;
    /* public : bras leves en alternance pendant les acclamations */
    if (cheer > 0)
    {
        bool g = (frameCount >> 3) & 1;
        cheer--;
        PAL_setColor(11, g ? crowdArm : crowdBase);
        PAL_setColor(12, g ? crowdBase : crowdArm);
        if (cheer == 0)
        {
            PAL_setColor(11, crowdBase);
            PAL_setColor(12, crowdBase);
        }
    }
    /* flashs d'appareils photo */
    if (flashT > 0)
    {
        if (--flashT == 0) PAL_setColor(13, crowdBase);
    }
    else if ((random() & 0xFF) < (cheer > 0 ? 24 : 3))
    {
        PAL_setColor(13, RGB24_TO_VDPCOLOR(0xFFFFFF));
        flashT = 2;
    }
}
