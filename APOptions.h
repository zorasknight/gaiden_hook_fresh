#pragma once

#include <string>

class APOptions
{
public:

    bool Load(
        const std::string& path
    );

    int GetMaxGoldenBallCount() const;

    int GetRequiredGoldenBallCount() const;

    bool GetGoldenBallWincon() const;
    bool GetDefeatShishidoWincon() const;
    bool GetDefeatPocketCircuitOwnerWincon() const;

private:

    int maxGoldenBallCount = 0;
    int requiredGoldenBallCount = 0;

    int goldenBallWincon = 0;
    int defeatShishidoWincon = 0;
    int defeatPocketCircuitOwnerWincon = 0;
};

extern APOptions g_APOptions;