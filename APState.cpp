#include "APState.h"

#include <fstream>
#include <cstdio>
#include <filesystem>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


bool APState::Load()
{
    std::filesystem::create_directories("data");

    checkedLocations.clear();
    receivedItems.clear();


    std::ifstream file(filename);

    if (!file.good())
    {
        printf("[AP] No state file found, creating one\n");
        return Save();
    }


    json data;
    file >> data;


    if (data.contains("checked_locations"))
    {
        for (auto& loc : data["checked_locations"])
        {
            checkedLocations.push_back(
                {
                    loc["location_id"],
                    loc["checked"]
                }
            );
        }
    }


    if (data.contains("received_items"))
    {
        for (auto& item : data["received_items"])
        {
            receivedItems.push_back(
                {
                    item["item_id"],
                    item["location_id"],
                    item["index"],
                    item["received"]
                }
            );
        }
    }


    printf(
        "[AP] Loaded state: %zu locations, %zu items\n",
        checkedLocations.size(),
        receivedItems.size()
    );


    return true;
}



bool APState::Save()
{
    std::filesystem::create_directories("data");


    json data;

    data["version"] = 1;


    data["checked_locations"] = json::array();

    for (auto& loc : checkedLocations)
    {
        data["checked_locations"].push_back(
            {
                {"location_id", loc.location_id},
                {"checked", loc.checked}
            });
    }


    data["received_items"] = json::array();

    for (auto& item : receivedItems)
    {
        data["received_items"].push_back(
            {
                {"item_id", item.item_id},
                {"location_id", item.location_id},
                {"index", item.index},
                {"received", item.received}
            });
    }


    std::ofstream file(filename);

    if (!file.good())
    {
        printf("[AP ERROR] Failed to save state file\n");
        return false;
    }


    file << data.dump(4);

    if (!file.good())
    {
        printf("[AP ERROR] Failed writing state file\n");
        return false;
    }

    return true;
}



void APState::AddCheckedLocation(int64_t location)
{
    for (auto& entry : checkedLocations)
    {
        if (entry.location_id == location)
        {
            entry.checked = true;
            return;
        }
    }


    checkedLocations.push_back(
        {
            location,
            true
        });
}



bool APState::HasCheckedLocation(int64_t location)
{
    for (const auto& entry : checkedLocations)
    {
        if (entry.location_id == location)
        {
            return entry.checked;
        }
    }


    return false;
}



void APState::AddReceivedItem(
    int64_t item,
    int64_t location,
    int index
)
{
    for (auto& entry : receivedItems)
    {
        if (entry.index == index)
        {
            return;
        }
    }


    receivedItems.push_back(
        {
            item,
            location,
            index,
            false
        });
}

void APState::MarkItemReceived(int index)
{
    for (auto& item : receivedItems)
    {
        if (item.index == index)
        {
            item.received = true;
            return;
        }
    }
}

bool APState::IsItemReceived(int index)
{
    for (const auto& item : receivedItems)
    {
        if (item.index == index)
        {
            return item.received;
        }
    }


    return false;
}



std::vector<APReceivedItem>& APState::GetReceivedItems()
{
    return receivedItems;
}



std::vector<APCheckedLocation>& APState::GetCheckedLocations()
{
    return checkedLocations;
}

const std::vector<APReceivedItem>& APState::GetReceivedItems() const
{
    return receivedItems;
}


const std::vector<APCheckedLocation>& APState::GetCheckedLocations() const
{
    return checkedLocations;
}

