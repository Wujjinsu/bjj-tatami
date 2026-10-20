/*
 * Roster de BJJ TATAMI : 9 categories de poids IBJJF (adultes), 3 combattants chacune.
 * Combattants FICTIFS. Pour changer un nom : modifier la chaine (13 caracteres max, majuscules, sans accent).
 *
 * stat = { GARDE, PASSAGE, LUTTE, SOUMISSION, CARDIO } de 1 a 9
 * special = famille de soumission preferee ; accent = couleur des ecussons (0-7) ; flag = pays
 */
#include "game.h"

const char* const subFamilyName[5] = { "CLES DE BRAS", "TRIANGLE", "KIMURA", "ETRANGLEMENTS", "CLES DE PIED" };

#define F(n, k, g, p, l, s, c, sp, gi, sk, ha, be, ac, fl) { n, k, { g, p, l, s, c }, sp, gi, sk, ha, be, ac, FLAG_##fl }

const Category roster[NUM_CATS] =
{
    { "GALO", "-57,5 KG", {
        F("TICO ARAUJO",   "LE MOUSTIQUE",   8, 5, 4, 6, 8, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK, 0, 0, BRA),
        F("SHO TAKEUCHI",  "LA TORNADE",     4, 6, 8, 6, 7, SUBF_KIM,   GI_WHITE, SKIN_LIGHT, HAIR_BLACK, 0, 1, JPN),
        F("RAFA DUTRA",    "LE RENARD",      6, 7, 5, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_MED,   HAIR_BROWN, 1, 2, BRA) } },
    { "PLUMA", "-64 KG", {
        F("BETO SALES",    "L'ARAIGNEE",     9, 4, 3, 7, 6, SUBF_TRI,   GI_WHITE, SKIN_MED,   HAIR_BLACK, 1, 3, BRA),
        F("IAGO MOTTA",    "LE PISTON",      5, 8, 6, 5, 8, SUBF_CHOKE, GI_BLUE,  SKIN_DARK,  HAIR_BALD,  1, 4, BRA),
        F("NOAH LEDUC",    "LE PETIT LOUP",  6, 6, 6, 6, 7, SUBF_CHOKE, GI_WHITE, SKIN_LIGHT, HAIR_BLOND, 0, 7, FRA) } },
    { "PENA", "-70 KG", {
        F("DIEGO VALE",    "LE SERPENT",     8, 6, 5, 8, 6, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK, 1, 5, BRA),
        F("MALIK DIOP",    "LE BULLDOZER",   4, 8, 8, 6, 7, SUBF_CHOKE, GI_WHITE, SKIN_DARK,  HAIR_BALD,  0, 2, SEN),
        F("THEO MARCHAND", "LE PROF",        7, 7, 5, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_LIGHT, HAIR_BROWN, 1, 1, FRA) } },
    { "LEVE", "-76 KG", {
        F("CAIO BRANDAO",  "L'EQUILIBRISTE", 7, 8, 6, 7, 7, SUBF_CHOKE, GI_WHITE, SKIN_MED,   HAIR_BLACK, 0, 2, BRA),
        F("YURI KOVAL",    "LE SAMBISTE",    5, 6, 8, 7, 6, SUBF_LEG,   GI_BLUE,  SKIN_LIGHT, HAIR_BLOND, 1, 0, UKR),
        F("IBRA SOW",      "LA PANTHERE",    6, 7, 7, 6, 9, SUBF_KIM,   GI_BLACK, SKIN_DARK,  HAIR_BLACK, 0, 6, SEN) } },
    { "MEDIO", "-82,3 KG", {
        F("VITOR LAGO",    "LE PITBULL",     5, 8, 7, 6, 8, SUBF_CHOKE, GI_BLUE,  SKIN_MED,   HAIR_BALD,  1, 1, BRA),
        F("ENZO RICCI",    "LE CHIRURGIEN",  8, 5, 4, 9, 5, SUBF_ARM,   GI_WHITE, SKIN_LIGHT, HAIR_BLACK, 1, 2, ITA),
        F("ADAM BELKACEM", "LE LION",        6, 7, 8, 6, 7, SUBF_KIM,   GI_WHITE, SKIN_MED,   HAIR_BLACK, 1, 7, ALG) } },
    { "MEIO-PESADO", "-88,3 KG", {
        F("RUBENS TELES",  "LE MUR",         4, 8, 7, 6, 7, SUBF_KIM,   GI_BLUE,  SKIN_DARK,  HAIR_BLACK, 1, 0, BRA),
        F("ANDRE FONTES",  "LE MAGICIEN",    8, 6, 5, 8, 6, SUBF_TRI,   GI_WHITE, SKIN_MED,   HAIR_BROWN, 0, 4, POR),
        F("OLEG VARGA",    "LE TRACTEUR",    5, 7, 9, 5, 6, SUBF_CHOKE, GI_BLACK, SKIN_LIGHT, HAIR_BALD,  0, 5, HUN) } },
    { "PESADO", "-94,3 KG", {
        F("BRUNO CAMARGO", "LE BUFFLE",      4, 8, 8, 6, 6, SUBF_CHOKE, GI_WHITE, SKIN_MED,   HAIR_BLACK, 1, 1, BRA),
        F("SAMI HADDAD",   "LE COBRA",       7, 6, 6, 8, 6, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK, 1, 2, LBN),
        F("JEAN-MARC ROY", "LE BUCHERON",    5, 7, 7, 7, 5, SUBF_ARM,   GI_WHITE, SKIN_LIGHT, HAIR_BLOND, 1, 1, CAN) } },
    { "SUPER-PESADO", "-100,5 KG", {
        F("GILSON MATOS",  "LE TAUREAU",     4, 9, 8, 6, 5, SUBF_KIM,   GI_BLUE,  SKIN_DARK,  HAIR_BALD,  0, 0, BRA),
        F("HENRIK DAHL",   "LE VIKING",      5, 7, 7, 7, 6, SUBF_CHOKE, GI_WHITE, SKIN_LIGHT, HAIR_BLOND, 1, 7, NOR),
        F("TOMAS REIS",    "LE ROCHER",      6, 7, 6, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_MED,   HAIR_BLACK, 0, 1, POR) } },
    { "PESADISSIMO", "+100,5 KG", {
        F("SEKOU DEMBA",   "LA MONTAGNE",    4, 8, 8, 6, 5, SUBF_KIM,   GI_WHITE, SKIN_DARK,  HAIR_BALD,  0, 0, MLI),
        F("FABIO PRATES",  "LE GEANT",       6, 7, 6, 7, 5, SUBF_CHOKE, GI_BLUE,  SKIN_MED,   HAIR_BROWN, 1, 2, BRA),
        F("IGOR SOKOL",    "L'OURS",         5, 8, 9, 6, 4, SUBF_LEG,   GI_BLACK, SKIN_LIGHT, HAIR_BALD,  1, 1, POL) } },
};

const FighterDef* fighterAt(u16 gid)
{
    return &roster[gid / FIGHTERS_PER_CAT].f[gid % FIGHTERS_PER_CAT];
}

const Image* const portraits[NUM_FIGHTERS] =
{
    &portrait_00, &portrait_01, &portrait_02, &portrait_03, &portrait_04, &portrait_05, &portrait_06,
    &portrait_07, &portrait_08, &portrait_09, &portrait_10, &portrait_11, &portrait_12, &portrait_13,
    &portrait_14, &portrait_15, &portrait_16, &portrait_17, &portrait_18, &portrait_19, &portrait_20,
    &portrait_21, &portrait_22, &portrait_23, &portrait_24, &portrait_25, &portrait_26,
};
