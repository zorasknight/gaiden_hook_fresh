#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct ProgressiveItem
{
    std::string name;
    std::vector<int> levels;
};

class APProgressiveItems
{
public:
    bool Load();

    const ProgressiveItem* Get(int itemID) const;

private:
    std::unordered_map<int, ProgressiveItem> items;
    std::string filename = "progressive_items.json";
};