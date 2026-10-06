#include "APClientManager.h"

#include "APItemDatabase.h"
#include "hooks.h"
#include "APOptions.h"
#include <cstdio>
#include <climits>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <asio.hpp>

#include "debug.h"
#include "APItemNames.h"
#include "APLocationNames.h"
#include "APProgressiveItems.h"
#include "Console.h"

APClientManager g_AP;

static const char* AP_CONFIG = "mods/Gaiden_Rando/APConfig.json";
constexpr int64_t SHISHIDO_WINCON_LOCATION = 2000;
constexpr int64_t POCKET_CIRCUIT_OWNER_WINCON_LOCATION = 2001;

bool SaveAPConfig(
    const std::string& host,
    int port,
    const std::string& slot,
    const std::string& password
);

bool APClientManager::GetPythonConnectionInfo(
    std::string& host,
    int& port,
    std::string& slot,
    std::string& password
)
{
    try
    {
        asio::io_context io;

        asio::ip::tcp::socket socket(io);

        socket.connect({
            asio::ip::make_address("127.0.0.1"),
            38282
            });

        asio::write(
            socket,
            asio::buffer("STATUS\n")
        );

        asio::streambuf buffer;

        asio::read_until(
            socket,
            buffer,
            '\n'
        );

        std::istream stream(&buffer);

        nlohmann::json json;

        stream >> json;

        if (!json.value("connected", false))
            return false;

        std::string server =
            json.value("server", "");

        size_t separator =
            server.rfind(':');

        if (separator == std::string::npos)
            return false;

        host = server.substr(0, separator);
        port = std::stoi(server.substr(separator + 1));

        slot = json.value("slot", "");
        password = json.value("password", "");

        return !host.empty() && !slot.empty();
    }
    catch (...)
    {
        return false;
    }
}

bool LoadAPConfig(
    std::string& host,
    int& port,
    std::string& slot,
    std::string& password
);


