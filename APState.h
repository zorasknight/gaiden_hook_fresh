#pragma once

#include <cstdint>
#include <string>
#include <vector>


struct APCheckedLocation
{
    int64_t location_id;
    bool checked;
};


struct APReceivedItem
{
    int64_t item_id;
    int64_t location_id;
    int index;
    bool received;
};


class APState
{
public:

    bool Load();
    bool Save();


    void AddCheckedLocation(int64_t location);
    bool HasCheckedLocation(int64_t location);


    void AddReceivedItem(
        int64_t item,
        int64_t location,
        int index
    );


    bool IsItemReceived(int index);
    void MarkItemReceived(int index);
    std::vector<APReceivedItem>& GetReceivedItems();
    std::vector<APCheckedLocation>& GetCheckedLocations();

    const std::vector<APReceivedItem>& GetReceivedItems() const;
    const std::vector<APCheckedLocation>& GetCheckedLocations() const;


private:

    std::string filename =
        "data/APState.json";


    std::vector<APCheckedLocation> checkedLocations;
    std::vector<APReceivedItem> receivedItems;
};