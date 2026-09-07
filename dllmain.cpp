#define ASIO_STANDALONE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX


#include "APClientManager.h"
#include "APItemDatabase.h"

#include <Windows.h>
#include <stdio.h>
#include <cstring>
#include <string>

#include "hooks.h"

#include "MinHook.h"

DWORD lastProgressCheck = 0;

HMODULE g_Module = nullptr;


DWORD WINAPI ConsoleThread(LPVOID)
{
    char command[256];
    char arg1[256] = {};
    char arg2[256] = {};


    printf("\nConsole ready.\n");
    printf("Waiting for player inventory... Interact with any item within the game to conect inventory hook. \nPlease connect to the AP server with /connect archipelago.gg:12345\n");


    while (true)
    {

        arg1[0] = '\0';
        arg2[0] = '\0';
        printf("> ");
        fflush(stdout);


        if (!fgets(command, sizeof(command), stdin))
            continue;


        int itemID;
        int amount;

        int timelineCategory;
        int timelineMission;
        int timelineState;


        if (sscanf_s(command, "set %d %d", &itemID, &amount) == 2)
        {
            printf(
                "Setting item %d to %d...\n",
                itemID,
                amount
            );

            SetItemAmount(
                itemID,
                amount
            );
        }


        else if (sscanf_s(command, "give %d %d", &itemID, &amount) == 2)
        {
            int current = GetItemAmount(itemID);
            int newAmount = current + amount;


            printf(
                "Giving item %d: %d -> %d\n",
                itemID,
                current,
                newAmount
            );


            SetItemAmount(
                itemID,
                newAmount
            );
        }


        else if (sscanf_s(command, "take %d %d", &itemID, &amount) == 2)
        {
            int current = GetItemAmount(itemID);
            int newAmount = current - amount;


            if (newAmount < 0)
                newAmount = 0;


            printf(
                "Taking item %d: %d -> %d\n",
                itemID,
                current,
                newAmount
            );


            SetItemAmount(
                itemID,
                newAmount
            );
        }


        else if (sscanf_s(command, "find %d", &itemID) == 1)
        {
            printf(
                "Item %d quantity: %d\n",
                itemID,
                GetItemAmount(itemID)
            );
        }

        else if (
            sscanf_s(
                command,
                "timeline %d %d %d",
                &timelineCategory,
                &timelineMission,
                &timelineState
            ) == 3
            )
        {
            printf(
                "[Timeline] Setting %d/%d/%d\n",
                timelineCategory,
                timelineMission,
                timelineState
            );

            SetTimelineState(
                timelineCategory,
                timelineMission,
                timelineState
            );
        }




        // Archipelago Connections

        else if (
            sscanf_s(
                command,
                "connect %255[^:]:%d",
                arg1,
                (unsigned)_countof(arg1),
                &itemID
            ) == 2 ||
            sscanf_s(
                command,
                "/connect %255[^:]:%d",
                arg1,
                (unsigned)_countof(arg1),
                &itemID
            ) == 2
            )
        {
            printf(
                "[AP] Connecting to %s:%d\n",
                arg1,
                itemID
            );


            g_AP.Connect(
                arg1,
                itemID
            );
        }


        // Old connect format kept:
        // ap connect archipelago.gg 36115
        else if (
            sscanf_s(
                command,
                "ap connect %255s %d",
                arg1,
                (unsigned)_countof(arg1),
                &itemID
            ) == 2)
        {
            printf(
                "[AP] Connecting to %s:%d\n",
                arg1,
                itemID
            );


            g_AP.Connect(
                arg1,
                itemID
            );
        }


        // slot MySlot password

        else if (
            strncmp(command, "/slot ", 6) == 0 ||
            strncmp(command, "slot ", 5) == 0 ||
            strncmp(command, "ap slot ", 8) == 0
            )
        {
            int offset = 5;


            if (strncmp(command, "ap slot ", 8) == 0)
                offset = 8;


            arg1[0] = '\0';
            arg2[0] = '\0';


            int count = sscanf_s(
                command + offset,
                "%255s %255s",
                arg1,
                (unsigned)_countof(arg1),
                arg2,
                (unsigned)_countof(arg2)
            );


            if (count >= 1)
            {
                printf(
                    "[AP] Connecting slot %s\n",
                    arg1
                );


                if (count == 2)
                {
                    g_AP.ConnectSlot(
                        arg1,
                        arg2
                    );
                }
                else
                {
                    g_AP.ConnectSlot(
                        arg1,
                        ""
                    );
                }
            }
        }



        else if (strncmp(command, "say ", 4) == 0 ||
            strncmp(command, "ap say ", 7) == 0)
        {
            int offset = 4;


            if (strncmp(command, "ap say ", 7) == 0)
                offset = 7;


            std::string message = command + offset;


            size_t newline = message.find('\n');

            if (newline != std::string::npos)
                message.erase(newline);


            g_AP.Say(message);
        }



        else if (
            sscanf_s(command, "give %d", &itemID) == 1 ||
            sscanf_s(command, "ap give %d", &itemID) == 1
            )
        {

            GiveAPItem(itemID);
        }



        else if (
            sscanf_s(command, "check %d", &itemID) == 1 ||
            sscanf_s(command, "ap check %d", &itemID) == 1
            )
        {
            printf(
                "[AP] Checking location %d\n",
                itemID
            );


            g_AP.CheckLocation(itemID);
        }



        else if (strcmp(command, "help\n") == 0)
        {
            printf("\nCommands:\n");

            printf("set  <itemID> <amount>\n");
            printf("give <itemID> <amount>\n");
            printf("take <itemID> <amount>\n");
            printf("find <itemID>\n");

            printf("\nArchipelago:\n");
            printf("ap give <itemID>\n");
            printf("ap connect <host> <port>\n");
            printf("ap slot <slot> [password]\n");
            printf("ap say <message>\n");
            printf("ap check <locationID>\n\n");
            printf("timeline <category> <mission> <state>\n");
        }



        else
        {
            printf("Unknown command.\n");
        }
    }


    return 0;
}




