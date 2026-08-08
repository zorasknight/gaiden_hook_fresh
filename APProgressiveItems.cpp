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

            if (value.contains("levels"))
            {
                for (const auto& level : value["levels"])
                {
                    item.levels.push_back(
                        level.get<int>()
                    );
                }
            }

            if (value.contains("checks"))
            {
                for (const auto& check : value["checks"])
                {
                    item.checks.push_back(
                        check.get<int64_t>()
                    );
                }
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
APProgressiveItems::FindByLevel(
    int levelID,
    int& levelIndex
) const
{
    levelIndex = -1;

    for (const auto& [progressiveID, item] : items)
    {
        for (
            int i = 0;
            i < static_cast<int>(item.levels.size());
            i++
            )
        {
            if (item.levels[i] == levelID)
            {
                levelIndex = i;

                return &item;
            }
        }
    }

    return nullptr;
}


const ProgressiveItem*
APProgressiveItems::Get(
    int itemID
) const
{
    auto it =
        items.find(itemID);

    if (it == items.end())
        return nullptr;

    return &it->second;
}


APProgressiveItems g_APProgressiveItems;