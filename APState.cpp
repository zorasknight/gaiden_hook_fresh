#include "APState.h"

#include <fstream>
#include <cstdio>
#include <filesystem>

#include <nlohmann/json.hpp>


using json = nlohmann::json;



bool APState::Load()
{
    std::filesystem::path statePath(filename);

    if (!statePath.parent_path().empty())
    {
        std::filesystem::create_directories(
            statePath.parent_path()
        );
    }


    queuedLocations.clear();
    receivedItems.clear();
    hintedLocations.clear();
    goldenBallWincon = false;
    defeatShishidoWincon = false;
    defeatPocketCircuitOwnerWincon = false;


    std::ifstream file(filename);


    if (!file.good())
    {

        return Save();
    }



    json data;

    file >> data;



    //
    // New format
    //
    if (data.contains("queued_locations"))
    {
        for (auto& loc : data["queued_locations"])
        {
            queuedLocations.push_back(
                {
                    loc["location_id"],
                    loc.value("queued", true),
                    loc.value("sent", false),
                    loc.value("acknowledged", false)
                }
            );
        }
    }


    //
    // Old format compatibility
    //
    else if (data.contains("checked_locations"))
    {
        for (auto& loc : data["checked_locations"])
        {
            bool checked =
                loc.value("checked", false);


            queuedLocations.push_back(
                {
                    loc["location_id"],
                    !checked,
                    checked,
                    checked
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
                    item["item_id"].get<int64_t>(),
                    item["location_id"].get<int64_t>(),
                    item["index"].get<int>(),

                    item.value("queued", true),
                    item.value("delivered", false)
                }
            );
        }
    }

    if (data.contains("hinted_locations"))
    {
        for (auto& location : data["hinted_locations"])
        {
            hintedLocations.insert(
                location.get<int64_t>()
            );
        }
    }


    if (data.contains("wincons"))
    {
        auto& wincons = data["wincons"];

        goldenBallWincon =
            wincons.value("golden_ball", false);

        defeatShishidoWincon =
            wincons.value("defeat_shishido", false);

        defeatPocketCircuitOwnerWincon =
            wincons.value("defeat_pocket_circuit_owner", false);

        if (goldenBallWincon)
        {
            printf(
                "[AP] Goal completed: Golden Ball\n"
            );
        }

        if (defeatShishidoWincon)
        {
            printf(
                "[AP] Goal completed: Defeat Shishido\n"
            );
        }

        if (defeatPocketCircuitOwnerWincon)
        {
            printf(
                "[AP] Goal completed: Defeat Pocket Circuit Owner\n"
            );
        }
    }


    return true;
}





bool APState::Save()
{
    std::filesystem::path statePath(filename);


    if (!statePath.parent_path().empty())
    {
        std::filesystem::create_directories(
            statePath.parent_path()
        );
    }



    json data;


    data["version"] = 7;



    data["queued_locations"] = json::array();


    for (auto& loc : queuedLocations)
    {
        data["queued_locations"].push_back(
            {
                {"location_id", loc.location_id},
                {"queued", loc.queued},
                {"sent", loc.sent},
                {"acknowledged", loc.acknowledged}
            }
        );
    }



    data["received_items"] =
        json::array();


    for (auto& item : receivedItems)
    {
        data["received_items"].push_back(
            {
                {"item_id", item.item_id},
                {"location_id", item.location_id},
                {"index", item.index},
                {"queued", item.queued},
                {"delivered", item.delivered}
            }
        );
    }

    data["hinted_locations"] = json::array();

    for (int64_t location : hintedLocations)
    {
        data["hinted_locations"].push_back(location);
    }

    data["wincons"] =
    {
        {"golden_ball", goldenBallWincon},
        {"defeat_shishido", defeatShishidoWincon},
        {"defeat_pocket_circuit_owner", defeatPocketCircuitOwnerWincon}
    };


    std::ofstream file(filename);


    if (!file.good())
    {
        printf(
            "[AP ERROR] Failed to save state file\n"
        );

        return false;
    }



    file << data.dump(4);



    if (!file.good())
    {
        printf(
            "[AP ERROR] Failed writing state file\n"
        );

        return false;
    }


    return true;
}






// =====================================================
// Location queue
// =====================================================


void APState::QueueLocation(
    int64_t location
)
{
    for (auto& entry : queuedLocations)
    {
        if (entry.location_id == location)
            return;
    }


    queuedLocations.push_back(
        {
            location,
            true,
            false,
            false
        }
    );
}



bool APState::IsLocationQueued(
    int64_t location
)
{
    for (auto& entry : queuedLocations)
    {
        if (entry.location_id == location)
            return entry.queued;
    }


    return false;
}



bool APState::IsLocationAcknowledged(
    int64_t location
)
{
    for (auto& entry : queuedLocations)
    {
        if (entry.location_id == location)
            return entry.acknowledged;
    }


    return false;
}



void APState::MarkLocationSent(
    int64_t location
)
{
    for (auto& entry : queuedLocations)
    {
        if (entry.location_id == location)
        {
            entry.sent = true;
            return;
        }
    }
}



void APState::MarkLocationAcknowledged(
    int64_t location
)
{
    for (auto& entry : queuedLocations)
    {
        if (entry.location_id == location)
        {
            entry.acknowledged = true;
            entry.queued = false;
            return;
        }
    }
}





// =====================================================
// Incoming items
// =====================================================


void APState::QueueReceivedItem(
    int64_t item,
    int64_t location,
    int index
)
{
    for (auto& entry : receivedItems)
    {
        if (entry.index == index)
            return;
    }


    receivedItems.push_back(
        {
            item,
            location,
            index,
            true,
            false
        }
    );
}



bool APState::IsItemDelivered(
    int index
)
{
    for (auto& item : receivedItems)
    {
        if (item.index == index)
            return item.delivered;
    }


    return false;
}



void APState::MarkItemDelivered(
    int index
)
{
    for (auto& item : receivedItems)
    {
        if (item.index == index)
        {
            item.delivered = true;
            item.queued = false;
            return;
        }
    }
}


bool APState::HasLocationHinted(
    int64_t location
) const
{
    return hintedLocations.find(location) != hintedLocations.end();
}


void APState::MarkLocationHinted(
    int64_t location
)
{
    hintedLocations.insert(location);
}


void APState::SetFilename(
    const std::string& path
)
{
    filename = path;


    std::filesystem::path statePath(filename);

}


void APState::MarkWinconComplete(
    APWincon wincon
)
{
    switch (wincon)
    {
    case APWincon::GoldenBall:
        goldenBallWincon = true;
        break;

    case APWincon::DefeatShishido:
        defeatShishidoWincon = true;
        break;

    case APWincon::DefeatPocketCircuitOwner:
        defeatPocketCircuitOwnerWincon = true;
        break;
    }
}


bool APState::IsWinconComplete(
    APWincon wincon
) const
{
    switch (wincon)
    {
    case APWincon::GoldenBall:
        return goldenBallWincon;

    case APWincon::DefeatShishido:
        return defeatShishidoWincon;

    case APWincon::DefeatPocketCircuitOwner:
        return defeatPocketCircuitOwnerWincon;
    }

    return false;
}



std::vector<APQueuedLocation>& APState::GetQueuedLocations()
{
    return queuedLocations;
}



const std::vector<APQueuedLocation>& APState::GetQueuedLocations() const
{
    return queuedLocations;
}



std::vector<APReceivedItem>& APState::GetReceivedItems()
{
    return receivedItems;
}



const std::vector<APReceivedItem>& APState::GetReceivedItems() const
{
    return receivedItems;
}