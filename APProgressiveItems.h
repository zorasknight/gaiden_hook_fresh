#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct ProgressiveItem
{
    std::string name;

    // Existing progressive item data
    std::vector<int> levels;

    // New progressive location data
    std::vector<int64_t> checks;
};

class APProgressiveItems
{
public:
    bool Load();

    const ProgressiveItem* Get(int itemID) const;
    const ProgressiveItem* FindByLevel(
        int levelID,
        int& levelIndex
    ) const;

private:
    std::unordered_map<int, ProgressiveItem> items;
    std::string filename =
        "mods/Gaiden_Rando/progressive_items.json";
};

extern APProgressiveItems g_APProgressiveItems;