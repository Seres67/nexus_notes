#include "globals.hpp"
#include <fstream>
#include <gui.hpp>
#include <imgui/imgui_internal.h>
#include <nexus/Nexus.h>
#include <settings.hpp>
#include <vector>
#include <windows.h>

void addon_load(AddonAPI *api_p);
void addon_unload();
void addon_render();
void addon_options();
void texture_callback(const char *identifier, Texture *texture);
void keybind_handler(const char *identifier, bool is_release);

BOOL APIENTRY dll_main(const HMODULE hModule, const DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        self_module = hModule;
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }
    return TRUE;
}

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" __declspec(dllexport) AddonDefinition *GetAddonDef()
{
    addon_def.Signature = -912894234;
    addon_def.APIVersion = NEXUS_API_VERSION;
    addon_def.Name = addon_name;
    addon_def.Version.Major = 0;
    addon_def.Version.Minor = 1;
    addon_def.Version.Build = 0;
    addon_def.Version.Revision = 4;
    addon_def.Author = "Seres67";
    addon_def.Description = "A Nexus addon to take notes in game.";
    addon_def.Load = addon_load;
    addon_def.Unload = addon_unload;
    addon_def.Flags = EAddonFlags_None;
    addon_def.Provider = EUpdateProvider_GitHub;
    addon_def.UpdateLink = "https://github.com/Seres67/nexus_notes";

    return &addon_def;
}

void addon_load(AddonAPI *api_p)
{
    api = api_p;

    ImGui::SetCurrentContext(static_cast<ImGuiContext *>(api->ImguiContext));
    ImGui::SetAllocatorFunctions(static_cast<void *(*)(size_t, void *)>(api->ImguiMalloc),
                                 static_cast<void (*)(void *, void *)>(api->ImguiFree)); // on imgui 1.80+
    api->Renderer.Register(ERenderType_Render, addon_render);
    api->Renderer.Register(ERenderType_OptionsRender, addon_options);

    Settings::settings_path = api->Paths.GetAddonDirectory("notes\\settings.json");
    Settings::notes_path = api->Paths.GetAddonDirectory("notes\\notes");
    Settings::images_path = api->Paths.GetAddonDirectory("notes\\images");
    if (std::filesystem::exists(Settings::settings_path)) {
        Settings::load(Settings::settings_path);

        if (std::filesystem::exists(Settings::notes_path)) {
            for (const std::filesystem::directory_iterator dir(Settings::notes_path); const auto &entry : dir) {
                if (entry.is_regular_file()) {
                    std::ifstream input_file(entry.path().string());
                    std::string buffer;
                    if (input_file.is_open()) {
                        input_file.seekg(0, std::ios::end);
                        buffer.resize(input_file.tellg());
                        input_file.seekg(0, std::ios::beg);
                        input_file.read(&buffer[0], buffer.size());
                        api->Log(ELogLevel_DEBUG, addon_name, "File opened & read!");
                    }
                    input_file.close();
                    files.emplace_back(entry.path().filename().string(), entry.path().string(), buffer);
                }
            }
        } else {
            std::filesystem::create_directories(Settings::notes_path);
        }
        if (std::filesystem::exists(Settings::images_path)) {
            for (const std::filesystem::directory_iterator dir(Settings::images_path); const auto &entry : dir) {
                if (entry.is_regular_file()) {
                    const auto pos = entry.path().filename().string().find('.');
                    const auto identifier =
                        std::string("NOTES_IMAGE_").append(entry.path().filename().string().substr(0, pos));
                    api->Textures.LoadFromFile(identifier.c_str(), entry.path().string().c_str(), texture_callback);
                }
            }
        } else {
            std::filesystem::create_directories(Settings::images_path);
        }
    } else {
        Settings::json_settings[Settings::IS_ADDON_ENABLED] = Settings::is_addon_enabled;
        Settings::json_settings[Settings::NOTES_PATH] = Settings::notes_path;
        Settings::save(Settings::settings_path);
        std::filesystem::create_directories(Settings::notes_path);
    }
    api->QuickAccess.Add("QA_NOTES", "ICON_NOTES", "ICON_NOTES_HOVER", "KB_NOTES_TOGGLE", "Notes");
    api->InputBinds.RegisterWithString("KB_NOTES_TOGGLE", keybind_handler, "((null))");
    api->Log(ELogLevel_INFO, addon_name, "addon loaded!");
}

void addon_unload()
{
    api->Log(ELogLevel_INFO, addon_name, "unloading addon...");
    for (auto &[name, path, buffer] : files) {
        std::ofstream output_file(path);
        output_file << buffer;
        output_file.close();
    }
    api->QuickAccess.Remove("QA_NOTES");
    api->Renderer.Deregister(addon_render);
    api->Renderer.Deregister(addon_options);
    api->Log(ELogLevel_INFO, addon_name, "addon unloaded!");
    api = nullptr;
}

bool tmp_open = false;
void addon_render()
{
    ImGui::SetNextWindowPos(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(Settings::window_alpha);
    auto flags = ImGuiWindowFlags_NoCollapse;
    if (tmp_open && ImGui::Begin("Notes###NotesMainWindow", &tmp_open, flags)) {
        render_file_browser();
        ImGui::SameLine(0, 0 * ImGui::GetStyle().ItemSpacing.x);
        render_text_editor();
        ImGui::End();
    }
}

void addon_options()
{
    if (ImGui::Checkbox("Enabled##NotesEnabled", &Settings::is_addon_enabled)) {
        Settings::json_settings[Settings::IS_ADDON_ENABLED] = Settings::is_addon_enabled;
        Settings::save(Settings::settings_path);
    }
    if (ImGui::SliderFloat("Window Opacity", &Settings::window_alpha, 0.f, 1.f)) {
        Settings::json_settings[Settings::WINDOW_ALPHA] = Settings::window_alpha;
        Settings::save(Settings::settings_path);
    }
}

void texture_callback(const char *identifier, Texture *texture)
{
    if (identifier == nullptr || texture == nullptr)
        return;
    // if (!strncmp(identifier, "IMAGE_", 6)) {
    // }
    api->Log(ELogLevel_INFO, addon_name, "texture loaded!");
    textures[identifier] = texture;
}

void keybind_handler(const char *identifier, bool is_release)
{
    if (!strcmp(identifier, "KB_NOTES_TOGGLE")) {
        tmp_open = !tmp_open;
    }
}