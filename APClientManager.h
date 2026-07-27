#pragma once

#include "APState.h"

#include <string>
#include <memory>
#include <cstdint>

#include "apclient.hpp"


class APClientManager
{
public:

    bool Initialize(
        const std::string& statePath
    );


    void Update();



    bool Connect(
        const std::string& host,
        int port
    );


    bool ConnectSlot(
        const std::string& slot,
        const std::string& password
    );



    void Say(
        const std::string& message
    );



    //
    // Location sending
    //
    void CheckLocation(
        int64_t location
    );


    void FlushLocationQueue();



    //
    // Incoming items
    //
    void ReceiveItem(
        int64_t item_id
    );


    void FlushItemQueue();



    //
    // Inventory
    //
    void InventoryReady();



    bool IsConnected();



private:

    std::unique_ptr<APClient> client;


    std::string pendingSlot;
    std::string pendingPassword;



    APState state;



    bool inventoryReadyFlag = false;
    bool initialized = false;
    bool connected = false;
};



extern APClientManager g_AP;