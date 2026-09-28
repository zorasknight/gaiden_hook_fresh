#pragma once

#include "APState.h"
#include "APProgressiveItems.h"

#include <string>
#include <memory>
#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>

#include "apclient.hpp"

extern APProgressiveItems g_APProgressiveItems;

// AP connection config

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

struct TimelineEventEntry
{
    uint32_t category;
    uint32_t mission;
    uint32_t state;
    uint32_t stateHint;
    int64_t locationID;
};



// Game-thread actions
// These actions are produced by non-game threads and
// consumed by the main game thread.

enum class GameActionType
{
    DeliverItem,
    TimelineEvent
};

struct GameAction
{
    GameActionType type = GameActionType::DeliverItem;

    // Item delivery
    int64_t itemID = 0;
    int itemIndex = -1;

    // Timeline event
    uint32_t category = 0;
    uint32_t mission = 0;
    uint32_t state = 0;
    uint8_t p5 = 0;
    uint8_t p6 = 0;
};


class APClientManager
{
public:

    bool Initialize(
        const std::string& statePath
    );

    bool GetPythonConnectionInfo(
        std::string& host,
        int& port,
        std::string& slot,
        std::string& password
    );

    void Update();
    void CheckAPProgress();


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

    void SendGoal();



    // Location sending

    void CheckLocation(
        int64_t location
    );

    void FlushLocationQueue();

    // Incoming items

    void ReceiveItem(
        int64_t item_id,
        int index
    );

    int GetAPProgress();

    void SetAPProgress(
        int progress
    );

    void FlushItemQueue();


    // Inventory

    void InventoryReady();

    bool IsConnected();


    // Game-thread action queue

    void QueueGameAction(
        const GameAction& action
    );
    bool AreWinconsComplete();
    void MarkWinconComplete(APWincon wincon);

private:


    // Game-thread action processing
    bool automatedConnection = false;

    void DrainGameActions();

    void ProcessGameAction(
        const GameAction& action
    );

    std::vector<TimelineEventEntry> timelineEvents;

    void LoadTimelineEvents(
        const std::string& path
    );

    void ProcessTimelineEvent(
        uint32_t category,
        uint32_t mission,
        uint32_t state
    );


    // AP worker thread

    void APWorkerThread();


    std::unique_ptr<APClient> client;

    int connectAttempts = 0;

    static constexpr int MAX_CONNECT_ATTEMPTS = 3;


    // Current connection info

    std::string pendingSlot;
    std::string pendingPassword;

    std::string lastHost;
    int lastPort = 0;



    // Persistent state

    APState state;


    // Runtime flags

    bool inventoryReadyFlag = false;
    bool initialized = false;
    bool connected = false;
    bool goalSent = false;


    // Game-thread action queue

    std::queue<GameAction> gameActionQueue;
    std::mutex gameActionMutex;


    // AP worker

    std::thread apWorker;
    bool apWorkerRunning = false;
};


extern APClientManager g_AP;