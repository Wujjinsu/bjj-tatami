/* fichier genere par tools/art/build_fighters.py - ne pas modifier */
#include "anim_data.h"

static const SoloFrame sf_IDLE[] = { {10,0}, {10,0}, {10,0}, {10,0} };
static const SoloFrame sf_WALK[] = { {6,0}, {6,0}, {6,0}, {6,0}, {6,0}, {6,0} };
static const SoloFrame sf_GRIP_REACH[] = { {5,0}, {5,0} };
static const SoloFrame sf_GRIP_HOLD[] = { {8,0}, {8,0} };
static const SoloFrame sf_SHOOT[] = { {4,0}, {4,6}, {6,10} };
static const SoloFrame sf_STUMBLE[] = { {4,6}, {6,10}, {10,14}, {8,0} };
static const SoloFrame sf_SPRAWL[] = { {4,0}, {14,-10}, {6,0} };
static const SoloFrame sf_HIPBLOCK[] = { {8,0}, {8,0} };
static const SoloFrame sf_TIRED[] = { {14,0}, {14,0} };
static const SoloFrame sf_VICTORY[] = { {8,0}, {8,0}, {8,0} };
static const SoloFrame sf_DEFEAT[] = { {16,0}, {16,0} };
static const SoloFrame sf_BOW[] = { {8,0}, {8,0}, {8,0}, {8,0} };
const SoloAnim soloAnims[SOLO_COUNT] = {
    { 4, 1, sf_IDLE },
    { 6, 1, sf_WALK },
    { 2, 0, sf_GRIP_REACH },
    { 2, 1, sf_GRIP_HOLD },
    { 3, 0, sf_SHOOT },
    { 4, 0, sf_STUMBLE },
    { 3, 0, sf_SPRAWL },
    { 2, 0, sf_HIPBLOCK },
    { 2, 1, sf_TIRED },
    { 3, 1, sf_VICTORY },
    { 2, 1, sf_DEFEAT },
    { 4, 0, sf_BOW },
};

