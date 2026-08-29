#include "APLocationNames.h"

#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cctype>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

APLocationNames g_APLocationNames;


// =====================================================
// Helpers
// =====================================================

static std::string RemoveTrailingSlot(
    const std::string& label
)
{
    size_t pos =
        label.find_last_of(' ');

    if (pos == std::string::npos)
        return label;

    std::string last =
        label.substr(pos + 1);

    if (
        !last.empty() &&
        std::all_of(
            last.begin(),
            last.end(),
            [](unsigned char c)
            {
                return std::isdigit(c);
            }
        )
        )
    {
        return label.substr(0, pos);
    }

    return label;
}


// =====================================================
// Load location data
// =====================================================

bool APLocationNames::Load(
    const std::string& path
)
{
    std::ifstream file(path);

    if (!file)
    {
        printf(
            "[AP] Failed to open location names: %s\n",
            path.c_str()
        );

        return false;
    }

    json data;

    try
    {
        file >> data;
    }
    catch (const std::exception& e)
    {
        printf(
            "[AP] Failed to parse location names: %s\n",
            e.what()
        );

        return false;
    }

    names.clear();
    locations.clear();

    for (auto& [key, value] : data.items())
    {
        try
        {
            int64_t id =
                std::stoll(
                    value["id"].get<std::string>()
                );

            std::string label =
                value["label"].get<std::string>();

            std::string location =
                value["location"].get<std::string>();

            std::string source =
                value["source"].get<std::string>();

            // Existing ID -> name lookup.
            names[id] = label;

            // Full location information.
            APLocationInfo info;

            info.id = id;
            info.label = label;
            info.location = location;
            info.source = source;

            locations[id] = info;
        }
        catch (const std::exception& e)
        {
            printf(
                "[AP] Failed to load location entry '%s': %s\n",
                key.c_str(),
                e.what()
            );
        }
    }

    printf(
        "[AP] Loaded %zu location names\n",
        names.size()
    );

    return true;
}


// =====================================================
// Location name lookup
// =====================================================

std::string APLocationNames::GetName(
    int64_t location_id
)
{
    auto it =
        names.find(location_id);

    if (it != names.end())
        return it->second;

    return "Unknown Location";
}


// =====================================================
// Find locations belonging to the same shop
// =====================================================

std::vector<int64_t>
APLocationNames::GetShopLocationsFor(
    int64_t location_id
)
{
    std::vector<int64_t> result;

    auto source =
        locations.find(location_id);

    if (source == locations.end())
        return result;

    //
    // Only locations originating from the shop
    // data should participate in shop grouping.
    //
    if (
        source->second.source !=
        "shop.bin.json"
        )
    {
        return result;
    }

    //
    // Remove the trailing slot number.
    //
    std::string sourceBase =
        RemoveTrailingSlot(
            source->second.label
        );

    //
    // If there is no trailing number, this isn't
    // a slot-based shop location.
    //
    if (
        sourceBase ==
        source->second.label
        )
    {
        return result;
    }

    //
    // Find every other shop location with the
    // same base label.
    //
    for (
        const auto& [id, info] :
        locations
        )
    {
        //
        // Only compare against shop locations.
        //
        if (
            info.source !=
            "shop.bin.json"
            )
        {
            continue;
        }

        if (
            RemoveTrailingSlot(info.label) ==
            sourceBase
            )
        {
            result.push_back(id);
        }
    }

    return result;
}