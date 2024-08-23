#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <nexus/Nexus.h>
#include <string>
#include <vector>
#include <map>
#include <imgui/imgui.h>

// handle to self hmodule
extern HMODULE self_module;
// addon definition
extern AddonDefinition addon_def;
// addon api
extern AddonAPI *api;

extern char addon_name[];

extern HWND game_handle;

typedef struct
{
    std::string name;
    std::string path;
    std::string buffer;
} File;
extern std::vector<File> files;

extern bool create_new_file;
extern char new_file_name[256];

extern std::map<std::string, Texture *> textures;

extern int open_file;

#endif // GLOBALS_HPP
