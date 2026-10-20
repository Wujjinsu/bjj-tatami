/*
 * Ecrans hors combat : titre, options, nom, selection, championnat, versus, resultat,
 * continue, sacre, tableau des scores.
 */
#include "game.h"
#include "gen/screens_pal.h"

static s16 timer;
static u16 cursor;

static void enterScreen(const Image* img)
{
    fadeOut(12);
    SPR_reset();
    loadScreenImage(img, FALSE);
    VDP_clearPlane(BG_A, TRUE);
}

static void flagAt(u16 flag, u16 x, u16 y)
{
    u16 f = HT_FLAG_BRA + flag * 4;
    putTile(x, y, f);
    putTile(x + 1, y, f + 1);
    putTile(x, y + 1, f + 2);
    putTile(x + 1, y + 1, f + 3);
}

static void setupPlayers(u16 humans)
{
    u16 i;
    for (i = 0; i < 2; i++)
    {
        P[i].gid = fighterSel[i];
        P[i].def = fighterAt(fighterSel[i]);
        P[i].gi = P[i].def->gi;
        P[i].joy = i ? JOY_2 : JOY_1;
        P[i].anim = SOLO_IDLE;
    }
    if (P[1].gi == P[0].gi) P[1].gi = (P[0].gi == GI_WHITE) ? GI_BLUE : GI_WHITE;
    P[0].human = humans >= 1;
    P[1].human = humans == 2;
    numHumans = humans;
    if (humans == 0)
    {
        u16 k;
        for (i = 0; i < 2; i++)
        {
            const char* n = P[i].def->name;
            for (k = 0; n[k] && n[k] != ' ' && k < NAME_LEN; k++) P[i].label[k] = n[k];
            P[i].label[k] = 0;
        }
    }
    else
    {
        scopy(P[0].label, playerName, NAME_LEN);
        scopy(P[1].label, humans == 2 ? "JOUEUR 2" : "CPU", NAME_LEN);
    }
    applyFighterPalettes();
}

/* ==================================================================== TITRE */
static u16 tPhase;
static u16 rayPhase;
static const char* const menuItems[5] = { "CHAMPIONNAT", "2 JOUEURS", "COMBAT LIBRE", "RECORDS", "OPTIONS" };

static void titleMenu(void)
{
    u16 i;
    clearRows(24, 27);
    if (tPhase == 0) { wtxtC("HOMEBREW MEGA DRIVE - SGDK", 27); return; }
    for (i = 0; i < 5; i++)
    {
        u16 x = (i & 1) ? 23 : 4, y = 24 + i / 2;
        if (i == cursor) { gtxt(">", x - 2, y); gtxt(menuItems[i], x, y); }
        else wtxt(menuItems[i], x, y);
    }
}

void title_enter(void)
{
    gameMode = MODE_TITLE;
    enterScreen(&bg_title);
    musicTitleStart();
    timer = 0;
    rayPhase = 0;
    titleMenu();
    fadeInScreen(16);
}

static void startDemo(void)
{
    gameType = GAME_DEMO;
    fighterSel[0] = random() % NUM_FIGHTERS;
    fighterSel[1] = (fighterSel[0] + 1 + random() % (NUM_FIGHTERS - 1)) % NUM_FIGHTERS;
    stageIdx = random() % 3;
    setupPlayers(0);
    fadeOut(16);
    musicStop();
    match_enter();
}

