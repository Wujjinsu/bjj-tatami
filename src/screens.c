/*
 * Ecrans hors combat : titre + menu, nom du joueur, selection, versus, resultat.
 */
#include "game.h"

#define STAND_Y     154

static void prepPlayers(u16 humans);

/* ==================================================================== TITRE */
static u16 tPhase, tItem;
static u16 tIdle;

static const char* const diffName[3] = { "FACILE", "NORMAL", "DIFFICILE" };

static void titleMenu(void)
{
    char b[41];
    u16 n;
    clearRows(23, 27);
    if (tPhase == 0)
    {
        txtCenter("HOMEBREW MEGA DRIVE - SGDK", 27, TXT_WHITE);
        return;
    }
    txt("1 JOUEUR", 8, 24, tItem == 0 ? TXT_RED : TXT_WHITE);
    txt("2 JOUEURS", 24, 24, tItem == 1 ? TXT_RED : TXT_WHITE);

    n = 0;
    { const char* p = "NOM DU JOUEUR 1 : "; while (*p) b[n++] = *p++; }
    { const char* p = playerName; while (*p) b[n++] = *p++; }
    b[n] = 0;
    txt(b, 8, 25, tItem == 2 ? TXT_RED : TXT_WHITE);

    n = 0;
    { const char* p = "NIVEAU : < "; while (*p) b[n++] = *p++; }
    { const char* p = diffName[difficulty]; while (*p) b[n++] = *p++; }
    b[n++] = ' '; b[n++] = '>'; b[n] = 0;
    txt(b, 8, 26, tItem == 3 ? TXT_RED : TXT_WHITE);

    n = 0;
    { const char* p = "DUREE  : < "; while (*p) b[n++] = *p++; }
    b[n++] = '0' + matchMinutes;
    { const char* p = " MIN >"; while (*p) b[n++] = *p++; }
    b[n] = 0;
    txt(b, 8, 27, tItem == 4 ? TXT_RED : TXT_WHITE);

    txt(">", tItem == 1 ? 22 : 6, tItem <= 1 ? 24 : 23 + tItem, TXT_RED);
}

static void startDemo(void)
{
    u16 i;
    numHumans = 0;
    catIdx = random() % NUM_CATS;
    fighterIdx[0] = random() % FIGHTERS_PER_CAT;
    fighterIdx[1] = (fighterIdx[0] + 1 + (random() % (FIGHTERS_PER_CAT - 1))) % FIGHTERS_PER_CAT;
    prepPlayers(0);
    for (i = 0; i < 2; i++)
    {
        /* en demo, on affiche le prenom du combattant */
        const char* n = P[i].def->name;
        u16 k = 0;
        while (n[k] && n[k] != ' ' && k < NAME_LEN) { P[i].label[k] = n[k]; k++; }
        P[i].label[k] = 0;
    }
    match_enter();
}

void title_enter(void)
{
    gameMode = MODE_TITLE;
    hideAllSprites();
    showBackground(&bg_title);
    musicTitleStart();
    tIdle = 0;
    titleMenu();
}

