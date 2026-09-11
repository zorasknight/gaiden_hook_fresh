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

struct TaskState
{
    uint32_t p4;
    uint8_t p5;
    uint8_t p6;
    bool valid;
};

TaskState g_taskStates[16] = {};



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
        "[Timeline] Set /g%u/%u/%u\n",
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