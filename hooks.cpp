#include "hooks.h"

#include "patternscan.h"
#include "MinHook.h"
#include "APItemNames.h"
#include <stdio.h>
#include "APProgressiveItems.h"
#include "APItemDatabase.h"
#include "APClientManager.h"
#include "APOptions.h"
#include <array>
#include <unordered_map>
#include "Console.h"
#include <vector>
#include <cstring>
#include <Windows.h>

constexpr int DRAG_ON_ID = 4760;
constexpr int GOLEM_TIGER_ID = 4761;
constexpr int COOL_STRIKER_ID = 4762;
constexpr int DEVIL_KILLER_ID = 4763;
constexpr int KILLER_BEE_ID = 4764;
constexpr int DRAG_ON_ULTRA_ID = 6003;
constexpr int GOLEM_JAGUAR_ID = 6004;
constexpr int ALPHA_COOL_STRIKER_ID = 6005;
constexpr int SUPER_DEVIL_KILLER_ID = 6006;
constexpr int BANGIN_KILLER_BEE_ID = 6007;

static volatile LONG g_timelineWritePending = 0;

static uint32_t g_writeCategory = 0;
static uint32_t g_writeMission = 0;
static uint32_t g_writeEntry = 0;
static uint8_t g_writeState = 0;

//Maybe grab these values from the locations.json?
const std::unordered_map<int, std::array<int64_t, 5>> CAR_LOCATIONS = {
    {
        DRAG_ON_ID,
        { 1246, 1247, 1248, 1249, 1250 }
    },
    {
        GOLEM_TIGER_ID,
        { 1251, 1252, 1253, 1254, 1255 }
    },
    {
        COOL_STRIKER_ID,
        { 1256, 1257, 1258, 1259, 1260 }
    },
    {
        DEVIL_KILLER_ID,
        { 1261, 1262, 1263, 1264, 1265 }
    },
    {
        KILLER_BEE_ID,
        { 1266, 1267, 1268, 1269, 1270 }
    },
    {
        DRAG_ON_ULTRA_ID,
        { 1271, 1272, 1273, 1274, 1275 }
    },
    {
        GOLEM_JAGUAR_ID,
        { 1276, 1277, 1278, 1279, 1280 }
    },
    {
        ALPHA_COOL_STRIKER_ID,
        { 1281, 1282, 1283, 1284, 1285 }
    },
    {
        SUPER_DEVIL_KILLER_ID,
        { 1286, 1287, 1288, 1289, 1290 }
    },
    {
        BANGIN_KILLER_BEE_ID,
        { 1291, 1292, 1293, 1294, 1295 }
    },
};

constexpr int GOLD_BALL_ID = 6056;

constexpr int AP_PROGRESS_ID = 6285;

using json = nlohmann::json;

using potentially_set_tasks_t =
uint64_t(*)(uint64_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t);

potentially_set_tasks_t original_potentially_set_tasks = nullptr;

inventory_set_quantity_t original_inventory = nullptr;

uint64_t g_inventoryPtr = 0;
uint64_t g_param1 = 0;
uint64_t g_timelineParam1 = 0;

timeline_lookup_t original_timeline_lookup = nullptr;

uint64_t g_timelineObject = 0;
uint64_t g_timelineArray = 0;
uint32_t g_timelineCount = 0;
volatile LONG g_timelineReady = 0;

struct TaskState
{
    uint32_t p4;
    uint8_t p5;
    uint8_t p6;
    bool valid;
};

TaskState g_taskStates[16] = {};

using timeline_tree_find_t = uint64_t(*)(
    uint64_t* treeHandle,
    uint32_t* key
);

