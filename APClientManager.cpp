#include "APClientManager.h"

#include "APItemDatabase.h"
#include "hooks.h"

#include <cstdio>
#include <climits>

#include <fstream>
#include <nlohmann/json.hpp>

APClientManager g_AP;
static const char* AP_CONFIG =
"mods/Gaiden_Rando/APConfig.json";



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


bool APClientManager::Initialize(
    const std::string& statePath
)
{
    state.SetFilename(statePath);

    state.Load();

    initialized = true;

    printf("[AP] Manager initialized\n");

    std::string host;
    std::string slot;
    std::string password;
    int port = 0;


    if (LoadAPConfig(
        host,
        port,
        slot,
        password))
    {
        printf(
            "[AP] Previous connection found: %s:%d\n",
            host.c_str(),
            port
        );
        
        pendingSlot = slot;
        pendingPassword = password;

        Connect(host, port);


    }

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

        printf("[AP] Connection saved\n");

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
        printf("[AP] Closing previous client\n");
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
            "[AP] Connecting to %s\n",
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

                printf("[AP] Socket connected\n");

                if (!pendingSlot.empty())
                {
                    printf("[AP] Auto-connecting slot %s\n",
                        pendingSlot.c_str());

                    client->ConnectSlot(
                        pendingSlot,
                        pendingPassword,
                        3
                    );
                }
            });


        client->set_socket_error_handler(
            [this](const std::string& error)
            {
                connectAttempts++;

                printf(
                    "[AP ERROR] %s\n",
                    error.c_str()
                );


                printf(
                    "[AP] Connection attempt %d/%d failed\n",
                    connectAttempts,
                    MAX_CONNECT_ATTEMPTS
                );


                if (connectAttempts >= MAX_CONNECT_ATTEMPTS)
                {
                    printf(
                        "[AP] Giving up after %d attempts.\n",
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
                    "[AP] Socket disconnected\n"
                );
            }
        );


        client->set_room_info_handler(
            []()
            {
                printf(
                    "[AP] Room info received\n"
                );
            }
        );


        //
        // Server confirmed location checks
        //
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
                    "[AP] Slot connected\n"
                );


                //
                // Flush pending location checks
                //


                for (auto& location :
                    state.GetQueuedLocations())
                {
                    if (location.queued &&
                        !location.acknowledged)
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



                //
                // Retry undelivered AP items
                //
                printf(
                    "[AP] Checking queued received items...\n"
                );


                if (inventoryReadyFlag)
                {
                    for (auto& item :
                        state.GetReceivedItems())
                    {
                        if (item.queued &&
                            !item.delivered)
                        {
                            printf(
                                "[AP] Delivering queued item after reconnect: %lld\n",
                                item.item_id
                            );


                            ReceiveItem(
                                item.item_id
                            );


                            state.MarkItemDelivered(
                                item.index
                            );
                        }
                    }
                }
                else
                {
                    printf(
                        "[AP] Inventory not ready, leaving items queued\n"
                    );
                }



                state.Save();

            }
        );



        client->set_slot_refused_handler(
            [](const std::list<std::string>& reasons)
            {
                printf(
                    "[AP] Slot refused:\n"
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


                    if (inventoryReadyFlag &&
                        !state.IsItemDelivered(item.index))
                    {


                        ReceiveItem(
                            item.item
                        );


                        state.MarkItemDelivered(
                            item.index
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
            "[AP] Exception: %s\n",
            e.what()
        );

        return false;
    }
}



bool APClientManager::ConnectSlot(
    const std::string& slot,
    const std::string& password
)
{
    if (!client)
    {
        printf(
            "[AP] No client exists\n"
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
            "[AP] Slot connect request sent: %s\n",
            slot.c_str()
        );


        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Slot connection exception: %s\n",
            e.what()
        );

        return false;
    }
}



void APClientManager::Update()
{
    if (!client)
        return;

    if (!connected &&
        connectAttempts >= MAX_CONNECT_ATTEMPTS)
    {
        printf("[AP] Cleaning up failed client\n");
        client.reset();
        return;
    }

    client->poll();
}



void APClientManager::ReceiveItem(
    int64_t item_id
)
{
    printf(
        "[AP] Injecting item: %lld\n",
        item_id
    );


    GiveAPItem(
        item_id
    );
}



void APClientManager::InventoryReady()
{
    inventoryReadyFlag = true;


    for (auto& item :
        state.GetReceivedItems())
    {
        if (item.queued &&
            !item.delivered)
        {
            printf(
                "[AP] Giving queued item: %lld\n",
                item.item_id
            );


            ReceiveItem(
                item.item_id
            );


            state.MarkItemDelivered(
                item.index
            );
        }
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

void APClientManager::SendGoal()
{
    if (!client)
    {
        printf(
            "[AP] Cannot send goal - no client\n"
        );

        return;
    }


    if (!connected)
    {
        printf(
            "[AP] Cannot send goal - not connected\n"
        );

        return;
    }


    if (goalSent)
    {
        printf(
            "[AP] Goal already sent\n"
        );

        return;
    }


    printf(
        "[AP] Sending goal completion!\n"
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
    printf(
        "[AP] Queueing location check: %lld\n",
        location
    );


    //
    // Always save locally first
    //
    state.QueueLocation(
        location
    );


    state.Save();



    if (!client || !connected)
    {
        printf(
            "[AP] Not connected, location saved for later\n"
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
}



bool APClientManager::IsConnected()
{
    return connected;
}