void title_update(void)
{
    /* rayons qui tournent (cycle de palette) */
    if ((frameCount % 6) == 0)
    {
        u16 k;
        rayPhase = (rayPhase + 1) & 3;
        for (k = 0; k < 4; k++) PAL_setColor(1 + k, titleRays[(k + rayPhase) & 3]);
    }
    if (tPhase == 0)
    {
        if ((frameCount & 31) == 0) gtxtC("APPUIE SUR START", 25);
        if ((frameCount & 31) == 20) clearRow(25);
        if (mpressed(BUTTON_START) || mpressed(BUTTON_A))
        {
            tPhase = 1;
            cursor = 0;
            sfx(660, 4);
            titleMenu();
            return;
        }
        if (++timer > 900)
        {
            timer = 0;
            if ((frameCount >> 10) & 1) records_enter();
            else startDemo();
        }
        return;
    }
    if (mpressed(BUTTON_UP) && cursor >= 2) { cursor -= 2; sfx(600, 3); titleMenu(); }
    else if (mpressed(BUTTON_DOWN) && cursor + 2 < 5) { cursor += 2; sfx(600, 3); titleMenu(); }
    else if (mpressed(BUTTON_LEFT) && (cursor & 1)) { cursor--; sfx(600, 3); titleMenu(); }
    else if (mpressed(BUTTON_RIGHT) && !(cursor & 1) && cursor + 1 < 5) { cursor++; sfx(600, 3); titleMenu(); }
    else if (mpressed(BUTTON_B)) { tPhase = 0; timer = 0; titleMenu(); }
    else if (mpressed(BUTTON_A) || mpressed(BUTTON_START))
    {
        sfx(880, 6);
        switch (cursor)
        {
            case 0: gameType = GAME_ARCADE; select_enter(); break;
            case 1: gameType = GAME_VERSUS; select_enter(); break;
            case 2: gameType = GAME_FREE; select_enter(); break;
            case 3: records_enter(); break;
            case 4: options_enter(); break;
        }
    }
}

/* ==================================================================== OPTIONS */
static const char* const diffName[3] = { "FACILE", "NORMAL", "DIFFICILE" };

static void optionsDraw(void)
{
    char b[41];
    u16 i;
    clearRows(8, 18);
    for (i = 0; i < 5; i++)
    {
        b[0] = 0;
        switch (i)
        {
            case 0: scat(b, "NOM DU JOUEUR 1 : ", 40); scat(b, playerName, 40); break;
            case 1: scat(b, "NIVEAU : < ", 40); scat(b, diffName[difficulty], 40); scat(b, " >", 40); break;
            case 2: { char n[2] = { '0' + matchMinutes, 0 }; scat(b, "DUREE DU COMBAT : < ", 40); scat(b, n, 40); scat(b, " MIN >", 40); } break;
            case 3: scat(b, "AIDE A L'ECRAN : < ", 40); scat(b, helpOn ? "OUI" : "NON", 40); scat(b, " >", 40); break;
            case 4: scat(b, "RETOUR", 40); break;
        }
        if (i == cursor) { gtxt(">", 5, 9 + i * 2); gtxt(b, 7, 9 + i * 2); }
        else wtxt(b, 7, 9 + i * 2);
    }
}

void options_enter(void)
{
    gameMode = MODE_OPTIONS;
    enterScreen(&bg_menu);
    gtxtC("OPTIONS", 4);
    cursor = 0;
    optionsDraw();
    fadeInScreen(12);
}

void options_update(void)
{
    bool l = mpressed(BUTTON_LEFT), r = mpressed(BUTTON_RIGHT);
    if (mpressed(BUTTON_UP)) { cursor = (cursor + 4) % 5; sfx(600, 3); optionsDraw(); }
    else if (mpressed(BUTTON_DOWN)) { cursor = (cursor + 1) % 5; sfx(600, 3); optionsDraw(); }
    else if (l || r)
    {
        if (cursor == 1) { if (r && difficulty < 2) difficulty++; if (l && difficulty > 0) difficulty--; }
        if (cursor == 2)
        {
            static const u16 mins[3] = { 2, 3, 5 };
            u16 k = (matchMinutes == 2) ? 0 : (matchMinutes == 3 ? 1 : 2);
            if (r && k < 2) k++;
            if (l && k > 0) k--;
            matchMinutes = mins[k];
        }
        if (cursor == 3) helpOn = !helpOn;
        sfx(600, 3);
        optionsDraw();
    }
    else if (mpressed(BUTTON_A) || mpressed(BUTTON_START))
    {
        if (cursor == 0) { name_enter(); return; }
        if (cursor == 4 || mpressed(BUTTON_START)) { saveWrite(); title_enter(); }
    }
    else if (mpressed(BUTTON_B)) { saveWrite(); title_enter(); }
}

