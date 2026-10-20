#ifndef _GAME_H_
#define _GAME_H_

#include <genesis.h>
#include "resources.h"
#include "gen/anim_data.h"
#include "gen/ui_data.h"

/* ------------------------------------------------------------------ VRAM (index de tuiles) */
#define BG_BASE         TILE_USER_INDEX     /* decor de l'ecran courant (560 tuiles max) */
#define HUD_BASE        576
#define WFONT_BASE      720                 /* police blanche (96 tuiles) */
#define BAR_BASE        816                 /* jauges dynamiques (2 x 16 tuiles) */
#define SLOT_BASE       848                 /* 3 emplacements de 64 tuiles pour les combattants */
#define SLOT_SIZE       64
#define SPR_VRAM        360                 /* moteur de sprites (arbitre, effets, bannieres...) */

/* ------------------------------------------------------------------ ecran / monde */
#define SCREEN_W        320
#define FLOOR_SCREEN    200                 /* ligne du sol a l'ecran */
#define WORLD_W         512
#define CAM_MAX         (WORLD_W - SCREEN_W)
#define MAT_MIN         40
#define MAT_MAX         472

/* ------------------------------------------------------------------ roster */
#define NUM_CATS            9
#define FIGHTERS_PER_CAT    3
#define NUM_FIGHTERS        (NUM_CATS * FIGHTERS_PER_CAT)
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
    u8 gi, skin, hair, beard, accent, flag;
} FighterDef;

typedef struct
{
    const char* name;
    const char* weight;
    FighterDef f[FIGHTERS_PER_CAT];
} Category;

extern const Category roster[NUM_CATS];
extern const char* const subFamilyName[5];
extern const Image* const portraits[NUM_FIGHTERS];
const FighterDef* fighterAt(u16 gid);

/* ------------------------------------------------------------------ modes */
enum { MODE_TITLE, MODE_NAME, MODE_SELECT, MODE_LADDER, MODE_VERSUS, MODE_MATCH, MODE_RESULT,
       MODE_CONTINUE, MODE_ENDING, MODE_RECORDS, MODE_OPTIONS };
enum { GAME_ARCADE, GAME_VERSUS, GAME_FREE, GAME_DEMO };

#define BTN_ACTS    (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_START)

typedef struct
{
    const FighterDef* def;
    u16 gid;                    /* index dans le roster */
    char label[NAME_LEN + 1];
    bool human;
    u16 joy;
    u16 pad, prev;
    u8 gi;
    u16 pal[16];

    /* combat */
    s16 x;                      /* position monde (bassin) */
    s16 dir;                    /* +1 regarde a droite */
    s16 souffle, cardio;
    s16 points, adv, pen;
    u16 attempts;
    u16 act;                    /* action debout */
    s16 actTimer, cooldown;
    s16 grip;
    u16 tdType;
    u16 defKind;
    s16 defTimer;
    s16 idle;
    s16 cur;                    /* curseur du mini-jeu de soumission */
    u16 anim;                   /* animation solo courante */
    s16 animFrame, animTick;
    bool animRev;
    s16 sweat;
    bool animHold;              /* animation figee (temps de preparation d'une attaque) */

    /* arcade : telegraphie, contres, super */
    s16 superG;                 /* jauge de super technique (0..100) */
    s16 wind, windMax;          /* preparation d'une attaque debout (visible par l'adversaire) */
    bool sure;                  /* attaque imparable (ouverture / super) */
    s16 opening;                /* ouverture : la prochaine technique part instantanement */
    s16 stun;                   /* sonne apres un contre */
    s16 stunNext;               /* sonne a appliquer au retour debout */

    /* IA */
    u16 aiWant;
    s16 aiTimer, aiTarget, aiDenyAt;
    u16 aiDenyBtn;
    s16 aiMoveDir;
} Player;

extern Player P[2];
extern u16 gameMode, gameType;
extern u16 frameCount;
extern u16 difficulty;
extern u16 matchMinutes;
extern bool helpOn;
extern u16 numHumans;
extern u16 fighterSel[2];           /* index roster global */
extern char playerName[NAME_LEN + 1];
extern u16 stageIdx;

/* championnat */
#define LADDER_LEN 6
extern u16 ladder[LADDER_LEN];
extern u16 ladderPos;
extern u32 score;
extern u16 continues;

/* resultat du dernier combat */
extern s16 resWinner;
extern char resMethod[32];
extern bool resBySub;
extern s16 resTimeLeft;

/* ------------------------------------------------------------------ texte (text.c) */
u16  slen(const char* s);
void scopy(char* dst, const char* src, u16 max);
void scat(char* dst, const char* src, u16 max);
void numStr(s32 v, u16 digits, char* out);
void gtxt(const char* s, u16 x, u16 y);             /* police doree */
void wtxt(const char* s, u16 x, u16 y);             /* police blanche */
void gtxtC(const char* s, u16 y);
void wtxtC(const char* s, u16 y);
void clearRow(u16 y);
void clearRows(u16 y0, u16 y1);
void putTile(u16 x, u16 y, u16 tile);               /* tuile HUD sur le plan A, PAL3, priorite haute */
void fillPanel(u16 x, u16 y, u16 w, u16 h);