uint64_t hooked_timeline_lookup(
    uint64_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4,
    uint8_t param5,
    int8_t param6
)
{
    // Only capture once, and only if the object exists.
    if (InterlockedCompareExchange(&g_timelineReady, 0, 0) == 0 && param1)
    {
        uint64_t array =
            *reinterpret_cast<uint64_t*>(param1 + 0xC0);

        uint32_t count =
            *reinterpret_cast<uint32_t*>(param1 + 0xCC);

        if (array && count > 0 && count < 100000)
        {
            // Claim the capture so another thread cannot publish simultaneously.
            if (InterlockedCompareExchange(&g_timelineReady, -1, 0) == 0)
            {
                g_timelineObject = param1;
                g_timelineArray = array;
                g_timelineCount = count;

                // Publish the captured data.
                InterlockedExchange(&g_timelineReady, 1);
            }
        }
    }

    if (param2 != 4 && param2 != 0)
    {
        printf(
            "/b[Timeline] Lookup:/g %u / %u / %u /w| /bState value:/g %u\n",
            param2,
            param3,
            param4,
            param5
        );
    }

    // Normal game lookup.
    uint64_t result = original_timeline_lookup(
        param1,
        param2,
        param3,
        param4,
        param5,
        param6
    );

    // Process a queued write on the game thread.
    if (InterlockedCompareExchange(
        &g_timelineWritePending, 0, 1
    ) == 1)
    {
        uint32_t category = g_writeCategory;
        uint32_t mission = g_writeMission;
        uint32_t entry = g_writeEntry;
        uint8_t state = g_writeState;

        uint64_t object = g_timelineObject;

        if (object &&
            InterlockedCompareExchange(&g_timelineReady, 0, 0) == 1)
        {
            printf(
                "/b[Timeline] Writing: %u / %u / %u = %u\n",
                category,
                mission,
                entry,
                state
            );

            uint64_t writeResult = original_timeline_lookup(
                object,
                category,
                mission,
                entry,
                state,
                1
            );

            printf(
                "/b[Timeline] Write result: /g%llu\n",
                static_cast<unsigned long long>(writeResult)
            );
        }
        else
        {
            printf("/r[Timeline] Write failed: timeline object unavailable!\n");
        }

        InterlockedExchange(&g_timelineWritePending, 0);
    }

    return result;
}

bool HookTimeline()
{
    uintptr_t address = PatternScan(
        "48 89 5C 24 08 "
        "48 89 74 24 10 "
        "48 89 7C 24 20 "
        "44 89 44 24 18 "
        "41 56 "
        "48 83 EC 30 "
        "44 8B 91 CC 00 00 00"
    );

    if (!address)
    {
        printf("/r[Timeline] Failed to find timeline function!\n");
        return false;
    }

    printf("/b[Timeline] Found timeline function at: %p\n", (void*)address);

    if (MH_CreateHook(
        (LPVOID)address,
        &hooked_timeline_lookup,
        (LPVOID*)&original_timeline_lookup
    ) != MH_OK)
    {
        printf("/r[Timeline] Failed to create hook!\n");
        return false;
    }

    if (MH_EnableHook((LPVOID)address) != MH_OK)
    {
        printf("/r[Timeline] Failed to enable hook!\n");
        MH_RemoveHook((LPVOID)address);
        return false;
    }

    printf("/b[Timeline] Hook installed successfully!\n");

    return true;
}

void SetTimelineState(
    uint32_t category,
    uint32_t mission,
    uint32_t entry,
    uint32_t state
)
{
    if (InterlockedCompareExchange(&g_timelineReady, 0, 0) != 1)
    {
        printf("/r[Timeline] Timeline data not ready!\n");
        return;
    }

    if (state > 1)
    {
        printf("/r[Timeline] State must be 0 or 1!\n");
        return;
    }

    // Only allow one pending write at a time.
    if (InterlockedCompareExchange(&g_timelineWritePending, -1, 0) != 0)
    {
        printf("/r[Timeline] Another write is already pending!\n");
        return;
    }

    g_writeCategory = category;
    g_writeMission = mission;
    g_writeEntry = entry;
    g_writeState = static_cast<uint8_t>(state);

    InterlockedExchange(&g_timelineWritePending, 1);

    printf(
        "/b[Timeline] Queued write: /g%u / %u / %u = %u\n",
        category,
        mission,
        entry,
        state
    );
}

