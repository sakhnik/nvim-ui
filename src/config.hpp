#pragma once

#include <libintl.h>
#include <string>
#include <filesystem>

#define _(x) gettext((x))

class ResourceDir
{
public:
    static void Initialize(const char *exe);

    static std::string Get(std::filesystem::path build_path, std::filesystem::path install_path);

private:
    static std::string _exe;
};
