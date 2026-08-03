#include "hooks.h"

#include "patternscan.h"
#include "MinHook.h"

#include <stdio.h>

#include "APItemDatabase.h"
#include "APClientManager.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>

constexpr int GOLD_BALL_ID = 6056;

constexpr int AP_PROGRESS_ID = 6285;

bool g_goalSent = false;

using json = nlohmann::json;


inventory_set_quantity_t original_inventory = nullptr;

uint64_t g_inventoryPtr = 0;
uint64_t g_param1 = 0;


// =====================================================
// Progressive item data
// =====================================================

struct ProgressiveItem
{
    std::string name;
    std::vector<int> levels;
};


std::unordered_map<int, ProgressiveItem> g_progressiveItems;


// =====================================================
// Inventory state
// =====================================================

bool IsInventoryReady()
{
    return g_inventoryPtr != 0;
}


// =====================================================
// Progressive item database
// =====================================================

bool LoadProgressiveItems(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        printf(
            "[AP] Failed to open progressive_items.json: %s\n",
            path.c_str()
        );

        return false;
    }


    try
    {
        json data;

        file >> data;

        g_progressiveItems.clear();


        for (auto& [idString, value] : data.items())
        {
            int progressiveID = std::stoi(idString);

            ProgressiveItem item;

            item.name = value.value(
                "name",
                ""
            );


            if (
                value.contains("levels")
                && value["levels"].is_array()
                )
            {
                for (const auto& levelID : value["levels"])
                {
                    item.levels.push_back(
                        levelID.get<int>()
                    );
                }
            }


            g_progressiveItems[progressiveID] = item;

        }



        return true;
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Failed to parse progressive_items.json: %s\n",
            e.what()
        );

        return false;
    }
}


// Forward declaration
void GiveProgressiveAPItem(int progressiveID);


// =====================================================
// Inventory helpers
// =====================================================

void DumpInventory()
{
    if (!IsInventoryReady())
    {
        printf(
            "[Inventory] Not initialized yet.\n"
        );

        return;
    }


    uint64_t entriesPtr =
        *(uint64_t*)g_inventoryPtr;

    int count =
        *(int*)(g_inventoryPtr + 0xC);


    printf(
        "\n========== INVENTORY DUMP ==========\n"
    );


    printf(
        "Inventory Ptr: 0x%llX\n",
        g_inventoryPtr
    );


    printf(
        "Entries Ptr:   0x%llX\n",
        entriesPtr
    );


    printf(
        "Count:         %d\n\n",
        count
    );


    for (int i = 0; i < count; i++)
    {
        uint64_t entry =
            entriesPtr + (i * 0x10);


        int itemID =
            *(int*)(entry + 0x0);

        int amount =
            *(int*)(entry + 0x4);

        int unknown =
            *(int*)(entry + 0x8);


        printf(
            "[%d] Item: %d | Amount: %d | Unknown: %d\n",
            i,
            itemID,
            amount,
            unknown
        );
    }


    printf(
        "====================================\n\n"
    );
}

int GetAPProgress()
{
    return GetItemAmount(AP_PROGRESS_ID);
}

void SetAPProgress(int nextIndex)
{
    SetItemAmount(AP_PROGRESS_ID, nextIndex);
}


int GetItemAmount(int itemID)
{
    if (!IsInventoryReady())
        return 0;


    uint64_t entriesPtr =
        *(uint64_t*)g_inventoryPtr;

    int count =
        *(int*)(g_inventoryPtr + 0xC);


    for (int i = 0; i < count; i++)
    {
        uint64_t entry =
            entriesPtr + (i * 0x10);


        int id =
            *(int*)(entry + 0x0);

        int amount =
            *(int*)(entry + 0x4);


        if (id == itemID)
        {
            return amount;
        }
    }


    return 0;
}


void SetItemAmount(
    int itemID,
    int amount
)
{
    if (!IsInventoryReady())
    {
        printf(
            "[Inventory] Cannot set item %d - inventory not ready\n",
            itemID
        );

        return;
    }


    if (!original_inventory)
    {
        printf(
            "[Inventory] Cannot set item %d - hook not ready\n",
            itemID
        );

        return;
    }


    printf(
        "[Inventory] Setting item %d to %d\n",
        itemID,
        amount
    );


    original_inventory(
        g_param1,
        itemID,
        amount,
        g_inventoryPtr,
        0
    );

    //
    // AP-only goal check
    //
    if (
        itemID == GOLD_BALL_ID &&
        !g_goalSent
        )
    {
        int count = GetItemAmount(GOLD_BALL_ID);

        printf(
            "[AP] Gold Ball count after injection: %d\n",
            count
        );


        if (count >= 7)
        {
            printf(
                "[AP] 7 Gold Balls collected! Sending goal!\n"
            );

            g_AP.SendGoal();

            g_goalSent = true;
        }
    }
}


// =====================================================
// AP item injection
// =====================================================