static const PairFrame pf_G_GUARD[] = { {24,5,101,85,86,98}, {24,5,101,86,86,98} };
static const PairFrame pf_G_HALF[] = { {24,5,100,82,86,98}, {24,5,100,83,86,98} };
static const PairFrame pf_G_SIDE[] = { {24,3,98,85,86,98}, {24,3,98,86,86,98} };
static const PairFrame pf_G_KNEE[] = { {24,5,92,66,86,98}, {24,5,92,67,86,98} };
static const PairFrame pf_G_MOUNT[] = { {24,3,74,82,86,98}, {24,3,74,83,86,98} };
static const PairFrame pf_G_BACK[] = { {24,2,110,99,90,98}, {24,2,110,99,90,98} };
static const PairFrame pf_TD_DOUBLE[] = { {4,5,98,83,68,69}, {4,5,94,81,66,66}, {5,5,90,79,64,63}, {3,5,84,79,59,69}, {3,5,78,79,54,74}, {3,5,90,82,70,86}, {10,13,101,85,86,98}, {4,5,101,85,86,98}, {4,5,101,85,86,98} };
static const PairFrame pf_TD_DOUBLE_SPRAWL[] = { {4,5,98,83,68,69}, {4,4,98,87,70,76}, {12,20,98,92,72,82}, {3,4,103,87,68,79}, {3,5,114,77,61,74}, {6,5,119,71,57,71} };
static const PairFrame pf_TD_SINGLE[] = { {4,5,98,83,68,69}, {4,5,100,77,67,69}, {8,5,102,72,66,69}, {3,5,99,73,64,72}, {3,5,95,75,62,79}, {3,5,92,76,60,82}, {3,5,96,79,73,90}, {10,13,100,82,86,98}, {4,5,100,82,86,98}, {4,5,100,82,86,98} };
static const PairFrame pf_TD_SINGLE_SPRAWL[] = { {4,5,98,83,68,69}, {4,4,98,87,70,76}, {12,20,98,92,72,82}, {3,4,103,87,68,79}, {3,5,114,77,61,74}, {6,5,119,71,57,71} };
static const PairFrame pf_TD_JUDO[] = { {4,5,111,71,65,71}, {4,5,94,73,64,71}, {6,5,77,74,63,70}, {3,2,77,74,69,66}, {3,2,77,75,75,63}, {3,2,77,75,90,64}, {3,2,77,76,105,66}, {3,3,78,80,98,82}, {10,11,78,85,90,98}, {3,3,78,85,90,98}, {3,3,78,85,90,98}, {4,3,78,85,90,98} };
static const PairFrame pf_TD_JUDO_BLOCK[] = { {4,5,111,71,65,71}, {4,5,94,73,62,74}, {12,5,77,74,59,77}, {3,5,88,73,58,75}, {3,5,108,72,58,73}, {6,5,119,71,57,71} };
static const PairFrame pf_PULL_GUARD[] = { {4,5,111,71,65,71}, {4,5,109,83,67,72}, {6,5,107,94,69,72}, {3,5,103,95,71,75}, {3,2,94,97,73,82}, {6,2,90,98,75,85} };
static const PairFrame pf_SHAKE[] = { {8,5,117,69,59,69}, {4,5,112,69,64,69}, {12,5,108,69,68,69}, {4,5,110,69,66,69}, {4,5,115,69,61,69}, {6,5,117,69,59,69} };
static const PairFrame pf_T_GUARD_HALF[] = { {3,5,101,85,86,98}, {4,5,101,83,86,98}, {4,5,102,78,86,98}, {5,5,102,76,86,98}, {4,5,101,78,86,98}, {4,5,101,80,86,98}, {6,5,100,82,86,98} };
static const PairFrame pf_T_GUARD_SIDE[] = { {3,5,101,85,86,98}, {4,5,101,83,86,98}, {4,5,102,78,86,98}, {5,5,102,76,86,98}, {4,5,105,76,86,98}, {4,5,109,75,86,98}, {5,5,112,75,86,98}, {4,5,108,77,86,98}, {4,3,102,82,86,98}, {6,3,98,85,86,98} };
static const PairFrame pf_T_GUARD_STAND[] = { {3,5,101,85,86,98}, {4,5,101,83,86,98}, {4,5,102,78,86,98}, {5,5,102,76,86,98}, {4,5,106,75,78,91}, {4,5,115,73,65,78}, {6,5,119,71,57,71} };
static const PairFrame pf_T_HALF_SIDE[] = { {3,5,100,82,86,98}, {4,5,99,83,86,98}, {4,3,99,84,86,98}, {6,3,98,85,86,98} };
static const PairFrame pf_T_HALF_MOUNT[] = { {3,5,100,82,86,98}, {4,5,97,80,86,98}, {4,5,92,76,86,98}, {5,5,89,74,86,98}, {4,5,85,76,86,98}, {4,3,78,80,86,98}, {6,3,74,82,86,98} };
static const PairFrame pf_T_HALF_STAND[] = { {3,5,100,82,86,98}, {4,5,107,80,84,97}, {4,3,119,75,82,95}, {5,3,126,73,80,94}, {4,3,124,72,74,88}, {4,5,121,72,63,77}, {6,5,119,71,57,71} };
static const PairFrame pf_T_SIDE_MOUNT[] = { {3,3,98,85,86,98}, {4,3,96,82,86,98}, {4,5,91,77,86,98}, {5,5,89,74,86,98}, {4,5,85,76,86,98}, {4,3,78,80,86,98}, {6,3,74,82,86,98} };
static const PairFrame pf_T_SIDE_KNEE[] = { {3,3,98,85,86,98}, {4,3,96,80,86,98}, {4,5,94,71,86,98}, {6,5,92,66,86,98} };
static const PairFrame pf_T_SIDE_BACK[] = { {3,3,98,85,86,98}, {4,3,101,83,86,94}, {4,3,107,79,86,86}, {5,3,110,77,86,82}, {4,3,110,83,87,86}, {4,2,110,93,89,94}, {6,2,110,99,90,98} };
static const PairFrame pf_T_KNEE_MOUNT[] = { {3,5,92,66,86,98}, {4,5,87,70,86,98}, {4,3,79,78,86,98}, {6,3,74,82,86,98} };
static const PairFrame pf_T_KNEE_SIDE[] = { {3,5,92,66,86,98}, {4,5,94,71,86,98}, {4,3,96,80,86,98}, {6,3,98,85,86,98} };
static const PairFrame pf_T_MOUNT_BACK[] = { {3,3,74,82,86,98}, {4,3,83,81,86,94}, {4,3,101,78,86,86}, {5,3,110,77,86,82}, {4,3,110,83,87,86}, {4,2,110,93,89,94}, {6,2,110,99,90,98} };
static const PairFrame pf_B_GUARD_SWEEP[] = { {3,5,101,85,86,98}, {4,5,99,81,85,97}, {4,2,94,74,85,95}, {5,2,92,70,84,94}, {4,2,91,77,82,91}, {4,2,91,91,78,85}, {6,2,90,98,76,82} };
static const PairFrame pf_B_GUARD_BACK[] = { {3,5,101,85,86,98}, {4,5,103,83,86,94}, {4,3,108,79,86,86}, {5,3,110,77,86,82}, {4,3,105,82,92,86}, {4,5,95,93,104,95}, {6,5,90,98,110,99} };
static const PairFrame pf_B_GUARD_STAND[] = { {3,5,101,85,86,98}, {4,5,107,82,84,97}, {4,3,120,76,82,95}, {5,3,126,73,80,94}, {4,3,124,72,74,88}, {4,5,121,72,63,77}, {6,5,119,71,57,71} };
static const PairFrame pf_B_HALF_GUARD[] = { {3,5,100,82,86,98}, {4,5,100,83,86,98}, {4,5,101,84,86,98}, {6,5,101,85,86,98} };
static const PairFrame pf_B_HALF_SWEEP[] = { {3,5,100,82,86,98}, {4,5,98,79,85,97}, {4,2,94,73,85,95}, {5,2,92,70,84,94}, {4,2,91,77,82,91}, {4,2,91,91,78,85}, {6,2,90,98,76,82} };
static const PairFrame pf_B_HALF_STAND[] = { {3,5,100,82,86,98}, {4,5,107,80,84,97}, {4,3,119,75,82,95}, {5,3,126,73,80,94}, {4,3,124,72,74,88}, {4,5,121,72,63,77}, {6,5,119,71,57,71} };
static const PairFrame pf_B_SIDE_HALF[] = { {3,3,98,85,86,98}, {4,3,99,84,86,98}, {4,5,99,83,86,98}, {6,5,100,82,86,98} };
static const PairFrame pf_B_SIDE_GUARD[] = { {3,3,98,85,86,98}, {4,3,99,85,86,98}, {4,5,100,85,86,98}, {6,5,101,85,86,98} };
static const PairFrame pf_B_KNEE_HALF[] = { {3,5,92,66,86,98}, {4,5,94,70,86,98}, {4,5,98,78,86,98}, {6,5,100,82,86,98} };
static const PairFrame pf_B_MOUNT_UPA[] = { {3,3,74,82,86,98}, {4,3,79,79,85,97}, {4,2,87,73,85,95}, {5,2,92,70,84,94}, {4,2,90,77,88,92}, {4,2,88,91,97,87}, {6,2,86,98,101,85} };
static const PairFrame pf_B_MOUNT_HALF[] = { {3,3,74,82,86,98}, {4,3,81,82,86,98}, {4,5,93,82,86,98}, {6,5,100,82,86,98} };
static const PairFrame pf_B_BACK_ROLL[] = { {3,2,110,99,90,98}, {4,2,105,91,88,97}, {4,2,97,78,86,95}, {5,2,92,70,84,94}, {4,2,91,77,82,92}, {4,2,91,91,77,87}, {6,2,90,98,75,85} };
static const PairFrame pf_B_BACK_HALF[] = { {3,2,110,99,90,98}, {4,2,107,95,89,98}, {4,5,103,86,87,98}, {6,5,100,82,86,98} };
static const PairFrame pf_S_ANKLE_SET[] = { {2,5,101,85,86,98}, {4,5,106,88,86,98}, {4,5,115,95,86,98}, {4,5,120,98,86,98} };
static const PairFrame pf_S_ANKLE_LOCK[] = { {6,5,120,98,86,98}, {6,5,120,98,86,98} };
static const PairFrame pf_S_ANKLE_TAP[] = { {5,5,120,98,86,98}, {5,5,120,98,86,98} };
static const PairFrame pf_S_KIMURA_H_SET[] = { {2,5,100,82,86,98}, {4,5,101,80,86,98}, {4,5,101,78,86,98}, {4,5,102,76,86,98} };
static const PairFrame pf_S_KIMURA_H_LOCK[] = { {6,5,102,76,86,98}, {6,5,102,76,86,98} };
static const PairFrame pf_S_KIMURA_H_TAP[] = { {5,5,102,76,86,98}, {5,5,102,76,86,98} };
static const PairFrame pf_S_DARCE_SET[] = { {2,5,100,82,86,98}, {4,5,99,83,86,98}, {4,5,97,83,86,98}, {4,5,96,84,86,98} };
static const PairFrame pf_S_DARCE_LOCK[] = { {6,5,96,84,86,98}, {6,5,96,84,86,98} };
static const PairFrame pf_S_DARCE_TAP[] = { {5,5,96,84,86,98}, {5,5,96,84,86,98} };
static const PairFrame pf_S_KIMURA_S_SET[] = { {2,3,98,85,86,98}, {4,3,99,83,86,98}, {4,5,99,81,86,98}, {4,5,100,79,86,98} };
static const PairFrame pf_S_KIMURA_S_LOCK[] = { {6,5,100,79,86,98}, {6,5,100,79,86,98} };
static const PairFrame pf_S_KIMURA_S_TAP[] = { {5,5,100,79,86,98}, {5,5,100,79,86,98} };
static const PairFrame pf_S_AMERICANA_SET[] = { {2,3,98,85,86,98}, {4,3,98,84,86,98}, {4,5,98,84,86,98}, {4,5,98,83,86,98} };
static const PairFrame pf_S_AMERICANA_LOCK[] = { {6,5,98,83,86,98}, {6,5,98,84,86,98} };
static const PairFrame pf_S_AMERICANA_TAP[] = { {5,5,98,84,86,98}, {5,5,98,84,86,98} };
static const PairFrame pf_S_ARMBAR_K_SET[] = { {2,5,92,66,86,98}, {4,5,85,74,86,98}, {4,5,73,88,86,98}, {4,5,66,96,86,98} };
static const PairFrame pf_S_ARMBAR_K_LOCK[] = { {6,5,66,96,86,98}, {6,5,66,96,86,98} };
static const PairFrame pf_S_ARMBAR_K_TAP[] = { {5,5,66,96,86,98}, {5,5,66,96,86,98} };
static const PairFrame pf_S_ARMBAR_M_SET[] = { {2,3,74,82,86,98}, {4,3,72,86,86,98}, {4,5,68,92,86,98}, {4,5,66,96,86,98} };
static const PairFrame pf_S_ARMBAR_M_LOCK[] = { {6,5,66,96,86,98}, {6,5,66,96,86,98} };
static const PairFrame pf_S_ARMBAR_M_TAP[] = { {5,5,66,96,86,98}, {5,5,66,96,86,98} };
static const PairFrame pf_S_CROSS_SET[] = { {2,3,74,82,86,98}, {4,3,75,83,86,98}, {4,3,75,85,86,98}, {4,3,76,86,86,98} };
static const PairFrame pf_S_CROSS_LOCK[] = { {6,3,76,86,86,98}, {6,3,76,87,86,98} };
static const PairFrame pf_S_CROSS_TAP[] = { {5,3,76,87,86,98}, {5,3,76,87,86,98} };
static const PairFrame pf_S_RNC_SET[] = { {2,2,110,99,90,98}, {4,2,110,99,90,98}, {4,2,110,99,90,98}, {4,2,110,99,90,98} };
static const PairFrame pf_S_RNC_LOCK[] = { {6,2,110,99,90,98}, {6,2,110,99,90,98} };
static const PairFrame pf_S_RNC_TAP[] = { {5,2,110,99,90,98}, {5,2,110,99,90,98} };
static const PairFrame pf_S_BOWARROW_SET[] = { {2,2,110,99,90,98}, {4,2,110,99,92,98}, {4,2,110,99,94,98}, {4,2,110,99,96,98} };
static const PairFrame pf_S_BOWARROW_LOCK[] = { {6,2,110,99,96,98}, {6,2,110,99,96,98} };
static const PairFrame pf_S_BOWARROW_TAP[] = { {5,2,110,99,96,98}, {5,2,110,99,96,98} };
static const PairFrame pf_S_TRIANGLE_SET[] = { {2,5,101,85,86,98}, {4,5,102,84,86,98}, {4,5,103,83,86,98}, {4,5,104,82,86,98} };
static const PairFrame pf_S_TRIANGLE_LOCK[] = { {6,5,104,82,86,98}, {6,5,104,82,86,98} };
static const PairFrame pf_S_TRIANGLE_TAP[] = { {5,5,104,82,86,98}, {5,5,104,82,86,98} };
static const PairFrame pf_S_ARMBAR_G_SET[] = { {2,5,101,85,86,98}, {4,5,102,85,86,98}, {4,5,103,84,86,98}, {4,5,104,84,86,98} };
static const PairFrame pf_S_ARMBAR_G_LOCK[] = { {6,5,104,84,86,98}, {6,5,104,84,86,98} };
static const PairFrame pf_S_ARMBAR_G_TAP[] = { {5,5,104,84,86,98}, {5,5,104,84,86,98} };
static const PairFrame pf_S_GUILLOTINE_SET[] = { {2,5,101,85,86,98}, {4,5,102,87,86,98}, {4,5,105,90,86,98}, {4,5,106,92,86,98} };
static const PairFrame pf_S_GUILLOTINE_LOCK[] = { {6,5,106,92,86,98}, {6,5,106,90,86,98} };
static const PairFrame pf_S_GUILLOTINE_TAP[] = { {5,5,106,90,86,98}, {5,5,106,90,86,98} };
static const PairFrame pf_S_KIMURA_HB_SET[] = { {2,5,100,82,86,98}, {4,5,100,82,86,98}, {4,5,100,82,86,98}, {4,5,100,82,86,98} };
static const PairFrame pf_S_KIMURA_HB_LOCK[] = { {6,5,100,82,86,98}, {6,5,100,82,86,98} };
static const PairFrame pf_S_KIMURA_HB_TAP[] = { {5,5,100,82,86,98}, {5,5,100,82,86,98} };
const PairSeq pairSeqs[SEQ_COUNT] = {
    { 2, 1, 0, 0, pf_G_GUARD },
    { 2, 1, 0, 0, pf_G_HALF },
    { 2, 1, 0, 0, pf_G_SIDE },
    { 2, 1, 0, 0, pf_G_KNEE },
    { 2, 1, 0, 0, pf_G_MOUNT },
    { 2, 1, 0, 0, pf_G_BACK },
    { 9, 0, 0, 0, pf_TD_DOUBLE },
    { 6, 0, 0, 0, pf_TD_DOUBLE_SPRAWL },
    { 10, 0, 0, 0, pf_TD_SINGLE },
    { 6, 0, 0, 0, pf_TD_SINGLE_SPRAWL },
    { 12, 0, 1, 0, pf_TD_JUDO },
    { 6, 0, 0, 0, pf_TD_JUDO_BLOCK },
    { 6, 0, 1, 1, pf_PULL_GUARD },
    { 6, 0, 0, 0, pf_SHAKE },
    { 7, 0, 0, 0, pf_T_GUARD_HALF },
    { 10, 0, 0, 0, pf_T_GUARD_SIDE },
    { 7, 0, 0, 0, pf_T_GUARD_STAND },
    { 4, 0, 0, 0, pf_T_HALF_SIDE },
    { 7, 0, 0, 0, pf_T_HALF_MOUNT },
    { 7, 0, 0, 0, pf_T_HALF_STAND },
    { 7, 0, 0, 0, pf_T_SIDE_MOUNT },
    { 4, 0, 0, 0, pf_T_SIDE_KNEE },
    { 7, 0, 0, 0, pf_T_SIDE_BACK },
    { 4, 0, 0, 0, pf_T_KNEE_MOUNT },
    { 4, 0, 0, 0, pf_T_KNEE_SIDE },
    { 7, 0, 0, 0, pf_T_MOUNT_BACK },
    { 7, 0, 1, 1, pf_B_GUARD_SWEEP },
    { 7, 0, 0, 1, pf_B_GUARD_BACK },
    { 7, 0, 0, 0, pf_B_GUARD_STAND },
    { 4, 0, 0, 0, pf_B_HALF_GUARD },
    { 7, 0, 1, 1, pf_B_HALF_SWEEP },
    { 7, 0, 0, 0, pf_B_HALF_STAND },
    { 4, 0, 0, 0, pf_B_SIDE_HALF },
    { 4, 0, 0, 0, pf_B_SIDE_GUARD },
    { 4, 0, 0, 0, pf_B_KNEE_HALF },
    { 7, 0, 0, 1, pf_B_MOUNT_UPA },
    { 4, 0, 0, 0, pf_B_MOUNT_HALF },
    { 7, 0, 1, 1, pf_B_BACK_ROLL },
    { 4, 0, 0, 0, pf_B_BACK_HALF },
    { 4, 0, 0, 0, pf_S_ANKLE_SET },
    { 2, 1, 0, 0, pf_S_ANKLE_LOCK },
    { 2, 1, 0, 0, pf_S_ANKLE_TAP },
    { 4, 0, 0, 0, pf_S_KIMURA_H_SET },
    { 2, 1, 0, 0, pf_S_KIMURA_H_LOCK },
    { 2, 1, 0, 0, pf_S_KIMURA_H_TAP },
    { 4, 0, 0, 0, pf_S_DARCE_SET },
    { 2, 1, 0, 0, pf_S_DARCE_LOCK },
    { 2, 1, 0, 0, pf_S_DARCE_TAP },
    { 4, 0, 0, 0, pf_S_KIMURA_S_SET },
    { 2, 1, 0, 0, pf_S_KIMURA_S_LOCK },
    { 2, 1, 0, 0, pf_S_KIMURA_S_TAP },
    { 4, 0, 0, 0, pf_S_AMERICANA_SET },
    { 2, 1, 0, 0, pf_S_AMERICANA_LOCK },
    { 2, 1, 0, 0, pf_S_AMERICANA_TAP },
    { 4, 0, 0, 0, pf_S_ARMBAR_K_SET },
    { 2, 1, 0, 0, pf_S_ARMBAR_K_LOCK },
    { 2, 1, 0, 0, pf_S_ARMBAR_K_TAP },
    { 4, 0, 0, 0, pf_S_ARMBAR_M_SET },
    { 2, 1, 0, 0, pf_S_ARMBAR_M_LOCK },
    { 2, 1, 0, 0, pf_S_ARMBAR_M_TAP },
    { 4, 0, 0, 0, pf_S_CROSS_SET },
    { 2, 1, 0, 0, pf_S_CROSS_LOCK },
    { 2, 1, 0, 0, pf_S_CROSS_TAP },
    { 4, 0, 0, 0, pf_S_RNC_SET },
    { 2, 1, 0, 0, pf_S_RNC_LOCK },
    { 2, 1, 0, 0, pf_S_RNC_TAP },
    { 4, 0, 0, 0, pf_S_BOWARROW_SET },
    { 2, 1, 0, 0, pf_S_BOWARROW_LOCK },
    { 2, 1, 0, 0, pf_S_BOWARROW_TAP },
    { 4, 0, 0, 0, pf_S_TRIANGLE_SET },
    { 2, 1, 0, 0, pf_S_TRIANGLE_LOCK },
    { 2, 1, 0, 0, pf_S_TRIANGLE_TAP },
    { 4, 0, 0, 0, pf_S_ARMBAR_G_SET },
    { 2, 1, 0, 0, pf_S_ARMBAR_G_LOCK },
    { 2, 1, 0, 0, pf_S_ARMBAR_G_TAP },
    { 4, 0, 0, 0, pf_S_GUILLOTINE_SET },
    { 2, 1, 0, 0, pf_S_GUILLOTINE_LOCK },
    { 2, 1, 0, 0, pf_S_GUILLOTINE_TAP },
    { 4, 0, 0, 0, pf_S_KIMURA_HB_SET },
    { 2, 1, 0, 0, pf_S_KIMURA_HB_LOCK },
    { 2, 1, 0, 0, pf_S_KIMURA_HB_TAP },
};