bool LoadAPConfig(
    std::string& host,
    int& port,
    std::string& slot,
    std::string& password
)
{
    std::ifstream file(AP_CONFIG);

    if (!file)
        return false;


    nlohmann::json json;

    try
    {
        file >> json;

        host = json["host"];
        port = json["port"];
        slot = json["slot"];
        password = json["password"];

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void APClientManager::LoadTimelineEvents(
    const std::string& path
)
{
    timelineEvents.clear();

    std::ifstream file(path);

    if (!file)
    {
        printf(
            "/r[Timeline] Event list not found: %s\n",
            path.c_str()
        );

        return;
    }

    std::string line;

    // Skip CSV header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string locationName;
        std::string type;
        std::string category;
        std::string mission;
        std::string state;
        std::string stateHint;
        std::string locationID;

        std::getline(ss, locationName, ',');
        std::getline(ss, type, ',');
        std::getline(ss, category, ',');
        std::getline(ss, mission, ',');
        std::getline(ss, state, ',');
        std::getline(ss, stateHint, ',');
        std::getline(ss, locationID, ',');

        if (
            category.empty() ||
            mission.empty() ||
            state.empty() ||
            locationID.empty()
            )
        {
            continue;
        }

        TimelineEventEntry entry;

        entry.category =
            static_cast<uint32_t>(
                std::stoul(category)
                );

        entry.mission =
            static_cast<uint32_t>(
                std::stoul(mission)
                );

        entry.state =
            static_cast<uint32_t>(
                std::stoul(state)
                );

        entry.stateHint = UINT32_MAX;

        if (!stateHint.empty())
        {
            entry.stateHint =
                static_cast<uint32_t>(
                    std::stoul(stateHint)
                    );
        }

        entry.locationID =
            std::stoll(locationID);

        timelineEvents.push_back(entry);
    }


    printf( "/b[Timeline] Loaded /g%zu /wevent entries\n", timelineEvents.size() );   
}

void APClientManager::ProcessTimelineEvent(
    uint32_t category,
    uint32_t mission,
    uint32_t stateValue
)
{
    if (debug)
    {
        printf(
            "[Timeline] Event %u/%u/%u\n",
            category,
            mission,
            stateValue
        );
    }

    for (const auto& event : timelineEvents)
    {
        if (
            event.category == category &&
            event.mission == mission
            )
        {

            // Actual event state

            if (event.state == stateValue)
            {
                //printf(
                //    "[Timeline] Matched event %u/%u/%u -> location %lld\n",
                //    category,
                //    mission,
                //    stateValue,
                //    event.locationID
                //);

                if (
                    event.locationID ==
                    SHISHIDO_WINCON_LOCATION
                    )
                {
                    printf(
                        "/y[AP] Shishido wincon complete!\n"
                    );

                    MarkWinconComplete(
                        APWincon::DefeatShishido
                    );
                }
                else if (
                    event.locationID ==
                    POCKET_CIRCUIT_OWNER_WINCON_LOCATION
                    )
                {
                    printf(
                        "/y[AP] Pocket Circuit Owner wincon complete!\n"
                    );

                    MarkWinconComplete(
                        APWincon::DefeatPocketCircuitOwner
                    );
                }
                else
                {
                    CheckLocation(
                        event.locationID
                    );
                }
            }


            // Independent hint state

            if (
                event.stateHint != UINT32_MAX &&
                event.stateHint == stateValue
                )
            {
                if (g_APItems.HasLocation(event.locationID))
                {
                    if (state.HasLocationHinted(event.locationID))
                    {
                        //printf(
                        //    "[AP] Hint already sent for location %s (%lld), skipping\n",
                        //    g_APLocationNames.GetName(
                        //        event.locationID
                        //    ).c_str(),
                        //    event.locationID
                        //);
                    }
                    else
                    {
                        //printf(
                        //    "[AP] Creating timeline hint for location %s (%lld)\n",
                        //    g_APLocationNames.GetName(
                        //        event.locationID
                        //    ).c_str(),
                        //    event.locationID
                        //);

                        client->CreateHints(
                            { event.locationID }
                        );

                        state.MarkLocationHinted(
                            event.locationID
                        );

                        state.Save();
                    }
                }
                else
                {
                    //printf(
                    //    "[AP] Timeline hint matched %u/%u/%u, but location %lld is not in item mapping\n",
                    //    category,
                    //    mission,
                    //    stateValue,
                    //    event.locationID
                    //);
                }
            }
        }
    }
}

void APClientManager::MarkWinconComplete(APWincon wincon)
{
    state.MarkWinconComplete(wincon);
    state.Save();

    SendGoal();
}


bool APClientManager::Initialize(
    const std::string& statePath
)
{
    state.SetFilename(statePath);

    state.Load();

    g_APItemNames.Load(
        "mods/Gaiden_Rando/items.json"
    );


    g_APLocationNames.Load(
        "mods/Gaiden_Rando/locations.json"
    );

    g_APOptions.Load(
        "mods/Gaiden_Rando/options.json"
    );


    g_APProgressiveItems.Load();

    LoadTimelineEvents(
        "mods/Gaiden_Rando/event_list.csv"
    );

    initialized = true;

    printf("/b[AP] Manager initialized\n");

    std::string host;
    std::string slot;
    std::string password;
    int port = 0;


    if (GetPythonConnectionInfo(
        host,
        port,
        slot,
        password))
    {
        printf(
            "/g[Python] Using Python connection: %s:%d\n",
            host.c_str(),
            port
        );

        pendingSlot = slot;
        pendingPassword = password;
        automatedConnection = true;

        SaveAPConfig(
            host,
            port,
            slot,
            password
        );

        Connect(host, port);
    }
    else if (LoadAPConfig(
        host,
        port,
        slot,
        password))
    {
        printf(
            "[AP] Previous connection found: /g%s:%d\n",
            host.c_str(),
            port
        );

        pendingSlot = slot;
        pendingPassword = password;
        automatedConnection = true;

        Connect(host, port);
    }



    apWorkerRunning = true;

    apWorker =
        std::thread(
            &APClientManager::APWorkerThread,
            this
        );


    return true;
}




bool SaveAPConfig(
    const std::string& host,
    int port,
    const std::string& slot,
    const std::string& password
)
{
    nlohmann::json json;

    json["host"] = host;
    json["port"] = port;
    json["slot"] = slot;
    json["password"] = password;


    std::ofstream file(AP_CONFIG);

    if (file)
    {
        file << json.dump(4);

        printf("/b[AP] Connection saved\n");

        return true;
    }

    return false;
}


bool APClientManager::Connect(
    const std::string& host,
    int port
)
{
    if (client)
    {
        printf("/r[AP] Closing previous client\n");
        client.reset();
    }

    try
    {
        std::string address =
            host + ":" + std::to_string(port);

        lastHost = host;
        lastPort = port;
        connectAttempts = 0;

        printf(
            "[AP] Connecting to /g%s\n",
            address.c_str()
        );


        LoadLibraryA(
            "mods\\Gaiden_Rando\\libcrypto-3-x64.dll"
        );

        LoadLibraryA(
            "mods\\Gaiden_Rando\\libssl-3-x64.dll"
        );


        client = std::make_unique<APClient>(
            "Yakuza-Gaiden-Client",
            "Yakuza Gaiden",
            address
        );


        client->set_socket_connected_handler(
            [this]()
            {
                connectAttempts = 0;

                printf("/b[AP] Socket connected\n");

                if (!pendingSlot.empty())
                {
                    printf(
                        "[AP] Auto-connecting slot /g%s\n",
                        pendingSlot.c_str()
                    );

                    client->ConnectSlot(
                        pendingSlot,
                        pendingPassword,
                        3
                    );
                }
                else {
                    printf(
                        "Please connect to your AP Slot using: /bslot SlotName\n"
                    );
                }
            }
        );


        client->set_socket_error_handler(
            [this](const std::string& error)
            {
                connectAttempts++;

                printf(
                    "/r[AP ERROR] %s\n",
                    error.c_str()
                );


                printf(
                    "/r[AP] Connection attempt %d/%d failed\n",
                    connectAttempts,
                    MAX_CONNECT_ATTEMPTS
                );


                if (connectAttempts >= MAX_CONNECT_ATTEMPTS)
                {
                    printf(
                        "/r[AP] Giving up after %d attempts. \nPlease manually connect again with: \n/bconnect archipelago.gg:12345\n",
                        MAX_CONNECT_ATTEMPTS
                    );

                    connected = false;

                    // Don't destroy the client here.
                }
            }
        );


        client->set_socket_disconnected_handler(
            [this]()
            {
                connected = false;

                printf(
                    "/r[AP] Socket disconnected\n"
                );
            }
        );


        client->set_room_info_handler(
            [this]()
            {
                if (!automatedConnection)
                {
                    printf(
                        "[AP] Room info received \nPlease connect with: slot /bYourSlotName Password. \n/w(if you do not have a password, leave the last part empty)\n"
                    );
                }
                else
                {
                    printf("/b[AP] Room info received\n");
                }
            }
        );



        client->set_location_checked_handler(
            [this](const std::list<int64_t>& locations)
            {
                for (auto location : locations)
                {

                    state.MarkLocationAcknowledged(
                        location
                    );
                }

                state.Save();
            }
        );



        client->set_slot_connected_handler(
            [this](const nlohmann::json&)
            {
                connected = true;


                printf(
                    "/b[AP] Slot connected\n"
                );


                for (auto& location :
                    state.GetQueuedLocations())
                {
                    if (
                        location.queued &&
                        !location.acknowledged
                        )
                    {
                        client->LocationChecks(
                            {
                                location.location_id
                            }
                        );


                        state.MarkLocationSent(
                            location.location_id
                        );
                    }
                }




                printf("[AP] Checking queued received items...\n");


                if (inventoryReadyFlag)
                {
                    for (auto& item :
                        state.GetReceivedItems())
                    {
                        printf(
                            "[AP] Queuing queued item after reconnect: /b%s (%lld)\n",
                            g_APItemNames.GetName(item.item_id).c_str(),
                            item.item_id
                        );

                        QueueGameAction(
                            {
                                GameActionType::DeliverItem,
                                item.item_id,
                                item.index
                            }
                        );
                    }
                }
                else
                {
                    printf(
                        "/r[AP] Inventory not ready, /wleaving items queued\n"
                    );
                }


                state.Save();
            }
        );



        client->set_slot_refused_handler(
            [](const std::list<std::string>& reasons)
            {
                printf(
                    "/r[AP] Slot refused:\n"
                );


                for (const auto& reason : reasons)
                {
                    printf(
                        "  %s\n",
                        reason.c_str()
                    );
                }
            }
        );




        client->set_items_received_handler(
            [this](const auto& items)
            {
                for (const auto& item : items)
                {
                    state.QueueReceivedItem(
                        item.item,
                        item.location,
                        item.index
                    );



                    if (inventoryReadyFlag)
                    {



                        QueueGameAction(
                            {
                                GameActionType::DeliverItem,
                                item.item,
                                item.index
                            }
                        );
                    }
                }


                state.Save();
            }
        );


        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "/r[AP] Exception: %s\n",
            e.what()
        );

        return false;
    }
}