void GiveAPItem(int64_t itemID)
{
    if (!IsInventoryReady())
    {
        printf(
            "[AP] Cannot give item %lld - inventory not ready\n",
            itemID
        );

        return;
    }


    int id = (int)itemID;


    // -------------------------------------------------
    // Progressive AP item
    // -------------------------------------------------

    if (
        g_progressiveItems.find(id)
        != g_progressiveItems.end()
        )
    {
        GiveProgressiveAPItem(id);

        return;
    }


    // -------------------------------------------------
    // Normal AP item
    // -------------------------------------------------

    int current =
        GetItemAmount(id);


    printf(
        "[AP] Giving item %lld (%d -> %d)\n",
        itemID,
        current,
        current + 1
    );


    SetItemAmount(
        id,
        current + 1
    );
}


// =====================================================
// Progressive AP item injection
// =====================================================

void GiveProgressiveAPItem(int progressiveID)
{
    auto it =
        g_progressiveItems.find(progressiveID);


    if (
        it
        == g_progressiveItems.end()
        )
    {
        printf(
            "[AP] Progressive item %d not found in database\n",
            progressiveID
        );

        return;
    }


    const ProgressiveItem& item =
        it->second;


    // -------------------------------------------------
    // How many copies of the AP progressive item
    // have we already received?
    // -------------------------------------------------

    int current =
        GetItemAmount(progressiveID);


    int nextCount =
        current + 1;


    // -------------------------------------------------
    // Prevent going beyond the final level
    // -------------------------------------------------

    if (
        nextCount
    > (int)item.levels.size()
        )
    {
        printf(
            "[AP] %s already at max level (%d)\n",
            item.name.c_str(),
            (int)item.levels.size()
        );

        return;
    }


    // -------------------------------------------------
    // Select the corresponding actual game item
    //
    // First AP copy:
    // levels[0]
    //
    // Second AP copy:
    // levels[1]
    //
    // etc.
    // -------------------------------------------------

    int levelID =
        item.levels[nextCount - 1];


    printf(
        "[AP] Giving %s level %d -> game item %d\n",
        item.name.c_str(),
        nextCount,
        levelID
    );


    // -------------------------------------------------
    // Track how many progressive AP items we've
    // received.
    // -------------------------------------------------

    SetItemAmount(
        progressiveID,
        nextCount
    );


    // -------------------------------------------------
    // Give the corresponding actual skill book.
    //
    // We intentionally DO NOT remove previous books.
    // -------------------------------------------------

    SetItemAmount(
        levelID,
        1
    );
}


// =====================================================
// Inventory hook
// =====================================================

uint64_t hooked_inventory(
    uint64_t param1,
    int itemID,
    int amount,
    uint64_t inventoryPtr,
    int mode
)
{
    //
    // Ignore fake startup calls
    //
    if (
        g_inventoryPtr == 0
        && itemID >= 4000
        && itemID != 6023
        )
    {
        g_inventoryPtr =
            inventoryPtr;

        g_param1 =
            param1;


        printf(
            "\n[Inventory] Player inventory detected!\n"
            "Pointer : 0x%llX\n"
            "Param1  : 0x%llX\n"
            "Item ID : %d\n\n",
            g_inventoryPtr,
            g_param1,
            itemID
        );


        g_AP.InventoryReady();
    }



    //
    // Game -> AP location checks
    //
    if (amount > 0)
    {
        APItemEntry entry;


        if (
            g_APItems.GetItem(
                itemID,
                entry
            )
            )
        {
            printf(
                "[AP] Checking location %lld from item %d\n",
                entry.location_id,
                itemID
            );


            g_AP.CheckLocation(
                entry.location_id
            );
        }
    }

    if (itemID >= 100)
    {
        printf(
            "Gained Item ID: %d\n",
            itemID
        );
    }

    uint64_t result = original_inventory(
        param1,
        itemID,
        amount,
        inventoryPtr,
        mode
    );

    // Goal check after inventory has actually updated
    if (
        itemID == GOLD_BALL_ID &&
        !g_goalSent &&
        IsInventoryReady()
        )
    {
        int count = GetItemAmount(GOLD_BALL_ID);

        if (count >= 7)
        {
            printf("[AP] 7 Gold Balls collected! Sending goal!\n");

            g_AP.SendGoal();

            g_goalSent = true;
        }
    }

    return result;
}


// =====================================================
// Hook setup
// =====================================================

bool HookInventory()
{


    uintptr_t address =
        PatternScan(
            "48 89 5C 24 18 "
            "55 "
            "57 "
            "41 56 "
            "48 83 EC 20 "
            "41 8B 79 0C "
            "89 D5 "
            "49 8B 11 "
            "89 F9 "
            "48 C1 E1 04 "
            "4C 89 CB "
            "48 01 D1 "
            "45 89 C6 "
            "48 89 D0 "
            "48 39 CA "
            "74 ?"
        );


    if (!address)
    {
        printf(
            "[Inventory] Pattern not found\n"
        );

        return false;
    }


    MH_STATUS status =
        MH_CreateHook(
            (LPVOID)address,
            &hooked_inventory,
            (LPVOID*)&original_inventory
        );


    if (status != MH_OK)
    {
        printf(
            "[Inventory] CreateHook failed: %d\n",
            status
        );

        return false;
    }


    status =
        MH_EnableHook(
            (LPVOID)address
        );


    if (status != MH_OK)
    {
        printf(
            "[Inventory] EnableHook failed: %d\n",
            status
        );

        return false;
    }


    printf(
        "[Inventory] Inventory_SetQuantity hook enabled!\n"
    );


    return true;
}