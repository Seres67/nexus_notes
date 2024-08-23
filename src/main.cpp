#include <globals.hpp>
#include <gui.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <nexus/Nexus.h>
#include <settings.hpp>
#include <vector>
#include <windows.h>

void addon_load(AddonAPI *api_p);
void addon_unload();
void addon_render();
void addon_options();

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
    addon_def.Version.Revision = 0;
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
    } else {
        Settings::json_settings[Settings::IS_ADDON_ENABLED] = Settings::is_addon_enabled;
        Settings::json_settings[Settings::NOTES_PATH] = Settings::notes_path;
        Settings::save(Settings::settings_path);
        std::filesystem::create_directories(Settings::notes_path);
    }
    api->QuickAccess.Add("QA_NOTES", "ICON_NOTES", "ICON_NOTES_HOVER", "KB_NOTES_TOGGLE", "Notes");
    api->Log(ELogLevel_INFO, addon_name, "addon loaded!");
}

void addon_unload()
{
    api->Log(ELogLevel_INFO, addon_name, "unloading addon...");
    for (auto &[name, path, buffer] : files) {
        char log[256];
        sprintf_s(log, "Closing file: %s", path.c_str());
        api->Log(ELogLevel_DEBUG, addon_name, log);
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

bool tmp_open = true;
static int open_file = 0;
void addon_render()
{
    if (tmp_open && ImGui::Begin("Notes###NotesMainWindow", &tmp_open, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginChild("Filesystem##NotesFS", {150, -FLT_MIN}, true)) {
            if (ImGui::SmallButton("Create##NotesCreate")) {
                create_new_file = true;
            }
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            ImGui::SmallButton("Delete##NotesDelete");
            if (create_new_file) {
                ImGui::InputText("Filename##NotesNewFileInput", new_file_name, 256);
                if (ImGui::SmallButton("Confirm##NotesConfirmCreate") || ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
                    std::string file_path = Settings::notes_path.string() + "\\" + (std::string(new_file_name));
                    char log[256];
                    sprintf_s(log, "Creating file: %s", file_path.c_str());
                    api->Log(ELogLevel_DEBUG, addon_name, log);
                    std::ifstream input_file(file_path);
                    std::string buffer;
                    if (input_file.is_open()) {
                        input_file.seekg(0, std::ios::end);
                        buffer.resize(input_file.tellg());
                        input_file.seekg(0, std::ios::beg);
                        input_file.read(&buffer[0], buffer.size());
                        api->Log(ELogLevel_DEBUG, addon_name, "File opened & read!");
                    }
                    input_file.close();
                    files.emplace_back(new_file_name, file_path, buffer);
                    memset(new_file_name, 0, 256);
                    create_new_file = false;
                }
            }
            for (int i = 0; i < files.size(); i++) {
                if (ImGui::Selectable(files[i].name.c_str(), files[open_file].name == files[i].name)) {
                    open_file = i;
                }
            }
            ImGui::EndChild();
        }
        ImGui::SameLine(0, 0 * ImGui::GetStyle().ItemSpacing.x);
        if (!files.empty() && ImGui::BeginChild("Editor##NotesEditor", {0, 0}, true)) {
            ImGui::Text("%s", files[open_file].name.c_str());
            ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::SmallButton("Save") ||
                (ImGui::GetIO().KeyMods == ImGuiKeyModFlags_Ctrl && ImGui::IsKeyPressed('S', false))) {
                api->Log(ELogLevel_INFO, addon_name, "Save button pressed!");
                char log[256];
                sprintf_s(log, "Saved %s, content %s", files[open_file].name.c_str(), files[open_file].buffer.c_str());
                api->Log(ELogLevel_DEBUG, addon_name, log);
                std::ofstream output_file(files[open_file].path);
                output_file << files[open_file].buffer;
                output_file.close();
            }
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::InputTextMultiline("##NotesEditorText", &files[open_file].buffer, ImVec2(-FLT_MIN, -FLT_MIN),
                                      ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopStyleVar();
            ImGui::EndChild();
        }
        ImGui::End();
    }
}

void addon_options() {}
