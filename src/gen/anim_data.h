/* fichier genere par tools/art/build_fighters.py - ne pas modifier */
#ifndef _ANIM_DATA_H_
#define _ANIM_DATA_H_
#include <genesis.h>

#define SOLO_W 128
#define SOLO_H 112
#define SOLO_X 56
#define PAIR_W 176
#define PAIR_H 112
#define PAIR_AX 88
#define FLOOR_Y 108

typedef struct { u8 ticks; s8 dx; } SoloFrame;
typedef struct { u8 n; u8 loop; const SoloFrame* f; } SoloAnim;
/* owners : bit k = 1 si le calque k appartient a B ; bits 3-4 : effet (1 impact, 2 poussiere) */
typedef struct { u8 ticks; u8 owners; u8 ax, ay, bx, by; } PairFrame;
typedef struct { u8 n; u8 loop; u8 endFlip; u8 swap; const PairFrame* f; } PairSeq;

enum {
    SOLO_IDLE,
    SOLO_WALK,
    SOLO_GRIP_REACH,
    SOLO_GRIP_HOLD,
    SOLO_SHOOT,
    SOLO_STUMBLE,
    SOLO_SPRAWL,
    SOLO_HIPBLOCK,
    SOLO_TIRED,
    SOLO_VICTORY,
    SOLO_DEFEAT,
    SOLO_BOW,
    SOLO_COUNT
};

enum {
    SEQ_G_GUARD,
    SEQ_G_HALF,
    SEQ_G_SIDE,
    SEQ_G_KNEE,
    SEQ_G_MOUNT,
    SEQ_G_BACK,
    SEQ_TD_DOUBLE,
    SEQ_TD_DOUBLE_SPRAWL,
    SEQ_TD_SINGLE,
    SEQ_TD_SINGLE_SPRAWL,
    SEQ_TD_JUDO,
    SEQ_TD_JUDO_BLOCK,
    SEQ_PULL_GUARD,
    SEQ_SHAKE,
    SEQ_T_GUARD_HALF,
    SEQ_T_GUARD_SIDE,
    SEQ_T_GUARD_STAND,
    SEQ_T_HALF_SIDE,
    SEQ_T_HALF_MOUNT,
    SEQ_T_HALF_STAND,
    SEQ_T_SIDE_MOUNT,
    SEQ_T_SIDE_KNEE,
    SEQ_T_SIDE_BACK,
    SEQ_T_KNEE_MOUNT,
    SEQ_T_KNEE_SIDE,
    SEQ_T_MOUNT_BACK,
    SEQ_B_GUARD_SWEEP,
    SEQ_B_GUARD_BACK,
    SEQ_B_GUARD_STAND,
    SEQ_B_HALF_GUARD,
    SEQ_B_HALF_SWEEP,
    SEQ_B_HALF_STAND,
    SEQ_B_SIDE_HALF,
    SEQ_B_SIDE_GUARD,
    SEQ_B_KNEE_HALF,
    SEQ_B_MOUNT_UPA,
    SEQ_B_MOUNT_HALF,
    SEQ_B_BACK_ROLL,
    SEQ_B_BACK_HALF,
    SEQ_S_ANKLE_SET,
    SEQ_S_ANKLE_LOCK,
    SEQ_S_ANKLE_TAP,
    SEQ_S_KIMURA_H_SET,
    SEQ_S_KIMURA_H_LOCK,
    SEQ_S_KIMURA_H_TAP,
    SEQ_S_DARCE_SET,
    SEQ_S_DARCE_LOCK,
    SEQ_S_DARCE_TAP,
    SEQ_S_KIMURA_S_SET,
    SEQ_S_KIMURA_S_LOCK,
    SEQ_S_KIMURA_S_TAP,
    SEQ_S_AMERICANA_SET,
    SEQ_S_AMERICANA_LOCK,
    SEQ_S_AMERICANA_TAP,
    SEQ_S_ARMBAR_K_SET,
    SEQ_S_ARMBAR_K_LOCK,
    SEQ_S_ARMBAR_K_TAP,
    SEQ_S_ARMBAR_M_SET,
    SEQ_S_ARMBAR_M_LOCK,
    SEQ_S_ARMBAR_M_TAP,
    SEQ_S_CROSS_SET,
    SEQ_S_CROSS_LOCK,
    SEQ_S_CROSS_TAP,
    SEQ_S_RNC_SET,
    SEQ_S_RNC_LOCK,
    SEQ_S_RNC_TAP,
    SEQ_S_BOWARROW_SET,
    SEQ_S_BOWARROW_LOCK,
    SEQ_S_BOWARROW_TAP,
    SEQ_S_TRIANGLE_SET,
    SEQ_S_TRIANGLE_LOCK,
    SEQ_S_TRIANGLE_TAP,
    SEQ_S_ARMBAR_G_SET,
    SEQ_S_ARMBAR_G_LOCK,
    SEQ_S_ARMBAR_G_TAP,
    SEQ_S_GUILLOTINE_SET,
    SEQ_S_GUILLOTINE_LOCK,
    SEQ_S_GUILLOTINE_TAP,
    SEQ_S_KIMURA_HB_SET,
    SEQ_S_KIMURA_HB_LOCK,
    SEQ_S_KIMURA_HB_TAP,
    SEQ_COUNT
};

extern const SoloAnim soloAnims[SOLO_COUNT];
extern const PairSeq pairSeqs[SEQ_COUNT];

#endif
