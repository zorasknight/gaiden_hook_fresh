#include "APOptions.h"

#include <fstream>
#include <cstdio>

#include <nlohmann/json.hpp>

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
            "[AP] Failed to open options: %s\n",
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


        printf(
            "[AP] Loaded options: %d/%d golden balls required\n",
            requiredGoldenBallCount,
            maxGoldenBallCount
        );


        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Failed to load options: %s\n",
            e.what()
        );

        maxGoldenBallCount = 0;
        requiredGoldenBallCount = 0;

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