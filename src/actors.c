/*
 * Combattants a l'ecran : 3 emplacements de sprites a VRAM fixe.
 *  - mode solo  : emplacement 0 = joueur 0, emplacement 1 = joueur 1 (animations individuelles)
 *  - mode paire : les 3 emplacements affichent les 3 calques d'une animation en paire,
 *                 chaque calque prenant la palette du combattant auquel il appartient.
 */
#include "game.h"

static const SpriteDefinition* const layerDef[3] = { &spr_pair_l0, &spr_pair_l1, &spr_pair_l2 };
static Sprite* slot[3];
static Sprite* shadow[2];
static Sprite* shadowPair;
static bool pairMode;
static bool shown = TRUE, shadowsOn = TRUE;

/* paire */
static u16 pSeq;
static s16 pFrame, pTick;
static bool pRev, pFlip, pDone;
static s16 pAnchor;
static u16 pRoleA;
static s16 pShake;

void actorsInit(void)
{
    u16 k;
    for (k = 0; k < 3; k++)
    {
        slot[k] = SPR_addSpriteEx(&spr_solo, 0, -128,
                                  TILE_ATTR_FULL(k == 1 ? PAL2 : PAL1, FALSE, FALSE, FALSE, SLOT_BASE + k * SLOT_SIZE),
                                  SPR_FLAG_AUTO_TILE_UPLOAD);
        SPR_setDepth(slot[k], 2 - k);
    }
    for (k = 0; k < 2; k++)
    {
        shadow[k] = SPR_addSprite(&spr_fx, 0, -64, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
        SPR_setAnimAndFrame(shadow[k], 0, FX_SHADOW);
        SPR_setDepth(shadow[k], 40);
    }
    shadowPair = SPR_addSprite(&spr_shadow_pair, 0, -64, TILE_ATTR(PAL3, FALSE, FALSE, FALSE));
    SPR_setDepth(shadowPair, 40);
    pairMode = FALSE;
    actorsSoloMode();
}

void actorsRelease(void)
{
    u16 k;
    for (k = 0; k < 3; k++) { if (slot[k]) SPR_releaseSprite(slot[k]); slot[k] = NULL; }
    for (k = 0; k < 2; k++) { if (shadow[k]) SPR_releaseSprite(shadow[k]); shadow[k] = NULL; }
    if (shadowPair) SPR_releaseSprite(shadowPair);
    shadowPair = NULL;
}

void actorsShow(bool show) { shown = show; }
void actorsShadows(bool show) { shadowsOn = show; }
void actorsShake(s16 frames) { pShake = frames; }

/* ------------------------------------------------------------------ solo */
void actorsSoloMode(void)
{
    u16 k;
    if (!slot[0]) return;
    pairMode = FALSE;
    for (k = 0; k < 2; k++)
    {
        SPR_setDefinition(slot[k], &spr_solo);
        SPR_setPalette(slot[k], k ? PAL2 : PAL1);
        P[k].animFrame = 0;
        P[k].animTick = 0;
    }
    SPR_setVisibility(slot[2], HIDDEN);
}

void actorsSoloSet(u16 i, u16 anim, bool restart)
{
    if (P[i].anim == anim && !restart) return;
    P[i].anim = anim;
    P[i].animFrame = P[i].animRev ? soloAnims[anim].n - 1 : 0;
    P[i].animTick = 0;
}

void actorsSoloReverse(u16 i, bool rev) { P[i].animRev = rev; }

bool actorsSoloDone(u16 i)
{
    const SoloAnim* a = &soloAnims[P[i].anim];
    if (a->loop) return FALSE;
    return P[i].animFrame >= a->n - 1 && P[i].animTick >= a->f[a->n - 1].ticks;
}

void actorsSoloUpdate(void)
{
    u16 i;
    if (pairMode) return;
    for (i = 0; i < 2; i++)
    {
        Player* p = &P[i];
        const SoloAnim* a = &soloAnims[p->anim];
        if (++p->animTick >= a->f[p->animFrame].ticks)
        {
            if (p->animRev)
            {
                if (p->animFrame > 0) { p->animFrame--; p->animTick = 0; }
                else if (a->loop) { p->animFrame = a->n - 1; p->animTick = 0; }
            }
            else
            {
                if (p->animFrame < a->n - 1) { p->animFrame++; p->animTick = 0; }
                else if (a->loop) { p->animFrame = 0; p->animTick = 0; }
            }
        }
    }
}

/* ------------------------------------------------------------------ paire */
void actorsPairStart(u16 seq, s16 anchorX, bool flip, u16 roleA, bool reverse)
{
    u16 k;
    if (!pairMode)
    {
        for (k = 0; k < 3; k++) SPR_setDefinition(slot[k], layerDef[k]);
        pairMode = TRUE;
    }
    pSeq = seq;
    pRev = reverse;
    pFrame = reverse ? pairSeqs[seq].n - 1 : 0;
    pTick = 0;
    pAnchor = anchorX;
    pFlip = flip;
    pRoleA = roleA;
    pDone = FALSE;
}

u16 actorsPairSeq(void) { return pSeq; }
s16 actorsPairFrame(void) { return pFrame; }
s16 actorsPairAnchor(void) { return pAnchor; }
bool actorsPairFlip(void) { return pFlip; }

void actorsPairWorldPos(u16 role, s16* wx, s16* wy)
{
    const PairFrame* f = &pairSeqs[pSeq].f[pFrame];
    s16 fx = role ? f->bx : f->ax;
    s16 fy = role ? f->by : f->ay;
    *wx = pAnchor + (pFlip ? (PAIR_AX - fx) : (fx - PAIR_AX));
    *wy = FLOOR_SCREEN - FLOOR_Y + fy;
}

/* renvoie TRUE quand la sequence (non bouclee) est terminee */
bool actorsPairUpdate(void)
{
    const PairSeq* s = &pairSeqs[pSeq];
    if (!pairMode || pDone) return pDone;
    if (++pTick >= s->f[pFrame].ticks)
    {
        pTick = 0;
        if (pRev)
        {
            if (pFrame > 0) pFrame--;
            else if (s->loop) pFrame = s->n - 1;
            else pDone = TRUE;
        }
        else
        {
            if (pFrame < s->n - 1) pFrame++;
            else if (s->loop) pFrame = 0;
            else pDone = TRUE;
        }
        if (!pDone && ((s->f[pFrame].owners >> 3) & 3) == 1)
        {
            s16 x, y;
            actorsPairWorldPos(1, &x, &y);
            fxImpact(x, y + 6, TRUE);
            sfxSlam();
            stageCheer(90);
        }
        else if (!pDone && ((s->f[pFrame].owners >> 3) & 3) == 2)
        {
            s16 x, y;
            actorsPairWorldPos(1, &x, &y);
            fxDust(x, FLOOR_SCREEN - 8);
        }
    }
    return pDone;
}

/* ------------------------------------------------------------------ affichage */
void actorsDraw(void)
{
    u16 k;
    s16 cx = camX();
    s16 sy = shakeY();
    s16 jit = 0;

    if (pShake > 0)
    {
        pShake--;
        jit = (pShake & 2) ? 2 : -2;
    }
    if (!shown)
    {
        for (k = 0; k < 3; k++) SPR_setVisibility(slot[k], HIDDEN);
        for (k = 0; k < 2; k++) SPR_setVisibility(shadow[k], HIDDEN);
        SPR_setVisibility(shadowPair, HIDDEN);
        return;
    }
    if (pairMode)
    {
        const PairFrame* f = &pairSeqs[pSeq].f[pFrame];
        for (k = 0; k < 3; k++)
        {
            u16 owner = ((f->owners >> k) & 1) ? 1 - pRoleA : pRoleA;
            SPR_setVisibility(slot[k], VISIBLE);
            SPR_setAnimAndFrame(slot[k], pSeq, pFrame);
            SPR_setPalette(slot[k], owner ? PAL2 : PAL1);
            SPR_setHFlip(slot[k], pFlip);
            SPR_setPosition(slot[k], pAnchor - cx - PAIR_AX + jit, FLOOR_SCREEN - FLOOR_Y + sy);
        }
        for (k = 0; k < 2; k++) SPR_setVisibility(shadow[k], HIDDEN);
        SPR_setVisibility(shadowPair, shadowsOn ? VISIBLE : HIDDEN);
        SPR_setPosition(shadowPair, pAnchor - cx - 64, FLOOR_SCREEN - 13 + sy);
        return;
    }
    for (k = 0; k < 2; k++)
    {
        Player* p = &P[k];
        bool flip = p->dir < 0;
        SPR_setVisibility(slot[k], VISIBLE);
        SPR_setAnimAndFrame(slot[k], p->anim, p->animFrame);
        SPR_setHFlip(slot[k], flip);
        SPR_setPosition(slot[k], p->x - cx - (flip ? SOLO_W - SOLO_X : SOLO_X), FLOOR_SCREEN - FLOOR_Y + sy);
        SPR_setVisibility(shadow[k], shadowsOn ? VISIBLE : HIDDEN);
        SPR_setPosition(shadow[k], p->x - cx - 16 + soloAnims[p->anim].f[p->animFrame].dx * p->dir,
                        FLOOR_SCREEN - 31 + sy);
    }
    SPR_setVisibility(slot[2], HIDDEN);
    SPR_setVisibility(shadowPair, HIDDEN);
}
