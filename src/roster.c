/*
 * Roster de BJJ TATAMI : 9 categories de poids IBJJF (adultes), 3 combattants chacune.
 * Combattants FICTIFS, inspires des grands styles du jiu-jitsu.
 * Pour changer un nom : modifier la chaine ci-dessous (13 caracteres max, majuscules, sans accent).
 *
 * stat = { GARDE, PASSAGE, LUTTE, SOUMISSION, CARDIO } de 1 a 9
 * special = famille de soumission preferee (curseur plus grand dans le mini-jeu)
 */
#include "game.h"

const char* const subFamilyName[5] = { "CLES DE BRAS", "TRIANGLE", "KIMURA", "ETRANGLEMENTS", "CLES DE PIED" };

#define F(n, k, g, p, l, s, c, sp, gi, sk, ha) { n, k, { g, p, l, s, c }, sp, gi, sk, ha }

const Category roster[NUM_CATS] =
{
    { "GALO", "-57,5 KG", {
        F("TICO ARAUJO",   "LE MOUSTIQUE",    8, 5, 4, 6, 8, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK),
        F("SHO TAKEUCHI",  "LA TORNADE",      4, 6, 8, 6, 7, SUBF_KIM,   GI_WHITE, SKIN_LIGHT, HAIR_BLACK),
        F("RAFA DUTRA",    "LE RENARD",       6, 7, 5, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_MED,   HAIR_BROWN) } },
    { "PLUMA", "-64 KG", {
        F("BETO SALES",    "L'ARAIGNEE",      9, 4, 3, 7, 6, SUBF_TRI,   GI_WHITE, SKIN_MED,   HAIR_BLACK),
        F("IAGO MOTTA",    "LE PISTON",       5, 8, 6, 5, 8, SUBF_CHOKE, GI_BLUE,  SKIN_DARK,  HAIR_BALD),
        F("NOAH LEDUC",    "LE PETIT LOUP",   6, 6, 6, 6, 7, SUBF_CHOKE, GI_WHITE, SKIN_LIGHT, HAIR_BLOND) } },
    { "PENA", "-70 KG", {
        F("DIEGO VALE",    "LE SERPENT",      8, 6, 5, 8, 6, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK),
        F("MALIK DIOP",    "LE BULLDOZER",    4, 8, 8, 6, 7, SUBF_CHOKE, GI_WHITE, SKIN_DARK,  HAIR_BALD),
        F("THEO MARCHAND", "LE PROF",         7, 7, 5, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_LIGHT, HAIR_BROWN) } },
    { "LEVE", "-76 KG", {
        F("CAIO BRANDAO",  "L'EQUILIBRISTE",  7, 8, 6, 7, 7, SUBF_CHOKE, GI_WHITE, SKIN_MED,   HAIR_BLACK),
        F("YURI KOVAL",    "LE SAMBISTE",     5, 6, 8, 7, 6, SUBF_LEG,   GI_BLUE,  SKIN_LIGHT, HAIR_BLOND),
        F("IBRA SOW",      "LA PANTHERE",     6, 7, 7, 6, 9, SUBF_KIM,   GI_BLACK, SKIN_DARK,  HAIR_BLACK) } },
    { "MEDIO", "-82,3 KG", {
        F("VITOR LAGO",    "LE PITBULL",      5, 8, 7, 6, 8, SUBF_CHOKE, GI_BLUE,  SKIN_MED,   HAIR_BALD),
        F("ENZO RICCI",    "LE CHIRURGIEN",   8, 5, 4, 9, 5, SUBF_ARM,   GI_WHITE, SKIN_LIGHT, HAIR_BLACK),
        F("ADAM BELKACEM", "LE LION",         6, 7, 8, 6, 7, SUBF_KIM,   GI_WHITE, SKIN_MED,   HAIR_BLACK) } },
    { "MEIO-PESADO", "-88,3 KG", {
        F("RUBENS TELES",  "LE MUR",          4, 8, 7, 6, 7, SUBF_KIM,   GI_BLUE,  SKIN_DARK,  HAIR_BLACK),
        F("ANDRE FONTES",  "LE MAGICIEN",     8, 6, 5, 8, 6, SUBF_TRI,   GI_WHITE, SKIN_MED,   HAIR_BROWN),
        F("OLEG VARGA",    "LE TRACTEUR",     5, 7, 9, 5, 6, SUBF_CHOKE, GI_BLACK, SKIN_LIGHT, HAIR_BALD) } },
    { "PESADO", "-94,3 KG", {
        F("BRUNO CAMARGO", "LE BUFFLE",       4, 8, 8, 6, 6, SUBF_CHOKE, GI_WHITE, SKIN_MED,   HAIR_BLACK),
        F("SAMI HADDAD",   "LE COBRA",        7, 6, 6, 8, 6, SUBF_TRI,   GI_BLUE,  SKIN_MED,   HAIR_BLACK),
        F("JEAN-MARC ROY", "LE BUCHERON",     5, 7, 7, 7, 5, SUBF_ARM,   GI_WHITE, SKIN_LIGHT, HAIR_BLOND) } },
    { "SUPER-PESADO", "-100,5 KG", {
        F("GILSON MATOS",  "LE TAUREAU",      4, 9, 8, 6, 5, SUBF_KIM,   GI_BLUE,  SKIN_DARK,  HAIR_BALD),
        F("HENRIK DAHL",   "LE VIKING",       5, 7, 7, 7, 6, SUBF_CHOKE, GI_WHITE, SKIN_LIGHT, HAIR_BLOND),
        F("TOMAS REIS",    "LE ROCHER",       6, 7, 6, 7, 6, SUBF_ARM,   GI_BLACK, SKIN_MED,   HAIR_BLACK) } },
    { "PESADISSIMO", "+100,5 KG", {
        F("SEKOU DEMBA",   "LA MONTAGNE",     4, 8, 8, 6, 5, SUBF_KIM,   GI_WHITE, SKIN_DARK,  HAIR_BALD),
        F("FABIO PRATES",  "LE GEANT",        6, 7, 6, 7, 5, SUBF_CHOKE, GI_BLUE,  SKIN_MED,   HAIR_BROWN),
        F("IGOR SOKOL",    "L'OURS",          5, 8, 9, 6, 4, SUBF_LEG,   GI_BLACK, SKIN_LIGHT, HAIR_BALD) } },
};
