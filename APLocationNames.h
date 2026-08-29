#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>

struct APLocationInfo
{
    std::string label;
    std::string location;
    std::string source;
    int64_t id;
};

class APLocationNames
{
public:

    bool Load(
        const std::string& path
    );

    std::string GetName(
        int64_t location_id
    );

    std::vector<int64_t> GetShopLocationsFor(
        int64_t location_id
    );

private:

    std::unordered_map<int64_t, std::string> names;

    std::unordered_map<int64_t, APLocationInfo> locations;
};

extern APLocationNames g_APLocationNames;