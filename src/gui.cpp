#include "gui.hpp"
#include "globals.hpp"
#include <fstream>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <settings.hpp>

void render_file_browser()
{
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
}

bool editing_text = false;
void render_text_editor()
{
    if (!files.empty() && ImGui::BeginChild("Editor##NotesEditor", {0, 0}, true)) {
        ImGui::Text("%s", files[open_file].name.c_str());
        ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
        if (editing_text && ImGui::SmallButton("Save##NotesSave") ||
            (ImGui::GetIO().KeyMods == ImGuiKeyModFlags_Ctrl && ImGui::IsKeyPressed('S', false))) {
            api->Log(ELogLevel_INFO, addon_name, "Saving!");
            std::ofstream output_file(files[open_file].path);
            output_file << files[open_file].buffer;
            output_file.close();
            editing_text = false;
        }
        if (!editing_text) {
            if (ImGui::SmallButton("Edit##NotesEditText")) {
                editing_text = true;
            }
        }
        static std::string texture_index;
        if (editing_text) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::InputTextMultiline("##NotesEditorText", &files[open_file].buffer, ImVec2(-FLT_MIN, -FLT_MIN),
                                      ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopStyleVar();
        } else {
            int offset = 0;
            while (true) {
                const auto pos = files[open_file].buffer.find(':', offset);
                const auto pos2 = files[open_file].buffer.find(':', pos + 1);
                if (pos == std::string::npos || pos2 == std::string::npos || pos2 < pos) {
                    ImGui::Text(files[open_file].buffer.substr(offset).c_str());
                    break;
                }
                char log[256];
                sprintf_s(log, "offset: %d, pos: %d, pos2: %d", offset, pos, pos2);
                api->Log(ELogLevel_INFO, addon_name, log);
                auto identifier = files[open_file].buffer.substr(pos + 1, pos2 - pos - 1);
                api->Log(ELogLevel_INFO, addon_name, identifier.c_str());
                bool found_texture = false;
                for (const auto &name : textures | std::views::keys) {
                    if (const auto texture_identifier = std::string("NOTES_IMAGE_").append(identifier);
                        texture_identifier == name) {
                        if (pos != 0) {
                            auto sub = files[open_file].buffer.substr(offset, pos - offset);
                            auto pos3 = sub.find('\n');
                            if (pos3 != std::string::npos) {
                                ImGui::Text("%s", sub.substr(0, pos3 - 1).c_str());
                                ImGui::Text("%s", sub.substr(pos3).c_str());
                            } else {
                                ImGui::Text("%s", files[open_file].buffer.substr(offset, pos - offset).c_str());
                                ImGui::SameLine();
                            }
                        }
                        ImGui::Image(textures[texture_identifier]->Resource, ImVec2(64, 64));
                        ImGui::SameLine();
                        offset = pos2 + 1;
                        found_texture = true;
                        break;
                    }
                }
                if (!found_texture) {
                    offset = pos2 + 1;
                }
            }
        }
        ImGui::EndChild();
    }
}