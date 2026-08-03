#include "APItemNames.h"

#include <fstream>
#include <cstdio>

#include <nlohmann/json.hpp>


using json = nlohmann::json;


APItemNames g_APItemNames;



bool APItemNames::Load(
    const std::string& path
)
{
    std::ifstream file(path);


    if (!file)
    {
        printf(
            "[AP] Failed to open item names: %s\n",
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
                value["item_id"].get<std::string>()
            );


        std::string label =
            value["label"].get<std::string>();


        names[id] = label;
    }


    printf(
        "[AP] Loaded %zu item names\n",
        names.size()
    );


    return true;
}



std::string APItemNames::GetName(
    int64_t item_id
)
{
    auto it =
        names.find(item_id);


    if (it != names.end())
        return it->second;


    return "Unknown Item";
}