/* ==================================================================== NOM */
static const char* const gridRow[3] = { "ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "0123456789-. " };
static u16 gx, gy;
static char editName[NAME_LEN + 1];

static void nameDraw(void)
{
    char b[24];
    u16 r, c, n = slen(editName);
    for (c = 0; c < NAME_LEN; c++) b[c] = (c < n) ? editName[c] : '_';
    b[NAME_LEN] = 0;
    gtxtC(b, 8);
    for (r = 0; r < 3; r++)
        for (c = 0; c < 13; c++)
        {
            char ch[2] = { gridRow[r][c], 0 };
            if (ch[0] == ' ') ch[0] = '_';
            if (gy == r && gx == c) gtxt(ch, 7 + c * 2, 11 + r * 2);
            else wtxt(ch, 7 + c * 2, 11 + r * 2);
        }
    if (gy == 3 && gx == 0) gtxt("EFFACER", 9, 18); else wtxt("EFFACER", 9, 18);
    if (gy == 3 && gx == 1) gtxt("VALIDER", 24, 18); else wtxt("VALIDER", 24, 18);
}

static void nameValidate(void)
{
    u16 n = slen(editName);
    while (n > 0 && editName[n - 1] == ' ') editName[--n] = 0;
    scopy(playerName, n ? editName : "JOUEUR 1", NAME_LEN);
    saveWrite();
    sfx(990, 8);
    options_enter();
}

void name_enter(void)
{
    const char* def = "JOUEUR 1";
    u16 k = 0;
    gameMode = MODE_NAME;
    enterScreen(&bg_menu);
    gtxtC("NOM DU JOUEUR 1", 5);
    wtxtC("A:AJOUTER  B:EFFACER  START:VALIDER", 22);
    scopy(editName, playerName, NAME_LEN);
    while (def[k] && editName[k] == def[k]) k++;
    if (def[k] == 0 && editName[k] == 0) editName[0] = 0;
    gx = gy = 0;
    nameDraw();
    fadeInScreen(12);
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
static u16 sPhase;          /* 0 joueur 1 choisit, 1 adversaire, 2 termine */
static u16 sCur[2];
static u16 mugBase, portBase;

static void cellPos(u16 gid, u16* x, u16* y)
{
    *x = 3 + (gid / FIGHTERS_PER_CAT) * 4;
    *y = 2 + (gid % FIGHTERS_PER_CAT) * 4;
}

static void drawMug(u16 gid)
{
    u16 x, y, tx, ty;
    cellPos(gid, &x, &y);
    for (ty = 0; ty < 3; ty++)
        for (tx = 0; tx < 3; tx++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, mugBase + gid * 9 + ty * 3 + tx), x + tx, y + ty);
}

static void drawCursors(void)
{
    u16 g, x, y;
    for (g = 0; g < NUM_FIGHTERS; g++)
    {
        cellPos(g, &x, &y);
        VDP_clearText(x - 1, y + 1, 1);
        VDP_clearText(x + 3, y + 1, 1);
        VDP_clearText(x, y + 3, 3);
    }
    cellPos(sCur[0], &x, &y);
    if ((frameCount & 16) || sPhase > 0) { gtxt(">", x - 1, y + 1); gtxt("<", x + 3, y + 1); }
    if (sPhase >= 1)
    {
        cellPos(sCur[1], &x, &y);
        if ((frameCount & 16) || sPhase > 1) wtxt("^2^", x, y + 3);
    }
}

static void drawCard(u16 side)
{
    const FighterDef* f = fighterAt(sCur[side]);
    static const char* const lab[ST_COUNT] = { "GARDE   ", "PASSAGE ", "LUTTE   ", "SOUMIS. ", "CARDIO  " };
    char b[24];
    u16 s, k, x = side ? 23 : 10;
    u16 cat = sCur[side] / FIGHTERS_PER_CAT;

    clearRows(15, 15);
    b[0] = 0;
    scat(b, "< ", 23);
    scat(b, roster[sCur[sPhase > 0 ? 1 : 0] / FIGHTERS_PER_CAT].name, 23);
    scat(b, " ", 23);
    scat(b, roster[sCur[sPhase > 0 ? 1 : 0] / FIGHTERS_PER_CAT].weight, 23);
    scat(b, " >", 23);
    gtxtC(b, 15);
    (void)cat;
    VDP_clearTextArea(x, 17, 15, 9);
    gtxt(f->name, x, 17);
    wtxt(f->nick, x, 18);
    for (s = 0; s < ST_COUNT; s++)
    {
        b[0] = 0;
        scat(b, lab[s] + 0, 23);
        b[5] = 0;
        wtxt(b, x, 19 + s);
        for (k = 0; k < 9; k++) b[k] = (k < f->stat[s]) ? '#' : '-';
        b[9] = 0;
        gtxt(b, x + 6, 19 + s);
    }
    wtxt(subFamilyName[f->special], x, 24);
    flagAt(f->flag, x + 13, 17);
}

static void drawSelPortrait(u16 side)
{
    fighterSel[side] = sCur[side];
    P[side].def = fighterAt(sCur[side]);
    P[side].gi = P[side].def->gi;
    if (side == 1 && P[1].gi == P[0].gi) P[1].gi = (P[0].gi == GI_WHITE) ? GI_BLUE : GI_WHITE;
    applyFighterPalettes();
    drawPortrait(side, sCur[side], side ? 31 : 1, 16, portBase + side * 72);
}

void select_enter(void)
{
    u16 g;
    gameMode = MODE_SELECT;
    enterScreen(&bg_menu);
    setPalette(0, mugPal, 16);
    mugBase = screenTilesEnd();
    VDP_loadTileSet(&mugshots, mugBase, DMA);
    portBase = mugBase + NUM_FIGHTERS * 9;
    for (g = 0; g < NUM_FIGHTERS; g++) drawMug(g);
    sPhase = 0;
    if (sCur[0] >= NUM_FIGHTERS) sCur[0] = 0;
    sCur[1] = (sCur[0] + 4) % NUM_FIGHTERS;
    gtxtC(gameType == GAME_ARCADE ? "CHAMPIONNAT - CHOISIS TON COMBATTANT" :
          gameType == GAME_VERSUS ? "2 JOUEURS - CHOISISSEZ" : "COMBAT LIBRE - CHOISIS", 0);
    drawSelPortrait(0);
    drawCard(0);
    drawCursors();
    fadeInScreen(12);
}

static void moveCur(u16 side, u16 btn)
{
    u16 c = sCur[side] / FIGHTERS_PER_CAT, r = sCur[side] % FIGHTERS_PER_CAT;
    if (btn & BUTTON_LEFT) c = (c + NUM_CATS - 1) % NUM_CATS;
    if (btn & BUTTON_RIGHT) c = (c + 1) % NUM_CATS;
    if (btn & BUTTON_UP) r = (r + FIGHTERS_PER_CAT - 1) % FIGHTERS_PER_CAT;
    if (btn & BUTTON_DOWN) r = (r + 1) % FIGHTERS_PER_CAT;
    sCur[side] = c * FIGHTERS_PER_CAT + r;
    sfx(600, 3);
    drawSelPortrait(side);
    drawCard(side);
}

static void buildLadder(void)
{
    u16 i, k, used[NUM_FIGHTERS];
    for (k = 0; k < NUM_FIGHTERS; k++) used[k] = 0;
    used[fighterSel[0]] = 1;
    for (i = 0; i < LADDER_LEN; i++)
    {
        u16 g;
        if (i == LADDER_LEN - 1)
        {
            /* finale : le plus complet du roster (hors joueur) */
            u16 best = 0, bs = 0;
            for (g = 0; g < NUM_FIGHTERS; g++)
            {
                const FighterDef* f = fighterAt(g);
                u16 s = f->stat[0] + f->stat[1] + f->stat[2] + f->stat[3] + f->stat[4];
                if (!used[g] && s > bs) { bs = s; best = g; }
            }
            g = best;
        }
        else
        {
            do { g = random() % NUM_FIGHTERS; } while (used[g]);
        }
        used[g] = 1;
        ladder[i] = g;
    }
    ladderPos = 0;
    score = 0;
    continues = 3;
}

void select_update(void)
{
    u16 btn;
    if ((frameCount & 15) == 0) drawCursors();
    if (sPhase == 0)
    {
        btn = 0;
        if (mpressed(BUTTON_LEFT)) btn |= BUTTON_LEFT;
        if (mpressed(BUTTON_RIGHT)) btn |= BUTTON_RIGHT;
        if (mpressed(BUTTON_UP)) btn |= BUTTON_UP;
        if (mpressed(BUTTON_DOWN)) btn |= BUTTON_DOWN;
        if (btn) { moveCur(0, btn); drawCursors(); }
        if (mpressed(BUTTON_B)) { title_enter(); return; }
        if (mpressed(BUTTON_A) || mpressed(BUTTON_START))
        {
            sfx(880, 8);
            fighterSel[0] = sCur[0];
            if (gameType == GAME_ARCADE)
            {
                buildLadder();
                setupPlayers(1);
                ladder_enter();
                return;
            }
            sPhase = 1;
            drawSelPortrait(1);
            drawCard(1);
            drawCursors();
            gtxtC(gameType == GAME_VERSUS ? "JOUEUR 2 : CHOISIS (MANETTE 2)" : "CHOISIS TON ADVERSAIRE", 0);
        }
        return;
    }
    if (sPhase == 1)
    {
        /* 2 joueurs : manette 2 ; combat libre : manette 1 */
        u16 joy = (gameType == GAME_VERSUS) ? JOY_2 : JOY_1;
        static u16 prev2;
        u16 pad = JOY_readJoypad(joy);
        u16 pr = pad & ~prev2;
        prev2 = pad;
        btn = pr & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN);
        if (btn) { moveCur(1, btn); drawCursors(); }
        if (mpressed(BUTTON_B) && gameType != GAME_VERSUS) { sPhase = 0; drawCursors(); return; }
        if (pr & (BUTTON_A | BUTTON_START))
        {
            sfx(880, 8);
            fighterSel[1] = sCur[1];
            stageIdx = random() % 3;
            setupPlayers(gameType == GAME_VERSUS ? 2 : 1);
            sPhase = 2;
            versus_enter();
        }
    }
}

