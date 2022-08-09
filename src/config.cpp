#include "config.hpp"


std::string GetResourceDir(const char *exe,
        std::filesystem::path build_path,
        std::filesystem::path install_path)
{
    // If running from the build directory, use local settings schema.
    namespace fs = std::filesystem;
    auto root_dir = fs::weakly_canonical(fs::path(exe)).parent_path().parent_path();
    if (fs::exists(root_dir / "build.ninja"))
    {
        auto res_dir = root_dir / build_path;
        if (fs::exists(res_dir) && fs::is_directory(res_dir))
            return res_dir.string();
    }

    // Reach the settings schema relative to the binary for Windows.
    auto res_dir = root_dir / install_path;
    if (fs::exists(res_dir) && fs::is_directory(res_dir))
        return res_dir.string();

    // Fallback to the system directory otherwise.
    res_dir = PREFIX / install_path;
    return res_dir.string();
}
