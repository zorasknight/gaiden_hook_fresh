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



HMODULE g_Module = nullptr;


DWORD WINAPI ConsoleThread(LPVOID)
{
    char command[256];
    char arg1[256] = {};
    char arg2[256] = {};

    printf("\nConsole ready.\n");
    printf("Waiting for player inventory...\n\n");


    while (true)
    {
        printf("> ");
        fflush(stdout);


        if (!fgets(command, sizeof(command), stdin))
            continue;


        int itemID;
        int amount;


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



        // =========================
        // Archipelago
        // =========================


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



        else if (strncmp(command, "ap slot ", 8) == 0)
        {
            arg1[0] = '\0';
            arg2[0] = '\0';


            int count = sscanf_s(
                command,
                "ap slot %255s %255s",
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



        else if (strncmp(command, "ap say ", 7) == 0)
        {
            std::string message = command + 7;


            size_t newline = message.find('\n');

            if (newline != std::string::npos)
                message.erase(newline);


            g_AP.Say(message);
        }

        else if (sscanf_s(command, "ap give %d", &itemID) == 1)
        {
            printf("[AP] Giving item %d\n", itemID);
            GiveAPItem(itemID);
        }

        else if (
            sscanf_s(
                command,
                "ap check %d",
                &itemID
            ) == 1)
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


    FILE* fp;


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



    printf(
        "[YakLadGai] DLL loaded\n"
    );


    Sleep(3000);



    MH_STATUS status = MH_Initialize();


    printf(
        "MinHook status: %d\n",
        status
    );



    if (status != MH_OK)
    {
        printf(
            "[YakLadGai] MinHook failed\n"
        );

        return 0;
    }



    printf(
        "[YakLadGai] MinHook initialized\n"
    );



    // =========================
    // Load AP database
    // =========================


    printf(
        "[AP] Loading item database...\n"
    );


    if (!g_APItems.Load(
        "mods/Gaiden_Rando_072526/item_mapping.csv"
    ))
    {
        printf(
            "[AP] Item database failed to load\n"
        );
    }
    else
    {
        printf(
            "[AP] Item database loaded\n"
        );
    }



    // =========================
    // Install inventory hook
    // =========================


    if (!HookInventory())
    {
        printf(
            "[YakLadGai] Inventory hook failed\n"
        );
    }
    else
    {
        printf(
            "[YakLadGai] Inventory hook installed\n"
        );
    }



    fflush(stdout);



    // AP can initialize before inventory exists

   // AP can initialize before inventory exists

    g_AP.Initialize(
        "mods/Gaiden_Rando_072526/APState.json"
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



    while (true)
    {
        g_AP.Update();
        Sleep(16);
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