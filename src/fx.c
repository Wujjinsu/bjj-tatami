/*
 * Effets : bannieres geantes (zoom), points qui s'envolent, impacts, poussiere, et l'arbitre.
 */
#include "game.h"

#define NPOPS 3
#define NFX 2

static Sprite* banner;
static s16 banTimer, banTick;

typedef struct { Sprite* s; s16 x, y, t; } Pop;
static Pop pops[NPOPS];

typedef struct { Sprite* s; s16 x, y, t, base; } Fx;
static Fx fxs[NFX];

static Sprite* ref;
static s16 refX, refGoal, refSig, refAnim, refTick, refFrame, refDir = -1;
static bool refVisible;

void fxInit(void)
{
    u16 i;
    banner = SPR_addSprite(&spr_banner, 36, 64, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
    SPR_setVisibility(banner, HIDDEN);
    SPR_setDepth(banner, SPR_MIN_DEPTH);
    banTimer = 0;
    for (i = 0; i < NPOPS; i++)
    {
        pops[i].s = SPR_addSprite(&spr_pops, 0, 0, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
        SPR_setVisibility(pops[i].s, HIDDEN);
        SPR_setDepth(pops[i].s, SPR_MIN_DEPTH + 1);
        pops[i].t = 0;
    }
    for (i = 0; i < NFX; i++)
    {
        fxs[i].s = SPR_addSprite(&spr_fx, 0, 0, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
        SPR_setVisibility(fxs[i].s, HIDDEN);
        SPR_setDepth(fxs[i].s, SPR_MIN_DEPTH + 2);
        fxs[i].t = 0;
    }
    ref = NULL;
}

void fxRelease(void)
{
    u16 i;
    if (banner) SPR_releaseSprite(banner);
    banner = NULL;
    for (i = 0; i < NPOPS; i++) { if (pops[i].s) SPR_releaseSprite(pops[i].s); pops[i].s = NULL; }
    for (i = 0; i < NFX; i++) { if (fxs[i].s) SPR_releaseSprite(fxs[i].s); fxs[i].s = NULL; }
    if (ref) SPR_releaseSprite(ref);
    ref = NULL;
}

/* ------------------------------------------------------------------ bannieres */
void fxBanner(u16 ban, u16 frames)
{
    if (!banner) return;
    SPR_setAnimAndFrame(banner, ban, 0);
    SPR_setVisibility(banner, VISIBLE);
    banTimer = frames;
    banTick = 0;
}

bool fxBannerActive(void) { return banTimer > 0; }

/* ------------------------------------------------------------------ points qui s'envolent */
void fxPop(u16 kind, s16 wx, s16 wy)
{
    u16 i, best = 0;
    for (i = 0; i < NPOPS; i++)
        if (pops[i].t < pops[best].t) best = i;
    if (!pops[best].s) return;
    pops[best].x = wx - 16;
    pops[best].y = wy - 12;
    pops[best].t = 70;
    SPR_setAnimAndFrame(pops[best].s, 0, kind);
    SPR_setVisibility(pops[best].s, VISIBLE);
}

static void startFx(s16 wx, s16 wy, s16 base)
{
    u16 i, best = 0;
    for (i = 0; i < NFX; i++)
        if (fxs[i].t < fxs[best].t) best = i;
    if (!fxs[best].s) return;
    fxs[best].x = wx - 16;
    fxs[best].y = wy - 16;
    fxs[best].t = 16;
    fxs[best].base = base;
    SPR_setAnimAndFrame(fxs[best].s, 0, base);
    SPR_setVisibility(fxs[best].s, VISIBLE);
}

void fxImpact(s16 wx, s16 wy, bool big)
{
    startFx(wx, wy, FX_IMPACT);
    if (big) stageShake(14);
}

void fxDust(s16 wx, s16 wy) { startFx(wx, wy, FX_DUST); }

/* ------------------------------------------------------------------ arbitre */
void refInit(s16 wx)
{
    if (!ref)
    {
        ref = SPR_addSprite(&spr_referee, 0, 0, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
        SPR_setDepth(ref, 20);
    }
    refX = refGoal = wx;
    refSig = 0;
    refAnim = REF_IDLE;
    refFrame = refTick = 0;
    refVisible = TRUE;
    SPR_setVisibility(ref, VISIBLE);
}

void refHide(void) { refVisible = FALSE; if (ref) SPR_setVisibility(ref, HIDDEN); }
void refShow(void) { refVisible = TRUE; if (ref) SPR_setVisibility(ref, VISIBLE); }

void refSignal(u16 anim, s16 faceToX, u16 frames)
{
    refAnim = anim;
    refFrame = 0;
    refTick = 0;
    refSig = frames;
    refDir = (faceToX >= refX) ? 1 : -1;
}

/* l'arbitre se place a cote de l'action, du cote ou il y a de la place */
void refFollow(s16 cx)
{
    s16 goal = (cx > WORLD_W / 2) ? cx - 84 : cx + 84;
    if (goal < 24) goal = 24;
    if (goal > WORLD_W - 24) goal = WORLD_W - 24;
    refGoal = goal;
}

static void refUpdate(void)
{
    s16 d;
    if (!ref) return;
    d = refGoal - refX;
    if (refSig > 0)
    {
        refSig--;
        if (refSig == 0) { refAnim = REF_IDLE; refFrame = 0; }
    }
    else if (d > 3 || d < -3)
    {
        refX += (d > 0) ? 1 : -1;
        refDir = (d > 0) ? 1 : -1;
        if (refAnim != REF_WALK) { refAnim = REF_WALK; refFrame = 0; }
    }
    else if (refAnim == REF_WALK)
    {
        refAnim = REF_IDLE;
        refFrame = 0;
    }
    if (++refTick >= (refAnim == REF_WALK ? 8 : 18))
    {
        refTick = 0;
        if (++refFrame >= refFrames[refAnim]) refFrame = (refAnim == REF_START || refAnim == REF_PEN) ? refFrames[refAnim] - 1 : 0;
    }
    SPR_setAnimAndFrame(ref, refAnim, refFrame);
    SPR_setHFlip(ref, refDir < 0);
    SPR_setPosition(ref, refX - camX() - (refDir < 0 ? 96 - REF_X : REF_X), FLOOR_SCREEN - 108 - 4 + shakeY());
}

/* ------------------------------------------------------------------ mise a jour */
void fxUpdate(void)
{
    u16 i;
    s16 cx = camX();
    if (banTimer > 0)
    {
        banTick++;
        if (banTick < 12) SPR_setFrame(banner, banTick / 3);
        if (--banTimer == 0) SPR_setVisibility(banner, HIDDEN);
    }
    for (i = 0; i < NPOPS; i++)
    {
        Pop* p = &pops[i];
        if (p->t <= 0 || !p->s) continue;
        p->t--;
        if (p->t & 1) p->y--;
        SPR_setPosition(p->s, p->x - cx, p->y);
        if (p->t < 14) SPR_setVisibility(p->s, (p->t & 2) ? VISIBLE : HIDDEN);
        if (p->t == 0) SPR_setVisibility(p->s, HIDDEN);
    }
    for (i = 0; i < NFX; i++)
    {
        Fx* f = &fxs[i];
        if (f->t <= 0 || !f->s) continue;
        f->t--;
        SPR_setFrame(f->s, f->base + (15 - f->t) / 4);
        SPR_setPosition(f->s, f->x - cx, f->y + shakeY());
        if (f->t == 0) SPR_setVisibility(f->s, HIDDEN);
    }
    if (refVisible) refUpdate();
}
