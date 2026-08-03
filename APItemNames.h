#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>


class APItemNames
{
public:

    bool Load(
        const std::string& path
    );


    std::string GetName(
        int64_t item_id
    );


private:

    std::unordered_map<int64_t, std::string> names;
};


extern APItemNames g_APItemNames;