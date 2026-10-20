/*
 * BJJ TATAMI - jeu de jiu-jitsu bresilien pour Mega Drive (SGDK 2.11)
 *
 * main.c    : initialisation et boucle principale
 * match.c   : le combat (debout, sol, soumissions, IA, penalites)
 * actors.c  : affichage des combattants (sprites solo / calques des animations en paire)
 * stage.c   : arenes, camera, parallaxe, public anime
 * hud.c     : tableau de score, jauges, messages, aides
 * fx.c      : bannieres, points, impacts, arbitre
 * screens.c : titre, options, selection, championnat, versus, resultat, records
 * gfx.c, text.c, sound.c, save.c, roster.c : outils
 * gen/      : tables generees par tools/art (animations, tuiles d'interface, palettes)
 */
#include "game.h"

Player P[2];
u16 gameMode, gameType;
u16 frameCount;
u16 difficulty = 1;
u16 matchMinutes = 3;
bool helpOn = TRUE;
u16 numHumans = 1;
u16 fighterSel[2] = { 9, 13 };
char playerName[NAME_LEN + 1];
u16 stageIdx;
u16 ladder[LADDER_LEN];
u16 ladderPos;
u32 score;
u16 continues;
s16 resWinner;
char resMethod[32];
bool resBySub;
s16 resTimeLeft;

int main(bool hardReset)
{
    u16 i;
    (void)hardReset;
    gfxInit();
    PSG_reset();
    saveLoad();
    for (i = 0; i < 2; i++)
    {
        P[i].gid = fighterSel[i];
        P[i].def = fighterAt(fighterSel[i]);
        P[i].gi = P[i].def->gi;
        P[i].joy = i ? JOY_2 : JOY_1;
    }
    title_enter();

    while (TRUE)
    {
        frameCount++;
        readPads();
        readMenuPad();
        switch (gameMode)
        {
            case MODE_TITLE:    title_update();    break;
            case MODE_NAME:     name_update();     break;
            case MODE_OPTIONS:  options_update();  break;
            case MODE_SELECT:   select_update();   break;
            case MODE_LADDER:   ladder_update();   break;
            case MODE_VERSUS:   versus_update();   break;
            case MODE_MATCH:
                if (gameType == GAME_DEMO && mpressed(BUTTON_START)) { matchLeave(); title_enter(); }
                else match_update();
                break;
            case MODE_RESULT:   result_update();   break;
            case MODE_CONTINUE: continue_update(); break;
            case MODE_ENDING:   ending_update();   break;
            case MODE_RECORDS:  records_update();  break;
        }
        musicUpdate();
        soundUpdate();
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
