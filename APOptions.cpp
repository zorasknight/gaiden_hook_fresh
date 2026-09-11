#include "APOptions.h"

#include <fstream>
#include <cstdio>

#include <nlohmann/json.hpp>
#include "Console.h"

using json = nlohmann::json;


APOptions g_APOptions;


bool APOptions::Load(
    const std::string& path
)
{
    std::ifstream file(path);

    if (!file)
    {
        printf(
            "/r[AP] Failed to open options: %s\n",
            path.c_str()
        );

        return false;
    }

    try
    {
        json data;

        file >> data;

        maxGoldenBallCount =
            data.value(
                "max_golden_ball_count",
                0
            );

        requiredGoldenBallCount =
            data.value(
                "required_golden_ball_count",
                0
            );

        goldenBallWincon =
            data.value(
                "golden_ball_wincon",
                0
            );

        defeatShishidoWincon =
            data.value(
                "defeat_shishido_wincon",
                0
            );

        defeatPocketCircuitOwnerWincon =
            data.value(
                "defeat_pocket_circuit_owner_wincon",
                0
            );

        pocketCircuit =
            data.value(
                "pocket_circuit",
                0
            );

        if (goldenBallWincon == 1){
            printf(
                "/b[AP] Wincon: /g%d|%d /bgolden balls required\n",
                requiredGoldenBallCount,
                maxGoldenBallCount
            );
        }
        if (defeatShishidoWincon == 1) {
            printf(
                "/b[AP] Wincon: Defeat Shishido\n"
            );
        }
        if (defeatPocketCircuitOwnerWincon == 1) {
            printf(
                "/b[AP] Wincon: Defeat Pocket Circuit Owner Rival\n"
            );
        }

        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "/r[AP] Failed to load options: %s\n",
            e.what()
        );

        maxGoldenBallCount = 0;
        requiredGoldenBallCount = 0;
        goldenBallWincon = 0;
        defeatShishidoWincon = 0;
        defeatPocketCircuitOwnerWincon = 0;
        pocketCircuit = 0;

        return false;
    }
}


int APOptions::GetMaxGoldenBallCount() const
{
    return maxGoldenBallCount;
}

int APOptions::GetRequiredGoldenBallCount() const
{
    return requiredGoldenBallCount;
}

bool APOptions::GetGoldenBallWincon() const
{
    return goldenBallWincon != 0;
}

bool APOptions::GetDefeatShishidoWincon() const
{
    return defeatShishidoWincon != 0;
}

bool APOptions::GetDefeatPocketCircuitOwnerWincon() const
{
    return defeatPocketCircuitOwnerWincon != 0;
}

bool APOptions::GetPocketCircuit() const
{
    return pocketCircuit != 0;
}