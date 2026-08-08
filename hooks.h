#pragma once

#include <cstdint>
#include <string>

typedef uint64_t(*inventory_set_quantity_t)(
    uint64_t param1,
    int itemID,
    int amount,
    uint64_t inventoryPtr,
    int mode
    );

extern inventory_set_quantity_t original_inventory;

extern uint64_t g_inventoryPtr;
extern uint64_t g_param1;

int GetItemAmount(int itemID);

void SetItemAmount(
    int itemID,
    int amount
);

void GiveAPItem(
    int64_t itemID
);

bool IsInventoryReady();

bool HookInventory();

void DumpInventory();