/*
 * BJJ TATAMI - jeu de jiu-jitsu bresilien pour Mega Drive (SGDK 2.11)
 *
 * main.c   : initialisation et boucle principale
 * screens.c: titre, nom, selection, versus, resultat
 * match.c  : le combat (debout, sol, soumissions, IA)
 * roster.c : les 27 combattants
 * ui.c     : textes, palettes, manettes, sons, sauvegarde
 */
#include "game.h"

Player P[2];
u16 gameMode;
u16 frameCount;
u16 difficulty = 1;
u16 matchMinutes = 3;
u16 numHumans = 1;
u16 catIdx = 5;
u16 fighterIdx[2] = { 0, 1 };
char playerName[NAME_LEN + 1];
s16 resWinner;
char resMethod[32];

int main(bool hardReset)
{
    u16 i;

    VDP_setScreenWidth320();
    SPR_init();
    PSG_reset();
    nameLoad();

    for (i = 0; i < 2; i++)
    {
        P[i].def = &roster[catIdx].f[fighterIdx[i]];
        P[i].gi = P[i].def->gi;
        P[i].joy = (i == 0) ? JOY_1 : JOY_2;
    }
    if (P[1].gi == P[0].gi) P[1].gi = (P[0].gi == GI_WHITE) ? GI_BLUE : GI_WHITE;

    P[0].spr = SPR_addSprite(&spr_fighter, 0, 154, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    P[1].spr = SPR_addSprite(&spr_fighter, 0, 154, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    groundSpr = SPR_addSprite(&spr_ground, 128, 162, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
    applyPlayerPalettes();

    title_enter();

    while (TRUE)
    {
        frameCount++;
        readPads();
        readMenuPad();

        switch (gameMode)
        {
            case MODE_TITLE:  title_update();  break;
            case MODE_NAME:   name_update();   break;
            case MODE_SELECT: select_update(); break;
            case MODE_VERSUS: versus_update(); break;
            case MODE_MATCH:
                if (numHumans == 0 && mpressed(BUTTON_START)) { crowdStop(); title_enter(); }
                else match_update();
                break;
            case MODE_RESULT: result_update(); break;
        }

        musicUpdate();
        soundUpdate();
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
