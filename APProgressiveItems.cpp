#include "APProgressiveItems.h"

#include <fstream>
#include <filesystem>
#include <stdio.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;


bool APProgressiveItems::Load()
{
    std::filesystem::path statePath(filename);

    if (!statePath.parent_path().empty())
    {
        std::filesystem::create_directories(
            statePath.parent_path()
        );
    }

    items.clear();

    std::ifstream file(filename);

    if (!file.good())
    {
        printf(
            "[AP] progressive_items.json not found\n"
        );

        return false;
    }

    try
    {
        json data;

        file >> data;

        for (auto& [idString, value] : data.items())
        {
            int progressiveID =
                std::stoi(idString);

            ProgressiveItem item;

            item.name =
                value.value(
                    "name",
                    ""
                );

            for (const auto& level : value["levels"])
            {
                item.levels.push_back(
                    level.get<int>()
                );
            }

            items[progressiveID] =
                std::move(item);
        }
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Failed to load progressive_items.json: %s\n",
            e.what()
        );

        items.clear();

        return false;
    }

    return true;
}


const ProgressiveItem*
APProgressiveItems::Get(int itemID) const
{
    auto it = items.find(itemID);

    if (it == items.end())
        return nullptr;

    return &it->second;
}