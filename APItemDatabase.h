#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>


struct APItemEntry
{
    int64_t item_id;
    int64_t location_id;
};


class APItemDatabase
{
public:

    bool Load(const std::string& path);

    bool GetItem(
        int64_t game_item_id,
        APItemEntry& out
    );


private:

    std::unordered_map<int64_t, APItemEntry> items;
};


extern APItemDatabase g_APItems;