/*
 * Sauvegarde en SRAM : nom du joueur 1, options, statistiques, tableau des scores.
 */
#include "game.h"

#define SAVE_VER    2
#define OFS_NAME    3
#define OFS_OPT     11
#define OFS_STATS   14
#define OFS_HI      26
#define HI_SIZE     14
#define OFS_SUM     (OFS_HI + HISCORES * HI_SIZE)

HiScore hiscores[HISCORES];
Stats stats;

static const char* const defNames[HISCORES] = { "MESTRE", "PROFESOR", "TATAMI", "OSS", "FAIXA", "GUARDA", "BASE", "KIMURA" };

static void defaults(void)
{
    u16 i;
    scopy(playerName, "JOUEUR 1", NAME_LEN);
    for (i = 0; i < HISCORES; i++)
    {
        scopy(hiscores[i].name, defNames[i], NAME_LEN);
        hiscores[i].score = 60000 - i * 7000;
        hiscores[i].fighter = (i * 7) % NUM_FIGHTERS;
        hiscores[i].stage = LADDER_LEN - (i / 2);
    }
    stats.matches = stats.wins = stats.subs = stats.champs = 0;
    stats.bestStreak = stats.streak = 0;
}

static u8 rd(u32 o) { return SRAM_readByte(o); }

static u16 rd16(u32 o) { return ((u16)rd(o) << 8) | rd(o + 1); }

static u32 rd32(u32 o) { return ((u32)rd16(o) << 16) | rd16(o + 2); }

static void wr16(u32 o, u16 v) { SRAM_writeByte(o, v >> 8); SRAM_writeByte(o + 1, v & 0xFF); }

static void wr32(u32 o, u32 v) { wr16(o, v >> 16); wr16(o + 2, v & 0xFFFF); }

void saveLoad(void)
{
    u16 i, k;
    u8 sum = 0;
    bool ok;

    SRAM_enableRO();
    ok = rd(0) == 'B' && rd(1) == 'J' && rd(2) == SAVE_VER;
    for (i = OFS_NAME; i < OFS_SUM; i++) sum += rd(i);
    ok = ok && (rd(OFS_SUM) == sum);
    if (!ok)
    {
        SRAM_disable();
        defaults();
        return;
    }
    for (i = 0; i < NAME_LEN; i++) playerName[i] = rd(OFS_NAME + i);
    playerName[NAME_LEN] = 0;
    if (playerName[0] == 0) scopy(playerName, "JOUEUR 1", NAME_LEN);
    difficulty = rd(OFS_OPT) % 3;
    matchMinutes = rd(OFS_OPT + 1);
    if (matchMinutes < 2 || matchMinutes > 5) matchMinutes = 3;
    helpOn = rd(OFS_OPT + 2) != 0;
    stats.matches = rd16(OFS_STATS);
    stats.wins = rd16(OFS_STATS + 2);
    stats.subs = rd16(OFS_STATS + 4);
    stats.champs = rd16(OFS_STATS + 6);
    stats.bestStreak = rd16(OFS_STATS + 8);
    stats.streak = rd16(OFS_STATS + 10);
    for (i = 0; i < HISCORES; i++)
    {
        u32 o = OFS_HI + i * HI_SIZE;
        for (k = 0; k < NAME_LEN; k++) hiscores[i].name[k] = rd(o + k);
        hiscores[i].name[NAME_LEN] = 0;
        hiscores[i].score = rd32(o + 8);
        hiscores[i].fighter = rd(o + 12) % NUM_FIGHTERS;
        hiscores[i].stage = rd(o + 13);
    }
    SRAM_disable();
}

void saveWrite(void)
{
    u16 i, k;
    u8 sum = 0;

    SRAM_enable();
    SRAM_writeByte(0, 'B');
    SRAM_writeByte(1, 'J');
    SRAM_writeByte(2, SAVE_VER);
    for (i = 0; i < NAME_LEN; i++) SRAM_writeByte(OFS_NAME + i, (i < slen(playerName)) ? playerName[i] : 0);
    SRAM_writeByte(OFS_OPT, difficulty);
    SRAM_writeByte(OFS_OPT + 1, matchMinutes);
    SRAM_writeByte(OFS_OPT + 2, helpOn ? 1 : 0);
    wr16(OFS_STATS, stats.matches);
    wr16(OFS_STATS + 2, stats.wins);
    wr16(OFS_STATS + 4, stats.subs);
    wr16(OFS_STATS + 6, stats.champs);
    wr16(OFS_STATS + 8, stats.bestStreak);
    wr16(OFS_STATS + 10, stats.streak);
    for (i = 0; i < HISCORES; i++)
    {
        u32 o = OFS_HI + i * HI_SIZE;
        for (k = 0; k < NAME_LEN; k++) SRAM_writeByte(o + k, (k < slen(hiscores[i].name)) ? hiscores[i].name[k] : 0);
        wr32(o + 8, hiscores[i].score);
        SRAM_writeByte(o + 12, hiscores[i].fighter);
        SRAM_writeByte(o + 13, hiscores[i].stage);
    }
    for (i = OFS_NAME; i < OFS_SUM; i++) sum += SRAM_readByte(i);
    SRAM_writeByte(OFS_SUM, sum);
    SRAM_disable();
}

/* insere un score ; renvoie le rang (0..7) ou -1 */
s16 hiscoreInsert(u32 sc, u16 fighter, u16 stage)
{
    s16 i, r = -1;
    for (i = 0; i < HISCORES; i++)
        if (sc > hiscores[i].score) { r = i; break; }
    if (r < 0) return -1;
    for (i = HISCORES - 1; i > r; i--) hiscores[i] = hiscores[i - 1];
    scopy(hiscores[r].name, playerName, NAME_LEN);
    hiscores[r].score = sc;
    hiscores[r].fighter = fighter;
    hiscores[r].stage = stage;
    return r;
}
