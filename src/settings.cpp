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
const char *IMAGES_PATH = "ImagesPath";
const char *WINDOW_ALPHA = "WindowAlpha";

json json_settings;
std::mutex mutex;
std::filesystem::path settings_path;

bool is_addon_enabled = true;
std::filesystem::path notes_path;
std::filesystem::path images_path;
float window_alpha = 1.f;

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
            api->Log(ELogLevel_WARNING, "Notes", "settings.json could not be parsed.");
            api->Log(ELogLevel_WARNING, "Notes", ex.what());
        }
    }
    if (!json_settings[IS_ADDON_ENABLED].is_null()) {
        json_settings[IS_ADDON_ENABLED].get_to(is_addon_enabled);
    }
    if (!json_settings[NOTES_PATH].is_null()) {
        json_settings[NOTES_PATH].get_to(notes_path);
    }
    if (!json_settings[IMAGES_PATH].is_null()) {
        json_settings[IMAGES_PATH].get_to(images_path);
    }
    if (!json_settings[WINDOW_ALPHA].is_null()) {
        json_settings[WINDOW_ALPHA].get_to(window_alpha);
    }
    api->Log(ELogLevel_INFO, "Notes", "settings loaded!");
}

void save(const std::filesystem::path &path)
{
    if (json_settings.is_null()) {
        api->Log(ELogLevel_WARNING, "Notes", "settings.json is null, cannot save.");
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
        api->Log(ELogLevel_INFO, "Notes", "settings saved!");
    }
}
} // namespace Settings