/* ==================================================================== CHAMPIONNAT */
static const char* const stageNames[3] = { "ARENA MUNDIAL", "ACADEMIA", "PRAIA DE COPACABANA" };

static u16 ladderStage(u16 i) { return (i < 2) ? 1 : (i < 4 ? 2 : 0); }

void ladder_enter(void)
{
    u16 i;
    char b[41];
    gameMode = MODE_LADDER;
    enterScreen(&bg_menu);
    gtxtC("CHAMPIONNAT ABSOLU", 3);
    for (i = 0; i < LADDER_LEN; i++)
    {
        const FighterDef* f = fighterAt(ladder[i]);
        u16 y = 6 + i * 3;
        b[0] = 0;
        scat(b, i == LADDER_LEN - 1 ? "FINALE" : "COMBAT ", 40);
        if (i < LADDER_LEN - 1) { char n[2] = { '1' + i, 0 }; scat(b, n, 40); }
        if (i == ladderPos) gtxt(b, 4, y); else wtxt(b, 4, y);
        flagAt(f->flag, 14, y);
        if (i == ladderPos) gtxt(f->name, 17, y); else wtxt(f->name, 17, y);
        if (i < ladderPos) gtxt("OK", 34, y);
        if (i == ladderPos) wtxt(stageNames[ladderStage(i)], 17, y + 1);
    }
    b[0] = 0;
    scat(b, "SCORE ", 40);
    {
        char n[8];
        numStr(score, 7, n);
        scat(b, n, 40);
    }
    gtxtC(b, 25);
    timer = 0;
    fadeInScreen(12);
}