DWORD WINAPI MainThread(LPVOID lpParam)
{
    HMODULE module = (HMODULE)lpParam;


    AllocConsole();

    SetConsoleTitleA("Gaiden Archipelago");

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

    DWORD inputMode = 0;
    if (GetConsoleMode(hInput, &inputMode))
    {
        inputMode |= ENABLE_EXTENDED_FLAGS;
        inputMode |= ENABLE_INSERT_MODE;
        inputMode |= ENABLE_QUICK_EDIT_MODE;

        SetConsoleMode(hInput, inputMode);
    }

    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD outputMode = 0;
    if (GetConsoleMode(hOutput, &outputMode))
    {
        SetConsoleMode(hOutput, outputMode);
    }

    FILE* fp = nullptr;

    freopen_s(
        &fp,
        "CONOUT$",
        "w",
        stdout
    );

    freopen_s(
        &fp,
        "CONIN$",
        "r",
        stdin
    );

    setvbuf(stdout, nullptr, _IONBF, 0);



    printf(
        "[YakLadGai] Archipelago DLL now loaded\n"
    );


    Sleep(3000);



    MH_STATUS status = MH_Initialize();




    if (status != MH_OK)
    {
        printf(
            "[YakLadGai] MinHook failed\n"
        );

        return 0;
    }


    // Load AP database


    if (!g_APItems.Load(
        "mods/Gaiden_Rando/item_mapping.csv"
    ))
    {
        printf(
            "[AP] Item database failed to load\n"
        );
    }


    // Load progressive item database

    if (!g_APProgressiveItems.Load())
    {
        printf(
            "[AP] Progressive item database failed to load\n"
        );
    }


    // Install inventory hook


    if (!HookInventory())
    {
        printf(
            "[YakLadGai] Inventory hook failed\n"
        );
    }
    if (!HookTaskTable())
    {
        printf(
            "[YakLadGai] Task hook failed\n"
        );
    }


    fflush(stdout);

   // AP can initialize before inventory exists

    g_AP.Initialize(
        "mods/Gaiden_Rando/APState.json"
    );


    HANDLE consoleThread = CreateThread(
        nullptr,
        0,
        ConsoleThread,
        nullptr,
        0,
        nullptr
    );



    if (consoleThread)
        CloseHandle(consoleThread);
    else
        printf(
            "Failed to start console thread.\n"
        );

    DWORD lastProgressSync = GetTickCount();

    while (true)
    {
        g_AP.Update();
        Sleep(16);

        if (GetTickCount() - lastProgressCheck >= 30000)
        {
            g_AP.CheckAPProgress();

            lastProgressCheck = GetTickCount();
        }
    }



    return 0;
}





BOOL WINAPI DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_Module = hModule;


        DisableThreadLibraryCalls(
            hModule
        );



        HANDLE thread = CreateThread(
            nullptr,
            0,
            MainThread,
            hModule,
            0,
            nullptr
        );


        if (thread)
            CloseHandle(thread);
    }


    return TRUE;
}