#include "globals.hpp"

HMODULE self_module = nullptr;
AddonDefinition addon_def{};
AddonAPI *api = nullptr;
char addon_name[] = "Notes";
HWND game_handle = nullptr;
std::vector<File> files;
bool create_new_file = false;
char new_file_name[256] = {};