constexpr int AP_PROGRESS_ITEM = 6285;


int APClientManager::GetAPProgress()
{
    if (!inventoryReadyFlag)
        return 0;

    int progress =
        GetItemAmount(
            AP_PROGRESS_ITEM
        );

    return progress;
}


void APClientManager::SetAPProgress(
    int progress
)
{
    SetItemAmount(
        AP_PROGRESS_ITEM,
        progress
    );
}



bool APClientManager::ConnectSlot(
    const std::string& slot,
    const std::string& password
)
{
    if (!client)
    {
        printf(
            "/r[AP] No client exists\n"
        );

        return false;
    }


    pendingSlot = slot;
    pendingPassword = password;


    SaveAPConfig(
        lastHost,
        lastPort,
        pendingSlot,
        pendingPassword
    );


    try
    {
        client->ConnectSlot(
            slot,
            password,
            3
        );


        printf(
            "[AP] Slot connect request sent: /g%s\n",
            slot.c_str()
        );


        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Slot connection exception: /r%s\n",
            e.what()
        );

        return false;
    }
}




void APClientManager::APWorkerThread()
{
    printf(
        "/b[AP] AP worker thread started\n"
    );


    while (apWorkerRunning)
    {
        if (client)
        {
            if (
                !connected &&
                connectAttempts >= MAX_CONNECT_ATTEMPTS
                )
            {
                printf(
                    "/r[AP] Cleaning up failed client, Failed to connect with existing information, please make sure your /bmultiworld page is refreshed /wand try again.\n"
                );

                client.reset();
            }
            else
            {
                try
                {
                    client->poll();
                }
                catch (const std::exception& e)
                {
                    printf(
                        "/r[AP ERROR] Poll exception: %s\n",
                        e.what()
                    );
                }
                catch (...)
                {
                    printf(
                        "/r[AP ERROR] Unknown poll exception\n"
                    );
                }
            }
        }


        Sleep(16);
    }


    printf(
        "/r[AP] AP worker thread stopped\n"
    );
}




