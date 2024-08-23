#include "settings.hpp"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <globals.hpp>

#include <nexus/Nexus.h>

using json = nlohmann::json;
namespace Settings
{
const char *IS_ADDON_ENABLED = "IsAddonEnabled";
const char *NOTES_PATH = "NotesPath";

json json_settings;
std::mutex mutex;
std::filesystem::path settings_path;

bool is_addon_enabled = true;
std::filesystem::path notes_path;

void load(const std::filesystem::path &path)
{
    json_settings = json::object();
    if (!std::filesystem::exists(path)) {
        return;
    }

    {
        std::lock_guard lock(mutex);
        try {
            if (std::ifstream file(path); file.is_open()) {
                json_settings = json::parse(file);
                file.close();
            }
        } catch (json::parse_error &ex) {
            api->Log(ELogLevel_WARNING, "App Launcher", "settings.json could not be parsed.");
            api->Log(ELogLevel_WARNING, "App Launcher", ex.what());
        }
    }
    if (!json_settings[IS_ADDON_ENABLED].is_null()) {
        json_settings[IS_ADDON_ENABLED].get_to(is_addon_enabled);
    }
    if (!json_settings[NOTES_PATH].is_null()) {
        json_settings[NOTES_PATH].get_to(notes_path);
    }
    api->Log(ELogLevel_INFO, "App Launcher", "settings loaded!");
}

void save(const std::filesystem::path &path)
{
    if (json_settings.is_null()) {
        api->Log(ELogLevel_WARNING, "App Launcher", "settings.json is null, cannot save.");
        return;
    }
    if (!std::filesystem::exists(path.parent_path())) {
        std::filesystem::create_directories(path.parent_path());
    }
    {
        std::lock_guard lock(mutex);
        if (std::ofstream file(path); file.is_open()) {
            file << json_settings.dump(1, '\t') << std::endl;
            file.close();
        }
        api->Log(ELogLevel_INFO, "App Launcher", "settings saved!");
    }
}
} // namespace Settings