void title_update(void)
{
    if (tPhase == 0)
    {
        if ((frameCount & 31) == 0) txtCenter("APPUIE SUR START", 25, TXT_WHITE);
        if ((frameCount & 31) == 20) clearRow(25);
        if (mpressed(BUTTON_START) || mpressed(BUTTON_A))
        {
            tPhase = 1;
            tItem = 0;
            sfx(660, 4);
            titleMenu();
            return;
        }
        if (++tIdle > 900) startDemo();
        return;
    }

    if (mpressed(BUTTON_UP))
    {
        tItem = (tItem <= 1) ? 4 : (tItem == 2 ? 0 : tItem - 1);
        sfx(600, 3); titleMenu();
    }
    else if (mpressed(BUTTON_DOWN))
    {
        tItem = (tItem <= 1) ? 2 : (tItem == 4 ? 0 : tItem + 1);
        sfx(600, 3); titleMenu();
    }
    else if (mpressed(BUTTON_LEFT) || mpressed(BUTTON_RIGHT))
    {
        bool right = mpressed(BUTTON_RIGHT);
        if (tItem <= 1) tItem = 1 - tItem;
        else if (tItem == 3)
        {
            if (right && difficulty < 2) difficulty++;
            if (!right && difficulty > 0) difficulty--;
        }
        else if (tItem == 4)
        {
            static const u16 mins[3] = { 2, 3, 5 };
            u16 k = (matchMinutes == 2) ? 0 : (matchMinutes == 3 ? 1 : 2);
            if (right && k < 2) k++;
            if (!right && k > 0) k--;
            matchMinutes = mins[k];
        }
        sfx(600, 3); titleMenu();
    }
    else if (mpressed(BUTTON_A) || mpressed(BUTTON_START))
    {
        sfx(880, 6);
        if (tItem <= 1) { numHumans = tItem + 1; select_enter(); }
        else if (tItem == 2) name_enter();
    }
    else if (mpressed(BUTTON_B))
    {
        tPhase = 0;
        tIdle = 0;
        titleMenu();
    }
}

