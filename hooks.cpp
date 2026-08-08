#include "hooks.h"

#include "patternscan.h"
#include "MinHook.h"
#include "APItemNames.h"
#include <stdio.h>
#include "APProgressiveItems.h"
#include "APItemDatabase.h"
#include "APClientManager.h"
#include "APOptions.h"
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
// Inventory state
// =====================================================

bool IsInventoryReady()
{
    return g_inventoryPtr != 0;
}



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

void GiveProgressiveAPItem(int progressiveID);

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


        int required =
            g_APOptions.GetRequiredGoldenBallCount();

        if (count >= required)
        {
            printf(
                "[AP] %d Gold Balls collected! Sending goal!\n",
                required
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

    int id = static_cast<int>(itemID);

    const ProgressiveItem* progressive =
        g_APProgressiveItems.Get(id);

    if (progressive)
    {
        GiveProgressiveAPItem(id);
        return;
    }

    int current =
        GetItemAmount(id);

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
    const ProgressiveItem* item =
        g_APProgressiveItems.Get(progressiveID);

    if (!item)
    {
        printf(
            "[AP] Progressive item %d not found in database\n",
            progressiveID
        );

        return;
    }

    int current =
        GetItemAmount(progressiveID);

    int nextCount =
        current + 1;

    if (nextCount > static_cast<int>(item->levels.size()))
    {
        printf(
            "[AP] %s already at max level (%d)\n",
            item->name.c_str(),
            static_cast<int>(item->levels.size())
        );

        return;
    }

    int levelID =
        item->levels[nextCount - 1];

    printf("[AP] Giving %s level %d -> %s (%d)\n", item->name.c_str(), nextCount, g_APItemNames.GetName(levelID).c_str(), levelID);

    SetItemAmount(
        progressiveID,
        nextCount
    );

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


            g_AP.CheckLocation(
                entry.location_id
            );
        }
    }

    uint64_t result = original_inventory(
        param1,
        itemID,
        amount,
        inventoryPtr,
        mode
    );

    // =====================================================
    // Progressive location check
    //
    // The original inventory function has now completed,
    // so the inventory contains the newly acquired item.
    // The current total determines which progressive
    // location check this acquisition represents.
    // =====================================================

    if (amount > 0)
    {
        const ProgressiveItem* progressive =
            g_APProgressiveItems.Get(itemID);

        if (
            progressive &&
            !progressive->checks.empty()
            )
        {
            int currentAmount =
                GetItemAmount(itemID);

            int checkIndex =
                currentAmount - 1;

            if (
                checkIndex >= 0 &&
                checkIndex <
                static_cast<int>(progressive->checks.size())
                )
            {
                int64_t location =
                    progressive->checks[checkIndex];

                g_AP.CheckLocation(location);
            }
            else
            {
                printf(
                    "[AP] Progressive %s acquired #%d, but no check exists for this count\n",
                    progressive->name.c_str(),
                    currentAmount
                );
            }
        }
    }

    // Goal check after inventory has actually updated
    if (
        itemID == GOLD_BALL_ID &&
        !g_goalSent &&
        IsInventoryReady()
        )
    {
        int count = GetItemAmount(GOLD_BALL_ID);

        int required =
            g_APOptions.GetRequiredGoldenBallCount();

        if (count >= required)
        {
            printf(
                "[AP] %d Gold Balls collected! Sending goal!\n",
                required
            );

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