void ladder_update(void)
{
    if ((frameCount & 31) == 0) wtxtC("START : COMBATTRE", 23);
    if ((frameCount & 31) == 16) clearRow(23);
    if (mpressed(BUTTON_START) || mpressed(BUTTON_A))
    {
        fighterSel[1] = ladder[ladderPos];
        stageIdx = ladderStage(ladderPos);
        setupPlayers(1);
        versus_enter();
    }
}

/* ==================================================================== VERSUS */
void versus_enter(void)
{
    u16 base;
    char b[41];
    gameMode = MODE_VERSUS;
    enterScreen(&bg_vs);
    applyFighterPalettes();
    base = screenTilesEnd();
    drawPortrait(0, fighterSel[0], 3, 3, base);
    drawPortrait(1, fighterSel[1], 29, 13, base + 72);
    gtxt(P[0].def->name, 2, 13);
    wtxt(P[0].def->nick, 2, 14);
    flagAt(P[0].def->flag, 2, 16);
    gtxt(P[0].label, 5, 16);
    gtxt(P[1].def->name, 38 - slen(P[1].def->name), 23);
    wtxt(P[1].def->nick, 38 - slen(P[1].def->nick), 24);
    flagAt(P[1].def->flag, 36, 9);
    gtxt(P[1].label, 35 - slen(P[1].label), 10);
    b[0] = 0;
    scat(b, stageNames[stageIdx], 40);
    wtxtC(b, 26);
    if (gameType == GAME_ARCADE && ladderPos == LADDER_LEN - 1) gtxtC("FINALE !", 2);
    timer = 200;
    sfx(440, 10);
    fadeInScreen(12);
}

