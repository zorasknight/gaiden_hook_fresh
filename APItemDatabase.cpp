#include "APItemDatabase.h"

#include <fstream>
#include <sstream>
#include <cstdio>
#include "Console.h"

APItemDatabase g_APItems;


bool APItemDatabase::Load(
    const std::string& path
)
{


    std::ifstream file(path);


    if (!file.is_open())
    {
        printf(
            "/r[AP] Failed to open CSV\n"
        );

        return false;
    }



    std::string line;


    // skip header
    if (std::getline(file, line))
    {

    }


    while (std::getline(file, line))
    {
        if (line.empty())
            continue;


        std::stringstream ss(line);


        std::string key;
        std::string item;
        std::string location;


        std::getline(ss, key, ',');
        std::getline(ss, item, ',');
        std::getline(ss, location, ',');


        if (key.empty())
            continue;


        APItemEntry entry;

        entry.item_id = 0; // unused for now

        if (!item.empty())
        {
            entry.item_id = std::stoll(item);
        }

        entry.location_id = std::stoll(location);

        int64_t gameID = std::stoll(key);

        items[gameID] = entry;
    }



    return true;
}



bool APItemDatabase::GetItem(
    int64_t game_item_id,
    APItemEntry& out
)
{


    auto it = items.find(game_item_id);


    if (it == items.end())
    {

        return false;
    }


    out = it->second;


    return true;
}

bool APItemDatabase::HasLocation(
    int64_t location_id
)
{
    for (const auto& pair : items)
    {
        if (pair.second.location_id == location_id)
        {
            return true;
        }
    }

    return false;
}