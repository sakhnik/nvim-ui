#pragma once

#include <libintl.h>
#include <string>
#include <filesystem>

#define _(x) gettext((x))

std::string GetResourceDir(const char *exe,
        std::filesystem::path build_path,
        std::filesystem::path install_path);