/* ------------------------------------------------------------------ graphismes (gfx.c) */
void gfxInit(void);
void loadScreenImage(const Image* img, bool pal0Only);
u16  screenTilesEnd(void);
void setPal3(void);
void setPalette(u16 index, const u16* cols, u16 n);
void buildFighterPalette(u16* pal, const FighterDef* f, u8 gi);
void applyFighterPalettes(void);
void drawPortrait(u16 who, u16 gid, u16 x, u16 y, u16 tileBase);
void fadeOut(u16 frames);
void fighterTint(u16 who, u16 mode);     /* 0 normal, 1 eclairci (attaque qui arrive), 2 blanc (choc), 3 dore (super) */
void stageDim(bool on);
void gfxFlash(u16 frames);
void gfxUpdate(void);
#define PORTRAIT_TILE   488
void fadeInScreen(u16 frames);

/* scene de combat (stage.c) */
void stageLoad(u16 idx);
void stageUpdate(void);
void stageSetCamera(s16 target);
s16  camX(void);
void stageCheer(s16 frames);
void stageShake(s16 frames);
s16  shakeY(void);

/* HUD (hud.c) */
void hudInit(void);
void hudDraw(s32 framesLeft, bool full);
void hudMessage(const char* s, u16 frames);
void hudMessageWho(u16 who, const char* s, u16 pts);
void hudUpdate(void);
void hudHints(const char* const* lines, u16 n);
void hudClearHints(void);
void hudSubGauge(const char* name, s16 tug);
void hudSubClear(void);

/* effets, arbitre, bannieres (fx.c) */
void fxInit(void);
void fxRelease(void);
void fxUpdate(void);
void fxBanner(u16 ban, u16 frames);
bool fxBannerActive(void);
void fxPop(u16 kind, s16 wx, s16 wy);
void fxImpact(s16 wx, s16 wy, bool big);
void fxDust(s16 wx, s16 wy);
void fxPrompt(u16 slot, s16 pr, s16 wx, s16 wy);    /* pr < 0 : cache l'invite */
void refInit(s16 wx);
void refSignal(u16 anim, s16 faceToX, u16 frames);
void refFollow(s16 cx);
void refHide(void);
void refShow(void);

/* combattants a l'ecran (actors.c) */
void actorsInit(void);
void actorsRelease(void);
void actorsSoloMode(void);
void actorsSoloSet(u16 i, u16 anim, bool restart);
void actorsSoloReverse(u16 i, bool rev);
void actorsSoloUpdate(void);
bool actorsSoloDone(u16 i);
void actorsPairStart(u16 seq, s16 anchorX, bool flip, u16 roleA, bool reverse);
bool actorsPairUpdate(void);         /* TRUE quand la sequence est finie */
u16  actorsPairSeq(void);
s16  actorsPairFrame(void);
void actorsPairWorldPos(u16 role, s16* wx, s16* wy);
s16  actorsPairAnchor(void);
bool actorsPairFlip(void);
void actorsShake(s16 frames);
bool actorsImpact(void);             /* TRUE une fois apres une image de chute (pour l'arret sur image) */
void actorsShow(bool show);
void actorsShadows(bool show);
void actorsDraw(void);

/* ------------------------------------------------------------------ manettes */
void readPads(void);
bool pressed(u16 i, u16 b);
bool held(u16 i, u16 b);
void readMenuPad(void);
bool mpressed(u16 b);
bool mheld(u16 b);
bool anyMenuKey(void);

/* ------------------------------------------------------------------ son (sound.c) */
void sfx(u16 freq, s16 frames);
void sfxSlam(void);
void sfxNoise(s16 frames);
void playTune(const u16* t, u16 len);
void soundUpdate(void);
void musicTitleStart(void);
void musicUpdate(void);
void musicStop(void);
void crowdStart(void);
void crowdStop(void);
void crowdReact(bool big);
void crowdUpdate(bool tense);
/* voix de l'arbitre (meme ordre que les bannieres BAN_*, puis les annonces de points) */
enum { VO_OSS, VO_COMBATE, VO_FINALIZOU, VO_TEMPO, VO_VITORIA, VO_EMPATE, VO_DESCLASSIFICADO, VO_PERFEITO, VO_CAMPEAO, VO_FINAL, VO_PRONTO, VO_PAROU, VO_CONTRA, VO_ESPECIAL, VO_P2, VO_P3, VO_P4, VO_VANTAGEM, VO_PUNICAO, VO_LUTEM, VO_COUNT };
void voiceSay(u16 v);
extern const u16 tuneWin[6];
extern const u16 tuneLose[4];
extern const u16 tuneStart[3];

/* ------------------------------------------------------------------ sauvegarde (save.c) */
#define HISCORES 8
typedef struct { char name[NAME_LEN + 1]; u32 score; u8 fighter; u8 stage; } HiScore;
typedef struct
{
    u16 matches, wins, subs, champs;
    u16 bestStreak, streak;
} Stats;
extern HiScore hiscores[HISCORES];
extern Stats stats;
void saveLoad(void);
void saveWrite(void);
s16  hiscoreInsert(u32 sc, u16 fighter, u16 stage);

/* ------------------------------------------------------------------ ecrans */
void title_enter(void);
void title_update(void);
void name_enter(void);
void name_update(void);
void options_enter(void);
void options_update(void);
void select_enter(void);
void select_update(void);
void ladder_enter(void);
void ladder_update(void);
void versus_enter(void);
void versus_update(void);
void result_enter(void);
void result_update(void);
void continue_enter(void);
void continue_update(void);
void ending_enter(void);
void ending_update(void);
void records_enter(void);
void records_update(void);

void match_enter(void);
void match_update(void);
void matchLeave(void);

#endif
