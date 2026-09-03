#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>


struct APQueuedLocation
{
    int64_t location_id;

    bool queued = true;
    bool sent = false;
    bool acknowledged = false;
};

enum class APWincon
{
    GoldenBall,
    DefeatShishido,
    DefeatPocketCircuitOwner
};


struct APReceivedItem
{
    int64_t item_id;
    int64_t location_id;
    int index;

    bool queued = true;
    bool delivered = false;
};


class APState
{
public:

    bool Load();
    bool Save();

    void SetFilename(const std::string& path);
    void MarkWinconComplete(APWincon wincon);
    bool IsWinconComplete(APWincon wincon) const;


    void QueueLocation(int64_t location);

    bool IsLocationQueued(int64_t location);
    bool IsLocationAcknowledged(int64_t location);

    void MarkLocationSent(int64_t location);
    void MarkLocationAcknowledged(int64_t location);

    std::vector<APQueuedLocation>& GetQueuedLocations();
    const std::vector<APQueuedLocation>& GetQueuedLocations() const;

    bool HasLocationHinted(int64_t location) const;
    void MarkLocationHinted(int64_t location);

    void QueueReceivedItem(
        int64_t item,
        int64_t location,
        int index
    );

    bool IsItemDelivered(int index);
    void MarkItemDelivered(int index);

    std::vector<APReceivedItem>& GetReceivedItems();
    const std::vector<APReceivedItem>& GetReceivedItems() const;


private:
    bool goldenBallWincon = false;
    bool defeatShishidoWincon = false;
    bool defeatPocketCircuitOwnerWincon = false;

    std::string filename;

    std::vector<APQueuedLocation> queuedLocations;
    std::vector<APReceivedItem> receivedItems;


    std::unordered_set<int64_t> hintedLocations;
};