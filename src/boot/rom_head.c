#include "genesis.h"

/* en-tete de la cartouche : "J6" = manette 6 boutons supportee
   (les emulateurs en mode auto activent alors X / Y / Z -> Xbox Y, LB, RB sous RetroArch) */
__attribute__((externally_visible))
const ROMHeader rom_header = {
    "SEGA MEGA DRIVE ",
    "(C)TARIQ 2026   ",
    "BJJ TATAMI                                      ",
    "BJJ TATAMI                                      ",
    "GM 00000000-00",
    0x000,
    "J6              ",
    0x00000000,
#if (ENABLE_BANK_SWITCH != 0)
    0x003FFFFF,
#else
    0x000FFFFF,
#endif
    0xE0FF0000,
    0xE0FFFFFF,
    "RA",
    0xF820,
    0x00200000,
    0x0020FFFF,
    "            ",
    "JIU-JITSU BRESILIEN - HOMEBREW SGDK     ",
    "JUE             "
};