bool GetTimelineState(
    uint32_t category,
    uint32_t mission,
    uint32_t entry,
    uint8_t& rawByte
)
{
    if (InterlockedCompareExchange(&g_timelineReady, 0, 0) != 1)
        return false;

    uint64_t object = g_timelineObject;

    if (!object)
        return false;

    uint32_t count = *reinterpret_cast<uint32_t*>(object + 0xCC);
    uint64_t array = *reinterpret_cast<uint64_t*>(object + 0xC0);

    if (!array || !count || count > 100000)
        return false;

    // Find the category in the outer array.
    uint32_t left = 0;
    uint32_t right = count;

    uint64_t treeHandle = 0;
    bool foundCategory = false;

    while (left < right)
    {
        uint32_t mid = left + (right - left) / 2;

        uint64_t record = array + (uint64_t)mid * 0x10;
        uint32_t currentCategory =
            *reinterpret_cast<uint32_t*>(record);

        if (category < currentCategory)
        {
            right = mid;
        }
        else if (category > currentCategory)
        {
            left = mid + 1;
        }
        else
        {
            treeHandle = record + 8;
            foundCategory = true;
            break;
        }
    }

    if (!foundCategory)
        return false;

    // Find the mission node using the game's tree lookup.
    static timeline_tree_find_t treeFind =
        reinterpret_cast<timeline_tree_find_t>(
            GetModuleHandleA(nullptr)
            ? reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + 0x864480
            : 0
            );

    if (!treeFind)
        return false;

    uint64_t node = treeFind(
        reinterpret_cast<uint64_t*>(treeHandle),
        &mission
    );

    printf(
        "[Timeline Debug] Mission node: %p | Category: %u | Mission: %u\n",
        reinterpret_cast<void*>(node),
        category,
        mission
    );

    if (!node)
        return false;

    printf(
        "[Timeline Debug] Entries pointer: %p | Entry count: %u\n",
        reinterpret_cast<void*>(
            *reinterpret_cast<uint64_t*>(node + 0x20)
            ),
        *reinterpret_cast<uint32_t*>(node + 0x2C)
    );

    uint64_t entries =
        *reinterpret_cast<uint64_t*>(node + 0x20);

    uint32_t entryCount =
        *reinterpret_cast<uint32_t*>(node + 0x2C);

    if (!entries || entryCount > 100000)
        return false;

    for (uint32_t i = 0; i < entryCount; i++)
    {
        uint64_t currentEntry = entries + (uint64_t)i * 0x18;

        uint32_t values[6];

        memcpy(values, reinterpret_cast<void*>(currentEntry), sizeof(values));

        printf(
            "[Timeline Debug] Entry %u: %u / %u / %u / %u / %u / %u | Raw byte: 0x%02X\n",
            i,
            values[0],
            values[1],
            values[2],
            values[3],
            values[4],
            values[5],
            *reinterpret_cast<uint8_t*>(currentEntry + 4)
        );
    }

    // Each inner entry is 0x18 bytes (six uint32 values).
    for (uint32_t i = 0; i < entryCount; i++)
    {
        uint64_t currentEntry = entries + (uint64_t)i * 0x18;

        uint32_t currentEntryID =
            *reinterpret_cast<uint32_t*>(currentEntry + 0x10);

        if (currentEntryID == entry)
        {
            rawByte =
                *reinterpret_cast<uint8_t*>(currentEntry + 0x04);

            return true;
        }
    }

    return false;
}

