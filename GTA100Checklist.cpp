#include "plugin.h"
#include "CStats.h"
#include "CMenuManager.h"
#include "CTheScripts.h"
#include "CText.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

using namespace plugin;

// GTA SA 1.0 US internal buffers used by CStats::ConstructStatLine
char* gStatLabelBuffer = (char*)0xB71670; // Left column text (ASCII)
char* gStatValueBuffer = (char*)0xB71770; // Right column text (ASCII)

// Original function pointer for CStats::ConstructStatLine
typedef int(__cdecl* tConstructStatLine)(int line, unsigned char category);
tConstructStatLine orig_ConstructStatLine = (tConstructStatLine)0x55A780;

// Helper to read CStats
int GetIntStat(unsigned short statId) {
    return (int)CStats::GetStatValue(statId);
}

float GetFloatStat(unsigned short statId) {
    return CStats::GetStatValue(statId);
}

// Helper to read 32-bit int from v1.0 US main.scm ScriptSpace (0xA49960)
int GetSCM(int varIndex) {
    int* scriptSpace = (int*)0xA49960;
    return scriptSpace[varIndex];
}

struct ChecklistRow {
    std::string label;
    std::string value;
};

// Builds the entire 187-point 100% Completion Checklist
void BuildFull100Checklist(std::vector<ChecklistRow>& rows) {
    rows.clear();

    auto AddHeader = [&](const char* title) {
        rows.push_back({ title, "----------------" });
    };

    auto AddProgress = [&](const char* name, int cur, int req) {
        char buf[32];
        if (cur < 0) cur = 0;
        if (cur >= req) {
            snprintf(buf, sizeof(buf), "%d/%d [DONE]", req, req);
        } else {
            snprintf(buf, sizeof(buf), "%d/%d", cur, req);
        }
        rows.push_back({ name, buf });
    };

    auto AddBool = [&](const char* name, bool done) {
        rows.push_back({ name, done ? "[DONE]" : "[TODO]" });
    };

    // =========================================================================
    // 0. OVERALL SUMMARY (187 Points Total)
    // =========================================================================
    float pts = GetFloatStat(STAT_PROGRESS_MADE); // Max 187.0
    float pct = (pts / 187.0f) * 100.0f;
    char summaryBuf[32];
    snprintf(summaryBuf, sizeof(summaryBuf), "%.0f/187 (%.2f%%)", pts, pct);
    rows.push_back({ ">> TOTAL 100% PROGRESS", summaryBuf });

    // =========================================================================
    // 1. STORYLINE & ASSET THREADS (96 Missions across Los Santos, Badlands, SF, Desert, LV, Return)
    // =========================================================================
    AddHeader("-- STORYLINE THREADS --");
    AddProgress("Big Smoke Missions",          GetSCM(436), 4);
    AddProgress("Ryder Missions",              GetSCM(437), 3);
    AddProgress("Sweet Missions (LS Act 1)",   GetSCM(435), 7);
    AddProgress("OG Loc Missions",             GetSCM(439), 4);
    AddProgress("Cesar Vialpando (LS)",        GetSCM(440), 1);
    AddProgress("C.R.A.S.H. (LS)",             GetSCM(438), 2);
    AddProgress("Catalina (Badlands)",         GetSCM(450), 4);
    AddProgress("The Truth (Badlands)",        GetSCM(448), 2);
    AddProgress("Cesar (Badlands Races)",      GetSCM(449), 2);
    AddProgress("CJ / Garage (San Fierro)",    GetSCM(459), 6);
    AddProgress("Loco Syndicate (SF)",         GetSCM(461), 6);
    AddProgress("Wozzie (San Fierro)",         GetSCM(460), 4);
    AddProgress("C.R.A.S.H. (San Fierro)",     GetSCM(462), 1);
    AddProgress("Zero RC Shop (Asset)",        GetSCM(463), 3);
    AddProgress("Wang Cars Showroom (Asset)",  GetSCM(465), 4);
    AddProgress("Toreno (Desert)",             GetSCM(473), 4);
    AddProgress("Verdant Meadows Airstrip",    GetSCM(474), 4);
    AddProgress("Four Dragons Casino (LV)",    GetSCM(481), 6);
    AddProgress("Caligula's Palace (LV)",      GetSCM(483), 4);
    AddProgress("C.R.A.S.H. (Las Venturas)",   GetSCM(484), 2);
    AddProgress("Madd Dogg (Las Venturas)",    GetSCM(482), 1);
    AddProgress("Caligula's Casino Heist",     GetSCM(485), 6);
    AddProgress("Mansion / Return to LS",      GetSCM(495), 4);
    AddProgress("Grove Street Final Act",      GetSCM(496), 5); // Includes "End of the Line"

    // =========================================================================
    // 2. VEHICLE SCHOOLS (At least Bronze in all tests required for 100%)
    // =========================================================================
    AddHeader("-- SCHOOLS (BRONZE+) --");
    AddBool("Driving School (Doherty)",        GetSCM(464) >= 1);
    AddBool("Boat School (Bayside)",           GetSCM(476) >= 1);
    AddBool("Pilot School (Verdant Meadows)",  GetSCM(475) >= 1);
    AddBool("Bike School (Las Venturas)",      GetSCM(486) >= 1);

    // =========================================================================
    // 3. ASSET ODD JOBS & REVENUE PROPERTIES
    // =========================================================================
    AddHeader("-- ASSET MISSIONS --");
    AddBool("RS Haul Trucking (8 Levels)",     GetSCM(2492) >= 1);
    AddBool("Hunter Quarry (7 Levels)",        GetSCM(2505) >= 1);
    AddProgress("Valet Parking (SF)",          GetSCM(2172), 5);
    AddProgress("Freight Train (2 Levels)",    GetSCM(2544), 2);
    AddBool("Roboi's Food Mart Courier (LS)",  GetSCM(2157) >= 1);
    AddBool("Hippy Shopper Courier (SF)",      GetSCM(2158) >= 1);
    AddBool("Burger Shot Courier (LV)",        GetSCM(2159) >= 1);

    // =========================================================================
    // 4. EMERGENCY & VEHICLE SUB-MISSIONS
    // =========================================================================
    AddHeader("-- VEHICLE SUB-MISSIONS --");
    rows.push_back({ "Paramedic (Level 12)", "[stat id missing]" }); // TODO: STAT_HIGHEST_LEVEL_PARAMEDIC_MISSION does not exist in plugin-sdk
    rows.push_back({ "Firefighter (Level 12)", "[stat id missing]" }); // TODO: STAT_HIGHEST_LEVEL_FIREFIGHTER_MISSION does not exist in plugin-sdk
    rows.push_back({ "Vigilante (Level 12)", "[stat id missing]" }); // TODO: STAT_HIGHEST_LEVEL_VIGILANTE_MISSION does not exist in plugin-sdk
    rows.push_back({ "Taxi Driver (50 Fares)", "[stat id missing]" }); // TODO: STAT_PASSENGERS_DROPPED_OFF_IN_TAXI does not exist in plugin-sdk
    rows.push_back({ "Pimping (Level 10)", "[stat id missing]" }); // TODO: STAT_HIGHEST_LEVEL_PIMPING_MISSION does not exist in plugin-sdk

    // =========================================================================
    // 5. GYM MOVES & CHALLENGES
    // =========================================================================
    AddHeader("-- GYMS & CHALLENGES --");
    AddBool("LS Gym Moves (Boxing)",           GetSCM(2395) >= 1);
    AddBool("SF Gym Moves (Martial Arts)",     GetSCM(2396) >= 1);
    AddBool("LV Gym Moves (Kickboxing)",       GetSCM(2397) >= 1);
    AddBool("BMX Challenge (Glen Park)",       GetSCM(2472) >= 1);
    AddBool("NRG-500 Challenge (Easter Basin)",GetSCM(2473) >= 1);
    AddProgress("Mount Chiliad Challenge",     GetSCM(2482), 3);
    AddBool("Ammu-Nation Shooting Range",      GetSCM(2531) >= 1);
    rows.push_back({ "Import/Export Lists (3x10)", "[stat id missing]" }); // TODO: STAT_CARS_EXPORTED does not exist in plugin-sdk

    // =========================================================================
    // 6. STADIUM EVENTS & STREET RACES (24 RACES + 4 STADIUMS)
    // =========================================================================
    AddHeader("-- STADIUMS & RACES --");
    AddBool("8-Track (LS Stadium)",            GetSCM(2514) >= 1);
    AddBool("Bloodring (SF Stadium)",          GetSCM(2517) >= 1);
    AddBool("Dirtring (LV Stadium)",           GetSCM(2522) >= 1);
    AddBool("Kickstart (LV Stadium >=26 pts)", GetSCM(2527) >= 1);

    // Street Races & Flying Races (24 total: 6 LS, 6 SF, 4 LV Road, 6 LV Air)
    // SCM indices 2550..2573 store 1 when each race is won
    int racesWon = 0;
    for (int i = 0; i < 24; i++) {
        if (GetSCM(2550 + i) >= 1) racesWon++;
    }
    AddProgress("Street & Air Races Won",      racesWon, 24);

    // =========================================================================
    // 7. SAFEHOUSES & COLLECTIBLES (29 Purchases + 250 Collectibles)
    // =========================================================================
    AddHeader("-- SAFEHOUSES & COLLECTIBLES --");
    rows.push_back({ "Safehouses & Suites Bought", "[stat id missing]" }); // TODO: STAT_SAFEHOUSES_VISITED does not exist in plugin-sdk
    AddProgress("Gang Tags Sprayed (LS)",      GetIntStat(STAT_TAGS_SPRAYED), 100);
    AddProgress("Snapshots Taken (SF)",        GetIntStat(STAT_SNAPSHOTS_TAKEN), 50);
    AddProgress("Horseshoes Collected (LV)",   GetIntStat(STAT_HORSESHOES_COLLECTED), 50);
    AddProgress("Oysters Collected (SA)",      GetIntStat(STAT_OYSTERS_COLLECTED), 50);
}

