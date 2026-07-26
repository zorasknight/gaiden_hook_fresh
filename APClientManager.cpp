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
            []()
            {
                printf("[AP] Socket disconnected\n");
            }
        );


        client->set_room_info_handler(
            []()
            {
                printf("[AP] Room info received\n");
            }
        );


        client->set_slot_connected_handler(
            [this](const nlohmann::json&)
            {
                connected = true;

                printf(
                    "[AP] Slot connected\n"
                );


                printf(
                    "[AP] Requesting received item history...\n"
                );

            }
        );


        client->set_slot_refused_handler(
            [](const std::list<std::string>& reasons)
            {
                printf("[AP] Slot refused:\n");

                for (const auto& reason : reasons)
                {
                    printf(
                        "  %s\n",
                        reason.c_str()
                    );
                }
            }
        );


        //
        // AP -> Game item handling
        //
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


                printf(
                    "========== AP ITEMS RECEIVED ==========\n"
                );


                for (const auto& item : items)
                {
                    printf(
                        "AP Item: %lld\n",
                        item.item
                    );

                    printf(
                        "Index: %d\n",
                        item.index
                    );

                    printf(
                        "Player: %d\n",
                        item.player
                    );

                    printf(
                        "Location: %lld\n",
                        item.location
                    );


                    //
                    // Save item locally
                    //
                    state.AddReceivedItem(
                        item.item,
                        item.location,
                        item.index
                    );


                    //
                    // Give item only once
                    //
                    if (inventoryReadyFlag &&
                        !state.IsItemReceived(item.index))
                    {
                        printf(
                            "[AP] Giving item immediately: %lld\n",
                            item.item
                        );


                        ReceiveItem(
                            item.item
                        );


                        state.MarkItemReceived(
                            item.index
                        );
                    }
                    else
                    {
                        printf(
                            "[AP] Inventory not ready or item already received, queued\n"
                        );
                    }


                    printf(
                        "--------------------------------\n"
                    );
                }


                printf(
                    "=======================================\n\n"
                );


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
        printf("[AP] No client exists\n");
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


    for (auto& item : state.GetReceivedItems())
    {
        if (!item.received)
        {
            printf(
                "[AP] Giving queued item: %lld\n",
                item.item_id
            );


            ReceiveItem(
                item.item_id
            );


            item.received = true;
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
    if (!client)
        return;


    printf(
        "[AP] Checking location %lld\n",
        location
    );


    //
    // Save locally first
    //
    state.AddCheckedLocation(location);
    state.Save();


    //
    // Send to AP server
    //
    client->LocationChecks(
        { location }
    );
}



bool APClientManager::IsConnected()
{
    return connected;
}