void MonitorTimeline()
{
    static DWORD lastCheck = 0;

    DWORD now = GetTickCount();
    if (now - lastCheck < 100)
        return;

    lastCheck = now;

    if (InterlockedCompareExchange(&g_timelineReady, 0, 0) != 1)
        return;

    static uint64_t previousArray = 0;
    static uint32_t previousCount = 0;
    static std::vector<uint8_t> previousSnapshot;
    static bool initialized = false;

    uint64_t array = g_timelineArray;
    uint32_t count = g_timelineCount;

    if (!array || count == 0 || count >= 100000)
        return;

    constexpr size_t RECORD_SIZE = 0x10;
    size_t totalSize = static_cast<size_t>(count) * RECORD_SIZE;

    std::vector<uint8_t> currentSnapshot(totalSize);

    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(
        GetCurrentProcess(),
        reinterpret_cast<LPCVOID>(array),
        currentSnapshot.data(),
        totalSize,
        &bytesRead
    ) || bytesRead != totalSize)
    {
        return;
    }

    if (!initialized ||
        previousArray != array ||
        previousCount != count)
    {
        previousSnapshot = currentSnapshot;
        previousArray = array;
        previousCount = count;
        initialized = true;

        printf("/b[Timeline] Monitoring started. Records: %u\n", count);
        return;
    }

    previousSnapshot = currentSnapshot;
}


uint64_t hooked_potentially_set_tasks(
    uint64_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4,
    uint8_t param5,
    uint8_t param6
)
{
    g_timelineParam1 = param1;

    uint64_t result =
        original_potentially_set_tasks(
            param1,
            param2,
            param3,
            param4,
            param5,
            param6
        );

    g_AP.QueueGameAction(
        {
            GameActionType::TimelineEvent,
            0,
            -1,
            param2,
            param3,
            param4,
            param5,
            param6
        }
    );

    return result;
}

void SetTimelineState(
    uint32_t category,
    uint32_t mission,
    uint32_t state
)
{
    if (!original_potentially_set_tasks)
    {
        printf(
            "/r[Timeline] Cannot set state - original function not ready\n"
        );

        return;
    }

    if (!g_timelineParam1)
    {
        printf(
            "/r[Timeline] Cannot set state - timeline parameter not captured\n"
        );

        return;
    }

    original_potentially_set_tasks(
        g_timelineParam1,
        category,
        mission,
        state,
        0,
        0
    );

    printf(
        "/b[Timeline] Set /g%u/%u/%u\n",
        category,
        mission,
        state
    );
}

bool HookTaskTable()
{
    uintptr_t address =
        PatternScan(
            "48 89 5C 24 08 "
            "48 89 6C 24 10 "
            "48 89 74 24 20 "
            "44 89 44 24 18 "
            "57 "
            "41 56 "
            "41 57 "
            "48 83 EC 30 "
            "44 8B 91 CC 00 00 00 "
            "31 DB "
            "44 89 CD "
            "45 89 C7 "
            "89 D7 "
            "49 89 CE "
            "41 89 DB "
            "45 85 D2 "
            "7E ?"
        );

    if (!address)
    {
        printf(
            "/r[Timeline] Timeline pattern not found\n"
        );

        return false;
    }

    //printf(
        //"[Timeline] Timeline pattern resolved to /g0x%llX\n",
        //address
    //);

    MH_STATUS status =
        MH_CreateHook(
            (LPVOID)address,
            &hooked_potentially_set_tasks,
            (LPVOID*)&original_potentially_set_tasks
        );

    if (status != MH_OK)
    {
        printf(
            "/r[Timeline] CreateHook failed: %d\n",
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
            "/r[Timeline] EnableHook failed: %d\n",
            status
        );

        return false;
    }

    //printf(
    //    "[Timeline] Timeline hook enabled at /g0x%llX\n",
    //    address
    //);

    return true;
}


// Inventory state

bool IsInventoryReady()
{
    return g_inventoryPtr != 0;
}