void APClientManager::QueueGameAction(
    const GameAction& action
)
{
    std::lock_guard<std::mutex> lock(
        gameActionMutex
    );

    if (action.type != GameActionType::DeliverItem)
    {
        gameActionQueue.push(action);
        return;
    }


    std::queue<GameAction> ordered;

    bool inserted = false;

    while (!gameActionQueue.empty())
    {
        GameAction current =
            gameActionQueue.front();

        gameActionQueue.pop();

        if (
            !inserted &&
            current.type == GameActionType::DeliverItem &&
            action.itemIndex < current.itemIndex
            )
        {
            ordered.push(action);
            inserted = true;
        }

        ordered.push(current);
    }

    if (!inserted)
        ordered.push(action);

    gameActionQueue.swap(ordered);
}



void APClientManager::DrainGameActions()
{
    std::queue<GameAction> actions;


    {
        std::lock_guard<std::mutex> lock(
            gameActionMutex
        );

        actions.swap(
            gameActionQueue
        );
    }


    constexpr int MAX_GAME_ACTIONS_PER_FRAME = 4;

    int processed = 0;

    while (!actions.empty() &&
        processed < MAX_GAME_ACTIONS_PER_FRAME)
    {
        ProcessGameAction(
            actions.front()
        );

        actions.pop();

        processed++;
    }


    while (!actions.empty())
    {
        QueueGameAction(
            actions.front()
        );

        actions.pop();
    }
}


void APClientManager::ProcessGameAction(
    const GameAction& action
)
{
    switch (action.type)
    {
    case GameActionType::DeliverItem:
    {

        int progress = GetAPProgress();

        if (action.itemIndex > progress)
        {
            printf(
                "[AP] Item gap detected: game expects index /g%d, incoming index /g%d\n",
                progress,
                action.itemIndex
            );

            for (
                int missingIndex = progress;
                missingIndex < action.itemIndex;
                missingIndex++
                )
            {
                for (auto& item :
                    state.GetReceivedItems())
                {
                    if (item.index == missingIndex)
                    {
                        printf(
                            "[AP] Replaying missing item index /g%d: %s (%lld)\n",
                            item.index,
                            g_APItemNames.GetName(
                                item.item_id
                            ).c_str(),
                            item.item_id
                        );


                        ReceiveItem(
                            item.item_id,
                            item.index
                        );

                        break;
                    }
                }
            }
        }


        ReceiveItem(
            action.itemID,
            action.itemIndex
        );

        break;
    }


    case GameActionType::TimelineEvent:
    {
        //printf(
            //"[Timeline] Category=%u Mission=%u State=%u p5=%u p6=%u\n",
            //action.category,
            //action.mission,
            //action.state,
            //action.p5,
            //action.p6
        //);

        ProcessTimelineEvent(
            action.category,
            action.mission,
            action.state
        );

        break;
    }
    }
}



void APClientManager::Update()
{
    DrainGameActions();
}




