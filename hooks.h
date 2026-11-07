#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>

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

void SetTimelineState(
    uint32_t category,
    uint32_t mission,
    uint32_t entry,
    uint32_t state
);

void GiveAPItem(
    int64_t itemID
);

bool IsInventoryReady();

bool HookInventory();

void DumpInventory();

using find_task_object_t =
uint64_t(*)(uint64_t param1, uint32_t* param2);

extern find_task_object_t original_find_task_object;

bool HookTaskTable();

using timeline_lookup_t = uint64_t(*)(
    uint64_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4,
    uint8_t param5,
    int8_t param6
    );

extern timeline_lookup_t original_timeline_lookup;

extern uint64_t g_timelineObject;
extern uint64_t g_timelineArray;
extern uint32_t g_timelineCount;
extern volatile LONG g_timelineReady;

uint64_t hooked_timeline_lookup(
    uint64_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4,
    uint8_t param5,
    int8_t param6
);

bool GetTimelineState(
    uint32_t category,
    uint32_t mission,
    uint32_t entry,
    uint8_t& rawByte
);

bool HookTimeline();

void MonitorTimeline();