// Global cached checklist for smooth menu scrolling
static std::vector<ChecklistRow> gCachedChecklist;

// Hooked CStats::ConstructStatLine
int __cdecl Hooked_ConstructStatLine(int line, unsigned char category) {
    // Category 7 is the last tab in Pause -> Stats ("Misc", renamed to "100% CHECKLIST")
    if (category == 7) {
        if (line == 0 || gCachedChecklist.empty()) {
            BuildFull100Checklist(gCachedChecklist);
        }

        int checklistCount = (int)gCachedChecklist.size();

        if (line < checklistCount) {
            strncpy(gStatLabelBuffer, gCachedChecklist[line].label.c_str(), 127);
            gStatLabelBuffer[127] = '\0';
            strncpy(gStatValueBuffer, gCachedChecklist[line].value.c_str(), 127);
            gStatValueBuffer[127] = '\0';
            return 1; // Line exists
        }

        // Render normal "Misc" stats underneath our checklist
        return orig_ConstructStatLine(line - checklistCount, category);
    }

    return orig_ConstructStatLine(line, category);
}

// Rename "Misc" tab in Pause -> Stats to "100% CHECKLIST"
typedef const wchar_t* (__fastcall* tCTextGet)(CText* pThis, int edx, const char* key);
tCTextGet orig_CTextGet = (tCTextGet)0x6A0050;

const wchar_t* __fastcall Hooked_CTextGet(CText* pThis, int edx, const char* key) {
    if (key && strcmp(key, "FES_MSC") == 0) {
        return L"100% CHECKLIST";
    }
    return orig_CTextGet(pThis, edx, key);
}

class GTA100ChecklistASI {
public:
    GTA100ChecklistASI() {
        // Redirect all 3 calls to CStats::ConstructStatLine inside CMenuManager
        patch::RedirectCall(0x574B4B, Hooked_ConstructStatLine);
        patch::RedirectCall(0x579D94, Hooked_ConstructStatLine);
        patch::RedirectCall(0x579F0C, Hooked_ConstructStatLine);

        // Rename "Misc" tab label -- DISABLED: RedirectJump overwrites the start of
        // CText::Get, and orig_CTextGet points at that same address, so every text
        // lookup would call itself forever and crash the game on load.
        // patch::RedirectJump(0x6A0050, Hooked_CTextGet);
    }
} gta100ChecklistASI;
