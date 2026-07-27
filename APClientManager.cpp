#include "APClientManager.h"

#include "APItemDatabase.h"
#include "hooks.h"

#include <cstdio>
#include <climits>

APClientManager g_AP;


bool APClientManager::Initialize(
    const std::string& statePath
)
{
    state.SetFilename(statePath);

    state.Load();

    initialized = true;

    printf("[AP] Manager initialized\n");

    return true;
}



bool APClientManager::Connect(
    const std::string& host,
    int port
)
{
    try
    {
        std::string address =
            host + ":" + std::to_string(port);


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
            []()
            {
                printf("[AP] Socket connected\n");
            }
        );


        client->set_socket_error_handler(
            [](const std::string& error)
            {
                printf(
                    "[AP ERROR] %s\n",
                    error.c_str()
                );
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
                printf(
                    "[AP] Location confirmation received\n"
                );


                for (auto location : locations)
                {
                    printf(
                        "[AP] Location acknowledged: %lld\n",
                        location
                    );


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
                printf(
                    "[AP] Flushing pending location checks...\n"
                );


                for (auto& location :
                    state.GetQueuedLocations())
                {
                    if (location.queued &&
                        !location.acknowledged)
                    {
                        printf(
                            "[AP] Resending location check: %lld\n",
                            location.location_id
                        );


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


                printf(
                    "[AP] Connection flush complete\n"
                );
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
                printf(
                    "\n[AP] ReceivedItems callback fired!\n"
                );


                printf(
                    "Count: %zu\n",
                    items.size()
                );


                for (const auto& item : items)
                {
                    printf(
                        "[AP] Item %lld Index %d Location %lld\n",
                        item.item,
                        item.index,
                        item.location
                    );


                    state.QueueReceivedItem(
                        item.item,
                        item.location,
                        item.index
                    );


                    if (inventoryReadyFlag &&
                        !state.IsItemDelivered(item.index))
                    {
                        printf(
                            "[AP] Delivering item immediately: %lld\n",
                            item.item
                        );


                        ReceiveItem(
                            item.item
                        );


                        state.MarkItemDelivered(
                            item.index
                        );
                    }
                    else
                    {
                        printf(
                            "[AP] Item queued\n"
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
    if (client)
    {
        client->poll();
    }
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


    printf(
        "[AP] Inventory ready, checking queued items\n"
    );


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



    printf(
        "[AP] Sending location check immediately\n"
    );


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