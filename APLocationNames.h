#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

class APLocationNames
{
public:

    bool Load(
        const std::string& path
    );

    std::string GetName(
        int64_t location_id
    );

private:

    std::unordered_map<int64_t, std::string> names;
};

extern APLocationNames g_APLocationNames;