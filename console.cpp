#include <stdio.h>

#include <Windows.h>

#include <cstdarg>

#include "Console.h"

void ColorPrintf(const char* format, ...)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO consoleInfo{};
    WORD originalColor =
        FOREGROUND_RED |
        FOREGROUND_GREEN |
        FOREGROUND_BLUE;

    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
    {
        originalColor = consoleInfo.wAttributes;
    }

    // Format the printf-style arguments first.
    char buffer[4096];

    va_list args;
    va_start(args, format);

    vsnprintf_s(
        buffer,
        sizeof(buffer),
        _TRUNCATE,
        format,
        args
    );

    va_end(args);

    WORD currentColor = originalColor;

    const char* textStart = buffer;
    const char* p = buffer;

    while (*p)
    {
        if (*p == '/' && *(p + 1))
        {
            WORD newColor = currentColor;
            bool colorCode = true;

            switch (*(p + 1))
            {
            case 'w':
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_BLUE;
                break;

            case 'r':
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_INTENSITY;
                break;

            case 'g':
                newColor =
                    FOREGROUND_GREEN |
                    FOREGROUND_INTENSITY;
                break;

            case 'b':
                newColor =
                    FOREGROUND_BLUE |
                    FOREGROUND_INTENSITY;
                break;

            case 'c':
                newColor =
                    FOREGROUND_GREEN |
                    FOREGROUND_BLUE |
                    FOREGROUND_INTENSITY;
                break;

            case 'm':
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_BLUE |
                    FOREGROUND_INTENSITY;
                break;

            case 'y':
                newColor =
                    FOREGROUND_RED |
                    FOREGROUND_GREEN |
                    FOREGROUND_INTENSITY;
                break;

            case 'n':
                // Print everything before /n.
                if (p > textStart)
                {
                    fwrite(
                        textStart,
                        1,
                        static_cast<size_t>(p - textStart),
                        stdout
                    );
                }

                putchar('\n');

                p += 2;
                textStart = p;
                continue;

            default:
                colorCode = false;
                break;
            }

            if (colorCode)
            {
                // Print everything before the color code.
                if (p > textStart)
                {
                    fwrite(
                        textStart,
                        1,
                        static_cast<size_t>(p - textStart),
                        stdout
                    );
                }

                currentColor = newColor;
                SetConsoleTextAttribute(
                    hConsole,
                    currentColor
                );

                p += 2;
                textStart = p;
                continue;
            }
        }

        ++p;
    }

    // Print remaining text.
    if (p > textStart)
    {
        fwrite(
            textStart,
            1,
            static_cast<size_t>(p - textStart),
            stdout
        );
    }

    // Restore the original console color.
    SetConsoleTextAttribute(
        hConsole,
        originalColor
    );
}