void versus_update(void)
{
    if (--timer <= 0 || mpressed(BUTTON_START) || mpressed(BUTTON_A))
    {
        fadeOut(16);
        musicStop();
        SPR_reset();
        match_enter();
    }
}

/* ==================================================================== RESULTAT */
static u32 lastGain;
static bool perfect;

void result_enter(void)
{
    char b[41];
    bool humanWon = resWinner >= 0 && P[resWinner].human;
    gameMode = MODE_RESULT;
    enterScreen(&bg_menu);
    applyFighterPalettes();
    if (resWinner >= 0)
    {
        drawPortrait(resWinner, fighterSel[resWinner], 16, 3, screenTilesEnd());
        b[0] = 0;
        scat(b, P[resWinner].label, 40);
        scat(b, " GAGNE !", 40);
        gtxtC(b, 13);
        wtxtC(P[resWinner].def->name, 14);
    }
    else gtxtC("MATCH NUL", 13);
    wtxtC(resMethod, 16);
    {
        char n0[4], n1[4];
        numStr(P[0].points, 2, n0);
        numStr(P[1].points, 2, n1);
        b[0] = 0;
        scat(b, P[0].label, 40);
        scat(b, "  ", 40);
        scat(b, n0, 40);
        scat(b, " - ", 40);
        scat(b, n1, 40);
        scat(b, "  ", 40);
        scat(b, P[1].label, 40);
        wtxtC(b, 18);
    }
    lastGain = 0;
    perfect = FALSE;
    if (gameType != GAME_DEMO && P[0].human)
    {
        stats.matches++;
        if (resWinner == 0)
        {
            stats.wins++;
            stats.streak++;
            if (stats.streak > stats.bestStreak) stats.bestStreak = stats.streak;
            if (resBySub) stats.subs++;
        }
        else stats.streak = 0;
    }
    if (gameType == GAME_ARCADE && humanWon)
    {
        u32 g = 1000 + P[0].points * 100 + P[0].adv * 50 + (resBySub ? 3000 : 0) + resTimeLeft * 10;
        if (P[1].points == 0) { g += 2000; perfect = TRUE; }
        g = g * (2 + difficulty) / 2;
        lastGain = g;
        score += g;
        b[0] = 0;
        scat(b, "BONUS +", 40);
        {
            char n[8];
            numStr(g, 5, n);
            scat(b, n, 40);
        }
        gtxtC(b, 21);
        if (perfect) gtxtC("PERFEITO ! +2000", 22);
    }
    saveWrite();
    timer = 0;
    if (gameType == GAME_ARCADE) wtxtC(humanWon ? "A : SUITE" : "A : CONTINUER", 25);
    else if (gameType == GAME_DEMO) wtxtC("DEMO", 25);
    else wtxtC("A : REVANCHE    START : MENU", 25);
    fadeInScreen(12);
}

void result_update(void)
{
    timer++;
    if (gameType == GAME_DEMO)
    {
        if (timer > 240 || mpressed(BUTTON_START)) title_enter();
        return;
    }
    if (timer < 30) return;
    if (gameType == GAME_ARCADE)
    {
        if (!(mpressed(BUTTON_A) || mpressed(BUTTON_START))) return;
        if (resWinner == 0)
        {
            ladderPos++;
            if (ladderPos >= LADDER_LEN) ending_enter();
            else ladder_enter();
        }
        else continue_enter();
        return;
    }
    if (mpressed(BUTTON_A)) { fadeOut(16); SPR_reset(); match_enter(); }
    else if (mpressed(BUTTON_START)) title_enter();
}

