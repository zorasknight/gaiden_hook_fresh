#pragma once

#include <cstdint>
#include <string>
#include <vector>


struct APQueuedLocation
{
    int64_t location_id;

    bool queued = true;
    bool sent = false;
    bool acknowledged = false;
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


    // =========================
    // Outgoing location queue
    // =========================

    void QueueLocation(int64_t location);

    bool IsLocationQueued(int64_t location);
    bool IsLocationAcknowledged(int64_t location);

    void MarkLocationSent(int64_t location);
    void MarkLocationAcknowledged(int64_t location);

    std::vector<APQueuedLocation>& GetQueuedLocations();
    const std::vector<APQueuedLocation>& GetQueuedLocations() const;


    // =========================
    // Incoming AP item queue
    // =========================

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

    std::string filename;

    std::vector<APQueuedLocation> queuedLocations;
    std::vector<APReceivedItem> receivedItems;
};