void APClientManager::CheckAPProgress()
{
    if (!inventoryReadyFlag)
        return;


    int gameProgress =
        GetAPProgress();


    int highestKnownIndex = -1;


    for (auto& item :
        state.GetReceivedItems())
    {
        if (item.index > highestKnownIndex)
        {
            highestKnownIndex =
                item.index;
        }
    }


    for (auto& item :
        state.GetReceivedItems())
    {

        if (item.index >= gameProgress)
        {
            printf(
                "[AP] Replaying missing item /g%s (%lld) (index %d)\n",
                g_APItemNames.GetName(item.item_id).c_str(),
                item.item_id,
                item.index
            );


            GiveAPItem(
                item.item_id
            );


            SetAPProgress(
                item.index + 1
            );
        }
    }
}



void APClientManager::ReceiveItem(
    int64_t item_id,
    int index
)
{
    int progress =
        GetAPProgress();


    if (index < progress)
    {

        return;
    }


    printf(
        "[AP] Giving /g%s (%lld)\n",
        g_APItemNames.GetName(item_id).c_str(),
        item_id
    );


    GiveAPItem(
        item_id
    );


    if (index + 1 > progress)
    {
        SetAPProgress(
            index + 1
        );
    }


    state.Save();
}



void APClientManager::InventoryReady()
{
    inventoryReadyFlag = true;


    for (auto& item :
        state.GetReceivedItems())
    {
        QueueGameAction(
            {
                GameActionType::DeliverItem,
                item.item_id,
                item.index
            }
        );
    }


    state.Save();
}



void APClientManager::Say(
    const std::string& message
)
{
    if (!client)
        return;


    client->Say(
        message
    );
}

bool APClientManager::AreWinconsComplete()
{
    if (
        g_APOptions.GetGoldenBallWincon() &&
        !state.IsWinconComplete(APWincon::GoldenBall)
        )
    {
        return false;
    }

    if (
        g_APOptions.GetDefeatShishidoWincon() &&
        !state.IsWinconComplete(APWincon::DefeatShishido)
        )
    {
        return false;
    }

    if (
        g_APOptions.GetDefeatPocketCircuitOwnerWincon() &&
        !state.IsWinconComplete(
            APWincon::DefeatPocketCircuitOwner
        )
        )
    {
        return false;
    }

    return true;
}

void APClientManager::SendGoal()
{
    if (!client)
    {
        printf(
            "/r[AP] Cannot send goal - no client\n"
        );

        return;
    }


    if (!connected)
    {
        printf(
            "/r[AP] Cannot send goal - not connected\n"
        );

        return;
    }


    if (goalSent)
    {
        printf(
            "/r[AP] Goal already sent\n"
        );

        return;
    }

    // Check all enabled win conditions.

    if (!AreWinconsComplete())
    {
        printf(
            "[AP] Goal conditions not yet complete\n"
        );

        return;
    }


    printf(
        "/y[AP] All enabled win conditions complete! Sending goal completion!\n"
    );


    client->StatusUpdate(
        APClient::ClientStatus::GOAL
    );


    goalSent = true;
}




void APClientManager::CheckLocation(
    int64_t location
)
{

    if (!g_APItems.HasLocation(location))
    {
        //printf(
        //    "[AP] Location %s (%lld) is not in item mapping - skipping check\n",
        //    g_APLocationNames.GetName(location).c_str(),
        //    location
        //);

        return;
    }

    printf(
        "[AP] Checking location /m%s (%lld)\n",
        g_APLocationNames.GetName(location).c_str(),
        location
    );

    // Normal AP location check.

    state.QueueLocation(
        location
    );

    state.Save();

    if (!client || !connected)
    {
        printf(
            "/r[AP] Not connected, /wlocation saved for later\n"
        );

        return;
    }

    client->LocationChecks(
        {
            location
        }
    );

    state.MarkLocationSent(
        location
    );

    state.Save();


    std::vector<int64_t> shopLocations =
        g_APLocationNames.GetShopLocationsFor(
            location
        );

    if (!shopLocations.empty())
    {

        for (int64_t shopLocation : shopLocations)
        {

            if (shopLocation == location)
                continue;


            if (state.HasLocationHinted(shopLocation))
            {
                continue;
            }

            //printf(
            //    "[AP] Hinting shop location: %s (%lld)\n",
            //    g_APLocationNames.GetName(shopLocation).c_str(),
            //    shopLocation
            //);

            client->CreateHints(
                {
                    shopLocation
                }
            );

            state.MarkLocationHinted(shopLocation);

            state.Save();
        }
    }
}



bool APClientManager::IsConnected()
{
    return connected;
}