/* ==================================================================== CONTINUE */
void continue_enter(void)
{
    gameMode = MODE_CONTINUE;
    enterScreen(&bg_menu);
    gtxtC("CONTINUER ?", 9);
    timer = 10 * 60 - 1;
    fadeInScreen(12);
}

static void gameOver(void)
{
    s16 r = hiscoreInsert(score, fighterSel[0], ladderPos);
    saveWrite();
    (void)r;
    records_enter();
}

void continue_update(void)
{
    char b[4];
    if ((timer % 60) == 59)
    {
        numStr(timer / 60, 1, b);
        gtxtC(b, 13);
        sfx(300, 6);
    }
    if (mpressed(BUTTON_START) || mpressed(BUTTON_A))
    {
        score = score / 2;
        ladder_enter();
        return;
    }
    if (mpressed(BUTTON_B)) timer -= timer % 60 + 1;
    if (--timer < 0) { gtxtC("FIN DU CHAMPIONNAT", 13); gameOver(); }
}

/* ==================================================================== SACRE */
void ending_enter(void)
{
    char b[41], n[8];
    gameMode = MODE_ENDING;
    enterScreen(&bg_menu);
    applyFighterPalettes();
    drawPortrait(0, fighterSel[0], 16, 3, screenTilesEnd());
    gtxtC("CAMPEAO ABSOLUTO !", 13);
    b[0] = 0;
    scat(b, P[0].label, 40);
    scat(b, " - ", 40);
    scat(b, P[0].def->name, 40);
    wtxtC(b, 15);
    wtxtC("MEDAILLE D'OR ET CEINTURE DE CHAMPION", 17);
    numStr(score, 7, n);
    b[0] = 0;
    scat(b, "SCORE FINAL ", 40);
    scat(b, n, 40);
    gtxtC(b, 20);
    stats.champs++;
    hiscoreInsert(score, fighterSel[0], LADDER_LEN);
    saveWrite();
    playTune(tuneWin, 6);
    timer = 0;
    fadeInScreen(20);
}

void ending_update(void)
{
    timer++;
    if ((timer & 31) == 0) wtxtC("OSS ! MERCI D'AVOIR JOUE", 24);
    if ((timer & 31) == 16) clearRow(24);
    if (timer > 60 && (mpressed(BUTTON_START) || mpressed(BUTTON_A))) records_enter();
}

/* ==================================================================== RECORDS */
void records_enter(void)
{
    u16 i;
    char b[41], n[8];
    gameMode = MODE_RECORDS;
    enterScreen(&bg_menu);
    gtxtC("TABLEAU DES SCORES", 3);
    for (i = 0; i < HISCORES; i++)
    {
        u16 y = 6 + i * 2;
        b[0] = 0;
        n[0] = '1' + i;
        n[1] = '.';
        n[2] = 0;
        scat(b, n, 40);
        if (i == 0) gtxt(b, 4, y); else wtxt(b, 4, y);
        if (i == 0) gtxt(hiscores[i].name, 8, y); else wtxt(hiscores[i].name, 8, y);
        numStr(hiscores[i].score, 7, n);
        gtxt(n, 18, y);
        wtxt(fighterAt(hiscores[i].fighter)->name, 26, y);
    }
    b[0] = 0;
    {
        char a[6], c[6], d[6], e[6];
        numStr(stats.matches, 3, a);
        numStr(stats.wins, 3, c);
        numStr(stats.subs, 3, d);
        numStr(stats.champs, 2, e);
        scat(b, "COMBATS ", 40); scat(b, a, 40);
        scat(b, "  VICTOIRES ", 40); scat(b, c, 40);
        wtxtC(b, 23);
        b[0] = 0;
        scat(b, "SOUMISSIONS ", 40); scat(b, d, 40);
        scat(b, "  TITRES ", 40); scat(b, e, 40);
        wtxtC(b, 24);
        numStr(stats.bestStreak, 2, a);
        b[0] = 0;
        scat(b, "MEILLEURE SERIE ", 40); scat(b, a, 40);
        wtxtC(b, 25);
    }
    timer = 0;
    fadeInScreen(12);
}

void records_update(void)
{
    if (++timer > 600 || (timer > 20 && anyMenuKey())) title_enter();
}
