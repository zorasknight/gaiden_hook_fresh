#include "APItemDatabase.h"

#include <fstream>
#include <sstream>
#include <cstdio>


APItemDatabase g_APItems;


bool APItemDatabase::Load(
    const std::string& path
)
{
    printf(
        "[AP] Opening CSV: %s\n",
        path.c_str()
    );


    std::ifstream file(path);


    if (!file.is_open())
    {
        printf(
            "[AP] Failed to open CSV\n"
        );

        return false;
    }


    printf(
        "[AP] CSV opened\n"
    );


    std::string line;


    // skip header
    if (std::getline(file, line))
    {
        printf(
            "[AP] Header: %s\n",
            line.c_str()
        );
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

        printf(
            "[AP CSV] Game Item %lld -> Location %lld\n",
            gameID,
            entry.location_id
        );

        items[gameID] = entry;


        printf(
            "[AP] Loaded %s -> location %s\n",
            key.c_str(),
            location.c_str()
        );
    }


    printf(
        "[AP] Total mappings: %zu\n",
        items.size()
    );


    return true;
}



bool APItemDatabase::GetItem(
    int64_t game_item_id,
    APItemEntry& out
)
{
    printf(
        "[AP] Looking for item %lld\n",
        game_item_id
    );


    auto it = items.find(game_item_id);


    if (it == items.end())
    {
        printf(
            "[AP] No mapping for item %lld\n",
            game_item_id
        );

        return false;
    }


    out = it->second;


    printf(
        "[AP] Found item %lld -> location %lld\n",
        game_item_id,
        out.location_id
    );


    return true;
}