// Inventory helpers

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
            "[Inventory] Cannot set item /g%d - /rinventory not ready\n",
            itemID
        );

        return;
    }


    if (!original_inventory)
    {
        printf(
            "[Inventory] Cannot set item /g%d - /rhook not ready\n",
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


    // Car item location checks
    // Each car item checks its five associated locations.

    if (g_APOptions.GetPocketCircuit()){
        if (amount > 0)
        {
            auto it = CAR_LOCATIONS.find(itemID);

            if (it != CAR_LOCATIONS.end())
            {
                printf(
                    "[AP] Car /g%d /wacquired - checking /g5 /wcar locations\n",
                    itemID
                );

                for (int64_t locationID : it->second)
                {
                    g_AP.CheckLocation(locationID);
                }
            }
        }
    }
    // AP-only goal check
    // Golden Ball wincon check
    if (itemID == GOLD_BALL_ID)
    {
        int count =
            GetItemAmount(GOLD_BALL_ID);

        int required =
            g_APOptions.GetRequiredGoldenBallCount();

        printf(
            "[AP] Gold Ball count after addition: /g%d/%d\n",
            count,
            required
        );

        if (count >= required)
        {
            printf(
                "/y[AP] Golden Ball wincon reached!\n"
            );

            g_AP.MarkWinconComplete(
                APWincon::GoldenBall
            );
        }
    }
}


// AP item injection

void GiveAPItem(int64_t itemID)
{
    if (!IsInventoryReady())
    {
        printf(
            "[AP] Cannot give item /g%lld - /rinventory not ready\n",
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


// Progressive AP item injection

void GiveProgressiveAPItem(int progressiveID)
{
    const ProgressiveItem* item =
        g_APProgressiveItems.Get(progressiveID);

    if (!item)
    {
        printf(
            "[AP] Progressive item /g%d /rnot found in database\n",
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
            "[AP] /g%s /walready at max level /g(%d)\n",
            item->name.c_str(),
            static_cast<int>(item->levels.size())
        );

        return;
    }

    int levelID =
        item->levels[nextCount - 1];

    printf("[AP] Giving /g%s /wlevel /g%d -> %s (%d)\n", item->name.c_str(), nextCount, g_APItemNames.GetName(levelID).c_str(), levelID);

    SetItemAmount(
        progressiveID,
        nextCount
    );

    SetItemAmount(
        levelID,
        1
    );
}


// Inventory hook

uint64_t hooked_inventory(
    uint64_t param1,
    int itemID,
    int amount,
    uint64_t inventoryPtr,
    int mode
)
{
    // Ignore fake startup calls

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
            "\n/g[Inventory] Player inventory detected!\n"
            //"Pointer : 0x%llX\n"
            //"Param1  : 0x%llX\n"
            //"Item ID : %d\n\n",
            //g_inventoryPtr,
            //g_param1,
            //itemID
        );


        g_AP.InventoryReady();
    }


    // Game -> AP location checks

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

    // Progressive location check
    // The original inventory function has now completed,
    // so the inventory contains the newly acquired item.
    // The current total determines which progressive
    // location check this should give.

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
                    "[AP] Progressive /g%s /wacquired /g#%d, /rbut no check exists for this count\n",
                    progressive->name.c_str(),
                    currentAmount
                );
            }
        }
    }

    // Goal check after inventory has actually updated
    // Golden Ball wincon check
    if (
        itemID == GOLD_BALL_ID &&
        IsInventoryReady()
        )
    {
        int count =
            GetItemAmount(GOLD_BALL_ID);

        int required =
            g_APOptions.GetRequiredGoldenBallCount();

        printf(
            "[AP] Gold Ball count after inventory update: /g%d/%d\n",
            count,
            required
        );

        if (count >= required)
        {
            printf(
                "/y[AP] Golden Ball wincon reached!\n"
            );

            g_AP.MarkWinconComplete(
                APWincon::GoldenBall
            );
        }
    }

    return result;
}

// Hook setup

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
            "/r[Inventory] Pattern not found\n"
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
            "/r[Inventory] CreateHook failed: %d\n",
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
            "/r[Inventory] EnableHook failed: %d\n",
            status
        );

        return false;
    }


    //printf(
    //    "/w[Inventory] Inventory_SetQuantity hook enabled!\n"
    //);


    return true;
}