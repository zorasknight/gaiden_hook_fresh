#include "APLocationNames.h"

#include <fstream>
#include <cstdio>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

APLocationNames g_APLocationNames;


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

    file >> data;

    names.clear();

    for (auto& [key, value] : data.items())
    {
        int64_t id =
            std::stoll(
                value["id"].get<std::string>()
            );

        std::string label =
            value["label"].get<std::string>();

        names[id] = label;
    }

    printf(
        "[AP] Loaded %zu location names\n",
        names.size()
    );

    return true;
}


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

