/*
 * Son : bruitages PSG, musique de titre (PCM), public et voix de l'arbitre (PCM4).
 */
#include "game.h"

/* ------------------------------------------------------------------ sons PSG */
const u16 tuneWin[6]   = { 523, 659, 784, 1046, 0, 1046 };
const u16 tuneLose[4]  = { 392, 330, 262, 196 };
const u16 tuneStart[3] = { 784, 0, 784 };

static s16 sfxTimer;
static void sfxTimerSet(s16 f) { sfxTimer = f; }
static const u16* tunePtr;
static u16 tuneLen, tuneIdx;
static s16 tuneTimer;

void sfx(u16 freq, s16 frames)
{
    PSG_setFrequency(0, freq);
    PSG_setEnvelope(0, 2);
    sfxTimer = frames;
}

void sfxSlam(void)
{
    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK2);
    PSG_setEnvelope(3, 2);
    sfxTimer = 10;
}

void playTune(const u16* t, u16 len)
{
    tunePtr = t;
    tuneLen = len;
    tuneIdx = 0;
    tuneTimer = 0;
}

void soundUpdate(void)
{
    if (tunePtr)
    {
        if (tuneTimer <= 0)
        {
            if (tuneIdx >= tuneLen)
            {
                tunePtr = NULL;
                PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
            }
            else
            {
                u16 f = tunePtr[tuneIdx++];
                if (f) { PSG_setFrequency(1, f); PSG_setEnvelope(1, 3); }
                else PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
                tuneTimer = 9;
            }
        }
        tuneTimer--;
    }
    if (sfxTimer > 0 && --sfxTimer == 0)
    {
        PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
        PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
    }
}

/* ------------------------------------------------------------------ musique (driver PCM) */
static u16 musicStage;      /* 0 rien, 1 intro, 2 boucle */

void musicTitleStart(void)
{
    if (musicStage) return;                 /* deja en cours : on ne la relance pas */
    crowdStop();
    SND_PCM_startPlay(title_intro, sizeof(title_intro), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, FALSE);
    musicStage = 1;
}

void musicUpdate(void)
{
    if (musicStage == 1 && !SND_PCM_isPlaying())
    {
        SND_PCM_startPlay(title_loop, sizeof(title_loop), SOUND_PCM_RATE_13400, SOUND_PAN_CENTER, TRUE);
        musicStage = 2;
    }
}

void musicStop(void)
{
    if (!musicStage) return;
    SND_PCM_stopPlay();
    musicStage = 0;
}

/* ------------------------------------------------------------------ public (driver PCM4) */
static bool crowdOn;
static s16 crowdBoost;
static u8 crowdVol;

void crowdStart(void)
{
    musicStop();
    SND_PCM4_startPlay(crowd_loop, sizeof(crowd_loop), SOUND_PCM_CH1, TRUE);
    crowdVol = 6;
    SND_PCM4_setVolume(SOUND_PCM_CH1, crowdVol);
    crowdBoost = 0;
    crowdOn = TRUE;
}

void crowdStop(void)
{
    if (!crowdOn) return;
    SND_PCM4_stopPlay(SOUND_PCM_CH1);
    SND_PCM4_stopPlay(SOUND_PCM_CH2);
    crowdOn = FALSE;
}

void crowdReact(bool big)
{
    if (!crowdOn) return;
    if (big) SND_PCM4_startPlay(cheer_big, sizeof(cheer_big), SOUND_PCM_CH2, FALSE);
    else SND_PCM4_startPlay(cheer_small, sizeof(cheer_small), SOUND_PCM_CH2, FALSE);
    SND_PCM4_setVolume(SOUND_PCM_CH2, 15);
    crowdBoost = big ? 240 : 90;
}

/* ------------------------------------------------------------------ voix de l'arbitre (PCM4, canal 3) */
static const u8* const voiceData[VO_COUNT] = { vo_oss, vo_combate, vo_finalizou, vo_tempo, vo_vitoria, vo_empate, vo_desclassificado, vo_perfeito, vo_campeao, vo_final, vo_pronto, vo_parou, vo_contra, vo_especial, vo_pontos2, vo_pontos3, vo_pontos4, vo_vantagem, vo_punicao, vo_lutem };
static const u16 voiceSize[VO_COUNT] = { sizeof(vo_oss), sizeof(vo_combate), sizeof(vo_finalizou), sizeof(vo_tempo), sizeof(vo_vitoria), sizeof(vo_empate), sizeof(vo_desclassificado), sizeof(vo_perfeito), sizeof(vo_campeao), sizeof(vo_final), sizeof(vo_pronto), sizeof(vo_parou), sizeof(vo_contra), sizeof(vo_especial), sizeof(vo_pontos2), sizeof(vo_pontos3), sizeof(vo_pontos4), sizeof(vo_vantagem), sizeof(vo_punicao), sizeof(vo_lutem) };
static s16 voiceTimer;

void voiceSay(u16 v)
{
    if (v >= VO_COUNT) return;
    SND_PCM4_startPlay(voiceData[v], voiceSize[v], SOUND_PCM_CH3, FALSE);
    SND_PCM4_setVolume(SOUND_PCM_CH3, 15);
    voiceTimer = voiceSize[v] / 267 + 4;    /* 16 kHz = ~267 octets par image */
}

void crowdUpdate(bool tense)
{
    u8 v = 6;
    if (voiceTimer > 0) voiceTimer--;
    if (!crowdOn) return;
    if (crowdBoost > 0) crowdBoost--;
    if (tense) v += 3;
    if (crowdBoost > 0) v += 4;
    if (voiceTimer > 0) v = (v > 9) ? v - 5 : 4;   /* le public se tait un peu quand l'arbitre parle */
    if (v > 15) v = 15;
    if (v != crowdVol)
    {
        crowdVol = v;
        SND_PCM4_setVolume(SOUND_PCM_CH1, v);
    }
}

void sfxNoise(s16 frames)
{
    PSG_setNoise(PSG_NOISE_TYPE_WHITE, PSG_NOISE_FREQ_CLOCK2);
    PSG_setEnvelope(3, 4);
    sfxTimerSet(frames);
}
