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


private:

    int maxGoldenBallCount = 0;

    int requiredGoldenBallCount = 0;
};


extern APOptions g_APOptions;