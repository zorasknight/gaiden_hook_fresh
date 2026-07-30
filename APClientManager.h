#pragma once

#include "APState.h"

#include <string>
#include <memory>
#include <cstdint>

#include "apclient.hpp"


//
// AP connection config
//
bool SaveAPConfig(
    const std::string& host,
    int port,
    const std::string& slot,
    const std::string& password
);


bool LoadAPConfig(
    std::string& host,
    int& port,
    std::string& slot,
    std::string& password
);



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
    int connectAttempts = 0;
    static constexpr int MAX_CONNECT_ATTEMPTS = 3;

    //
    // Current connection info
    //
    std::string pendingSlot;
    std::string pendingPassword;

    std::string lastHost;
    int lastPort = 0;


    //
    // Persistent state
    //
    APState state;



    //
    // Runtime flags
    //
    bool inventoryReadyFlag = false;
    bool initialized = false;
    bool connected = false;
};



extern APClientManager g_AP;