/* ==================================================================== NOM */
static const char* const gridRow[3] = { "ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "0123456789-. " };
static u16 gx, gy;
static char editName[NAME_LEN + 1];

static void nameDraw(void)
{
    char b[24];
    u16 r, c, n;

    n = slen(editName);
    for (c = 0; c < NAME_LEN; c++) b[c] = (c < n) ? editName[c] : '_';
    b[NAME_LEN] = 0;
    txtCenter(b, 10, TXT_RED);

    for (r = 0; r < 3; r++)
        for (c = 0; c < 13; c++)
        {
            char ch[2] = { gridRow[r][c], 0 };
            if (ch[0] == ' ') ch[0] = '_';
            txt(ch, 8 + c * 2, 12 + r, (gy == r && gx == c) ? TXT_RED : TXT_DARK);
        }
    txt("EFFACER", 9, 16, (gy == 3 && gx == 0) ? TXT_RED : TXT_WHITE);
    txt("VALIDER", 24, 16, (gy == 3 && gx == 1) ? TXT_RED : TXT_WHITE);
}

static void nameValidate(void)
{
    u16 n = slen(editName);
    while (n > 0 && editName[n - 1] == ' ') editName[--n] = 0;
    if (n == 0) scopy(playerName, "JOUEUR 1", NAME_LEN);
    else scopy(playerName, editName, NAME_LEN);
    nameSave();
    sfx(990, 8);
    title_enter();
}

void name_enter(void)
{
    gameMode = MODE_NAME;
    hideAllSprites();
    showBackground(&bg_dojo);
    scopy(editName, playerName, NAME_LEN);
    {
        /* nom par defaut : on repart d'un nom vide */
        const char* def = "JOUEUR 1";
        u16 k = 0;
        while (def[k] && editName[k] == def[k]) k++;
        if (def[k] == 0 && editName[k] == 0) editName[0] = 0;
    }
    gx = gy = 0;
    txtCenter("NOM DU JOUEUR 1", 9, TXT_DARK);
    txtCenter("A:AJOUTER  B:EFFACER  START:VALIDER", 24, TXT_WHITE);
    nameDraw();
}

void name_update(void)
{
    bool redraw = FALSE;
    u16 n = slen(editName);

    if (mpressed(BUTTON_LEFT))  { if (gy == 3) gx = 1 - gx; else gx = (gx + 12) % 13; redraw = TRUE; }
    if (mpressed(BUTTON_RIGHT)) { if (gy == 3) gx = 1 - gx; else gx = (gx + 1) % 13; redraw = TRUE; }
    if (mpressed(BUTTON_UP))    { gy = (gy + 3) % 4; if (gy == 3) gx = (gx >= 6); redraw = TRUE; }
    if (mpressed(BUTTON_DOWN))  { gy = (gy + 1) % 4; if (gy == 3) gx = (gx >= 6); redraw = TRUE; }

    if (mpressed(BUTTON_A))
    {
        if (gy == 3)
        {
            if (gx == 0) { if (n) editName[n - 1] = 0; }
            else { nameValidate(); return; }
        }
        else if (n < NAME_LEN)
        {
            editName[n] = gridRow[gy][gx];
            editName[n + 1] = 0;
            if (n + 1 == NAME_LEN) { gy = 3; gx = 1; }
        }
        sfx(700, 3);
        redraw = TRUE;
    }
    if (mpressed(BUTTON_B) && n) { editName[n - 1] = 0; sfx(400, 3); redraw = TRUE; }
    if (mpressed(BUTTON_START)) { nameValidate(); return; }
    if (redraw) nameDraw();
}

/* ==================================================================== SELECTION */
static u16 sPhase;
static s16 sTimer;

static void prepPlayers(u16 humans)
{
    u16 i;
    for (i = 0; i < 2; i++)
    {
        P[i].def = &roster[catIdx].f[fighterIdx[i]];
        P[i].gi = P[i].def->gi;
        P[i].joy = (i == 0) ? JOY_1 : JOY_2;
    }
    P[0].human = (humans >= 1);
    P[1].human = (humans == 2);
    scopy(P[0].label, (humans >= 1) ? playerName : "CPU 1", NAME_LEN);
    scopy(P[1].label, (humans == 2) ? "JOUEUR 2" : "CPU", NAME_LEN);
    /* deux kimonos de la meme couleur : le deuxieme change */
    if (P[1].gi == P[0].gi) P[1].gi = (P[0].gi == GI_WHITE) ? GI_BLUE : GI_WHITE;
    applyPlayerPalettes();
}

static void selectDraw(void)
{
    char b[41];
    u16 n = 0, k;
    const char* p;

    p = "< "; while (*p) b[n++] = *p++;
    p = roster[catIdx].name; while (*p) b[n++] = *p++;
    b[n++] = ' '; b[n++] = ' ';
    p = roster[catIdx].weight; while (*p) b[n++] = *p++;
    p = " >"; while (*p) b[n++] = *p++;
    b[n] = 0;
    txtCenter(b, 9, TXT_DARK);

    for (k = 0; k < FIGHTERS_PER_CAT; k++)
    {
        clearRow(11 + k);
        txt(roster[catIdx].f[k].name, 13, 11 + k, TXT_DARK);
        if (fighterIdx[0] == k) txt(">", 11, 11 + k, TXT_RED);
        if (sPhase >= 1 && fighterIdx[1] == k) txt("<", 28, 11 + k, TXT_GREEN);
    }

    /* fiches : joueur 1 a gauche ; adversaire a droite des qu'il est choisi */
    clearRows(16, 22);
    {
        const FighterDef* f = &roster[catIdx].f[fighterIdx[0]];
        static const char* const lab[ST_COUNT] = { "GAR ", "PAS ", "LUT ", "SOU ", "CAR " };
        u16 s;
        txt(f->nick, 6, 16, TXT_WHITE);
        for (s = 0; s < ST_COUNT; s++)
        {
            txt(lab[s], 6, 17 + s, TXT_WHITE);
            statBar(f->stat[s], b);
            txt(b, 10, 17 + s, TXT_WHITE);
        }
        txt(subFamilyName[f->special], 6, 22, TXT_WHITE);
        if (sPhase >= 1)
        {
            f = &roster[catIdx].f[fighterIdx[1]];
            txt(f->nick, 21, 16, TXT_WHITE);
            for (s = 0; s < ST_COUNT; s++)
            {
                txt(lab[s], 21, 17 + s, TXT_WHITE);
                statBar(f->stat[s], b);
                txt(b, 25, 17 + s, TXT_WHITE);
            }
            txt(subFamilyName[f->special], 21, 22, TXT_WHITE);
        }
    }

    /* apercu des combattants (couleurs reelles) */
    if (sPhase == 0) fighterIdx[1] = (fighterIdx[0] + 1) % FIGHTERS_PER_CAT;
    prepPlayers(numHumans);
    P[0].x = 26; P[1].x = 294;
    P[0].act = P[1].act = 0;
    P[0].walking = P[1].walking = FALSE;
    SPR_setVisibility(P[0].spr, VISIBLE);
    SPR_setVisibility(P[1].spr, sPhase >= 1 ? VISIBLE : HIDDEN);
}

void select_enter(void)
{
    gameMode = MODE_SELECT;
    hideAllSprites();
    showBackground(&bg_dojo);
    sPhase = 0;
    sTimer = 0;
    if (catIdx >= NUM_CATS) catIdx = 0;
    txtCenter("CHOISIS TA CATEGORIE ET TON COMBATTANT", 24, TXT_WHITE);
    txtCenter("A : VALIDER    B : RETOUR", 25, TXT_WHITE);
    selectDraw();
}

void select_update(void)
{
    u16 i;

    if (sPhase == 0)
    {
        if (mpressed(BUTTON_LEFT))  { catIdx = (catIdx + NUM_CATS - 1) % NUM_CATS; sfx(600, 3); selectDraw(); }
        if (mpressed(BUTTON_RIGHT)) { catIdx = (catIdx + 1) % NUM_CATS; sfx(600, 3); selectDraw(); }
        if (mpressed(BUTTON_UP))    { fighterIdx[0] = (fighterIdx[0] + FIGHTERS_PER_CAT - 1) % FIGHTERS_PER_CAT; sfx(600, 3); selectDraw(); }
        if (mpressed(BUTTON_DOWN))  { fighterIdx[0] = (fighterIdx[0] + 1) % FIGHTERS_PER_CAT; sfx(600, 3); selectDraw(); }
        if (mpressed(BUTTON_B))     { title_enter(); return; }
        if (mpressed(BUTTON_A) || mpressed(BUTTON_START))
        {
            sfx(880, 6);
            sPhase = 1;
            if (numHumans == 2)
            {
                fighterIdx[1] = (fighterIdx[0] + 1) % FIGHTERS_PER_CAT;
                txtCenter("JOUEUR 2 : HAUT/BAS PUIS A", 24, TXT_WHITE);
            }
            else
            {
                fighterIdx[1] = random() % FIGHTERS_PER_CAT;
                if (fighterIdx[1] == fighterIdx[0] && (random() & 1)) fighterIdx[1] = (fighterIdx[1] + 1) % FIGHTERS_PER_CAT;
                sTimer = 70;
                txtCenter("LE CPU CHOISIT...", 24, TXT_WHITE);
            }
            selectDraw();
        }
    }
    else if (sPhase == 1)
    {
        if (numHumans == 2)
        {
            if (pressed(1, BUTTON_UP))   { fighterIdx[1] = (fighterIdx[1] + FIGHTERS_PER_CAT - 1) % FIGHTERS_PER_CAT; sfx(600, 3); selectDraw(); }
            if (pressed(1, BUTTON_DOWN)) { fighterIdx[1] = (fighterIdx[1] + 1) % FIGHTERS_PER_CAT; sfx(600, 3); selectDraw(); }
            if (pressed(1, BUTTON_A) || pressed(1, BUTTON_START)) { sfx(880, 6); versus_enter(); return; }
            if (mpressed(BUTTON_B)) { sPhase = 0; select_enter(); return; }
        }
        else if (--sTimer <= 0) { versus_enter(); return; }
    }

    for (i = 0; i < 2; i++) updateFighterSprite(i, STAND_Y);
}

/* ==================================================================== VERSUS */
static s16 vTimer;

void versus_enter(void)
{
    char b[41];
    u16 n = 0;
    const char* p;

    gameMode = MODE_VERSUS;
    prepPlayers(numHumans);
    clearRows(9, 25);

    p = P[0].label; while (*p) b[n++] = *p++;
    p = "   VS   "; while (*p) b[n++] = *p++;
    p = P[1].label; while (*p) b[n++] = *p++;
    b[n] = 0;
    txtCenter(b, 9, TXT_RED);
    txt(P[0].def->name, 2, 11, TXT_DARK);
    txt(P[0].def->nick, 2, 12, TXT_DARK);
    txt(P[1].def->name, 38 - slen(P[1].def->name), 11, TXT_DARK);
    txt(P[1].def->nick, 38 - slen(P[1].def->nick), 12, TXT_DARK);
    n = 0;
    p = roster[catIdx].name; while (*p) b[n++] = *p++;
    b[n++] = ' '; b[n++] = ' ';
    p = roster[catIdx].weight; while (*p) b[n++] = *p++;
    b[n] = 0;
    txtCenter(b, 14, TXT_DARK);
    txtCenter("OSS !", 17, TXT_WHITE);

    P[0].x = 120; P[1].x = 200;
    P[0].act = P[1].act = 0;
    SPR_setVisibility(P[0].spr, VISIBLE);
    SPR_setVisibility(P[1].spr, VISIBLE);
    updateFighterSprite(0, STAND_Y);
    updateFighterSprite(1, STAND_Y);
    vTimer = 180;
    sfx(440, 10);
}

void versus_update(void)
{
    updateFighterSprite(0, STAND_Y);
    updateFighterSprite(1, STAND_Y);
    if (--vTimer <= 0 || mpressed(BUTTON_START) || mpressed(BUTTON_A)) match_enter();
}

/* ==================================================================== RESULTAT */
static s16 rTimer;

void result_enter(void)
{
    char b[41];
    u16 n = 0;
    const char* p;

    gameMode = MODE_RESULT;
    clearRows(3, 27);
    SPR_setVisibility(groundSpr, HIDDEN);
    P[0].x = 120; P[1].x = 200;
    P[0].act = P[1].act = 0;
    P[0].walking = P[1].walking = FALSE;
    SPR_setVisibility(P[0].spr, VISIBLE);
    SPR_setVisibility(P[1].spr, VISIBLE);

    if (resWinner >= 0)
    {
        p = P[resWinner].label; while (*p) b[n++] = *p++;
        p = " GAGNE !"; while (*p) b[n++] = *p++;
        b[n] = 0;
        txtCenter(b, 9, TXT_RED);
        txtCenter(P[resWinner].def->name, 10, TXT_DARK);
    }
    else txtCenter("EGALITE", 9, TXT_RED);
    txtCenter(resMethod, 11, TXT_DARK);

    n = 0;
    numStr(P[0].points, 2, b); n = 2;
    p = " PTS  -  "; while (*p) b[n++] = *p++;
    numStr(P[1].points, 2, b + n); n += 2;
    p = " PTS"; while (*p) b[n++] = *p++;
    b[n] = 0;
    txtCenter(b, 13, TXT_DARK);

    if (numHumans == 0) txtCenter("DEMO", 24, TXT_WHITE);
    else txtCenter("A : REVANCHE    START : MENU", 24, TXT_WHITE);
    rTimer = 0;
}

void result_update(void)
{
    rTimer++;
    updateFighterSprite(0, STAND_Y);
    updateFighterSprite(1, STAND_Y);
    if (numHumans == 0)
    {
        if (rTimer > 240 || mpressed(BUTTON_START)) { crowdStop(); title_enter(); }
        return;
    }
    if (rTimer < 30) return;
    if (mpressed(BUTTON_A)) match_enter();
    else if (mpressed(BUTTON_START)) { crowdStop(); title_enter(); }
}
