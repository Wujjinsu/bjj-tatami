#ifndef _GAME_H_
#define _GAME_H_

#include <genesis.h>
#include "resources.h"

/* ------------------------------------------------------------------ textes */
/* La police SGDK utilise la couleur 15 de la palette choisie */
#define TXT_WHITE       PAL0
#define TXT_DARK        PAL1
#define TXT_RED         PAL2
#define TXT_GREEN       PAL3

/* ------------------------------------------------------------------ roster */
#define NUM_CATS            9
#define FIGHTERS_PER_CAT    3
#define NAME_LEN            8

enum { ST_GARDE, ST_PASSAGE, ST_LUTTE, ST_SOUM, ST_CARDIO, ST_COUNT };
enum { SUBF_ARM, SUBF_TRI, SUBF_KIM, SUBF_CHOKE, SUBF_LEG };
enum { GI_WHITE, GI_BLUE, GI_BLACK };
enum { SKIN_LIGHT, SKIN_MED, SKIN_DARK };
enum { HAIR_BLACK, HAIR_BROWN, HAIR_BLOND, HAIR_BALD };

typedef struct
{
    const char* name;
    const char* nick;
    u8 stat[ST_COUNT];
    u8 special;
    u8 gi, skin, hair;
} FighterDef;

typedef struct
{
    const char* name;
    const char* weight;
    FighterDef f[FIGHTERS_PER_CAT];
} Category;

extern const Category roster[NUM_CATS];
extern const char* const subFamilyName[5];

/* ------------------------------------------------------------------ etat du jeu */
enum { MODE_TITLE, MODE_NAME, MODE_SELECT, MODE_VERSUS, MODE_MATCH, MODE_RESULT };

/* directions "lues" au sol et pour les defenses */
enum { DIR_UP, DIR_SIDE, DIR_DOWN };

#define BTN_DIRS    (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)
#define BTN_ACTS    (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_START)

typedef struct
{
    const FighterDef* def;
    char label[NAME_LEN + 1];   /* nom affiche : nom du joueur ou "CPU" */
    bool human;
    u16 joy;
    u16 pad, prev;
    u8 gi;                      /* couleur de kimono reellement utilisee */

    /* combat */
    s16 x;
    s16 souffle, cardio;        /* 0..100 ; le souffle ne remonte pas au-dessus du cardio */
    s16 points, adv, pen;
    u16 attempts;
    u16 act;
    s16 actTimer, cooldown;
    u16 walkTick;
    bool walking;
    s16 grip;
    u16 tdType;
    u16 defKind;
    s16 idle;
    s16 cur;                    /* curseur du mini-jeu de soumission (1/16 de case) */

    /* IA */
    u16 aiWant;                 /* boutons a "appuyer" des que possible */
    s16 aiTimer, aiReact, aiTarget, aiDenyAt;
    u16 aiDenyBtn;
    s16 aiMoveDir;

    Sprite* spr;
    u16 pal[16];
} Player;

extern Player P[2];
extern u16 gameMode;
extern u16 frameCount;
extern u16 difficulty;          /* 0 facile, 1 normal, 2 difficile */
extern u16 matchMinutes;
extern u16 numHumans;           /* 1, 2, ou 0 = demonstration */
extern u16 catIdx;
extern u16 fighterIdx[2];
extern char playerName[NAME_LEN + 1];
extern Sprite* groundSpr;

/* resultat du dernier combat */
extern s16 resWinner;           /* 0, 1, ou -1 = egalite */
extern char resMethod[32];

/* ------------------------------------------------------------------ ui.c */
u16  slen(const char* s);
void scopy(char* dst, const char* src, u16 max);
void txt(const char* s, u16 x, u16 y, u16 pal);
void txtCenter(const char* s, u16 y, u16 pal);
void clearRow(u16 y);
void clearRows(u16 y0, u16 y1);
void numStr(s16 v, u16 digits, char* out);
void drawNum(s16 v, u16 digits, u16 x, u16 y, u16 pal);
void statBar(u8 v, char* out);              /* 9 cases */

void showBackground(const Image* img);
void buildFighterPalette(u16* pal, const FighterDef* f, u8 gi, u16 textColor);
void applyPlayerPalettes(void);
void setGroundPalette(u16 top);
void hideAllSprites(void);

void readPads(void);
bool pressed(u16 i, u16 b);
bool held(u16 i, u16 b);
u16  dirOf(u16 i);
void readMenuPad(void);         /* manette 1 brute, pour les menus et la demo */
bool mpressed(u16 b);
bool mheld(u16 b);

void sfx(u16 freq, s16 frames);
void sfxSlam(void);
void playTune(const u16* t, u16 len);
void soundUpdate(void);
void musicTitleStart(void);
void musicUpdate(void);
void musicStop(void);
void crowdStart(void);
void crowdStop(void);
void crowdReact(bool big);
void crowdUpdate(bool tense);

void nameLoad(void);
void nameSave(void);

extern const u16 tuneWin[6];
extern const u16 tuneLose[4];
extern const u16 tuneStart[3];

/* ------------------------------------------------------------------ ecrans */
void title_enter(void);
void title_update(void);
void name_enter(void);
void name_update(void);
void select_enter(void);
void select_update(void);
void versus_enter(void);
void versus_update(void);
void result_enter(void);
void result_update(void);

void match_enter(void);
void match_update(void);
void updateFighterSprite(u16 i, s16 y);

#endif
