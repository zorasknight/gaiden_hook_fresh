#include "hooks.h"

#include "patternscan.h"
#include "MinHook.h"


#include <stdio.h>

#include "APItemDatabase.h"
#include "APClientManager.h"


inventory_set_quantity_t original_inventory = nullptr;
uint64_t g_inventoryPtr = 0;
uint64_t g_param1 = 0;

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
        printf("[Inventory] Not initialized yet.\n");
        return;
    }


    uint64_t entriesPtr = *(uint64_t*)g_inventoryPtr;
    int count = *(int*)(g_inventoryPtr + 0xC);


    printf("\n========== INVENTORY DUMP ==========\n");
    printf("Inventory Ptr: 0x%llX\n", g_inventoryPtr);
    printf("Entries Ptr:   0x%llX\n", entriesPtr);
    printf("Count:         %d\n\n", count);


    for (int i = 0; i < count; i++)
    {
        uint64_t entry = entriesPtr + (i * 0x10);


        int itemID = *(int*)(entry + 0x0);
        int amount = *(int*)(entry + 0x4);
        int unknown = *(int*)(entry + 0x8);


        printf(
            "[%d] Item: %d | Amount: %d | Unknown: %d\n",
            i,
            itemID,
            amount,
            unknown
        );
    }


    printf("====================================\n\n");
}



int GetItemAmount(int itemID)
{
    if (!IsInventoryReady())
        return 0;


    uint64_t entriesPtr = *(uint64_t*)g_inventoryPtr;
    int count = *(int*)(g_inventoryPtr + 0xC);


    for (int i = 0; i < count; i++)
    {
        uint64_t entry = entriesPtr + (i * 0x10);


        int id = *(int*)(entry + 0x0);
        int amount = *(int*)(entry + 0x4);


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


    int current =
        GetItemAmount((int)itemID);


    printf(
        "[AP] Giving item %lld (%d -> %d)\n",
        itemID,
        current,
        current + 1
    );


    SetItemAmount(
        (int)itemID,
        current + 1
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
    if (g_inventoryPtr == 0 && itemID >= 4000 && itemID != 6023)
    {
        g_inventoryPtr = inventoryPtr;
        g_param1 = param1;


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


        if (g_APItems.GetItem(itemID, entry))
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



    printf(
        "\n========== Inventory_SetQuantity ==========\n"
    );


    printf(
        "Item ID: %d\n",
        itemID
    );


    printf(
        "Amount: %d\n",
        amount
    );


    printf(
        "Mode: %d\n",
        mode
    );


    printf(
        "Param1: 0x%llX\n",
        param1
    );


    printf(
        "Inventory Ptr: 0x%llX\n",
        inventoryPtr
    );


    printf(
        "==========================================\n"
    );



    return original_inventory(
        param1,
        itemID,
        amount,
        inventoryPtr,
        mode
    );
}

// =====================================================
// Hook setup
// =====================================================


bool HookInventory()
{
    printf(
        "[Inventory] Searching for Inventory_SetQuantity...\n"
    );


    uintptr_t address = PatternScan(
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


    printf(
        "[Inventory] Found at %p\n",
        (void*)address
    );



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