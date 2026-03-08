/**
 * @file PathResolver.cpp
 * @brief Implementation of runtime path detection utilities
 * @lines ~85
 *
 * Purpose: Detect runtime environment (development vs installed bundle)
 * and resolve paths accordingly for cross-platform distribution.
 *
 * Key functions:
 * - GetExecutableDir() - Platform-specific executable path detection (line ~25)
 * - IsInstalledBundle() - Installed bundle detection: .app on macOS, res/ next to exe on Win/Linux (line ~55)
 * - GetResourcePath() - Resource path resolution (line ~70)
 * - GetBundledPythonHome() - Bundled Python detection (line ~80)
 *
 * Platform notes:
 * - macOS: Uses _NSGetExecutablePath
 * - Linux: Uses /proc/self/exe
 * - Windows: Uses GetModuleFileName
 */

#include "util/PathResolver.h"
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

namespace PathResolver
{

std::string GetExecutableDir()
{
    std::string execPath;

#ifdef __APPLE__
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        execPath = path;
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    execPath = path;
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1)
    {
        execPath = std::string(path, count);
    }
#endif

    if (!execPath.empty())
    {
        fs::path p(execPath);
        return p.parent_path().string() + "/";
    }

    // Fallback to current directory
    return fs::current_path().string() + "/";
}

bool IsInstalledBundle()
{
    std::string execDir = GetExecutableDir();

#ifdef __APPLE__
    // Check if we're inside a .app bundle structure
    // Path should contain .app/Contents/MacOS
    if (execDir.find(".app/Contents/MacOS") != std::string::npos)
    {
        // Verify Resources/res exists
        fs::path resourcesPath = fs::path(execDir) / ".." / "Resources" / "res";
        return fs::exists(resourcesPath);
    }
#else
    // Windows/Linux: check if res/ exists next to executable
    fs::path resourcesPath = fs::path(execDir) / "res";
    if (fs::exists(resourcesPath))
    {
        return true;
    }
#endif
    return false;
}

std::string GetResourcePath()
{
    std::string execDir = GetExecutableDir();

    if (IsInstalledBundle())
    {
#ifdef __APPLE__
        // Bundle mode: executable is in .app/Contents/MacOS/
        // Resources are in .app/Contents/Resources/res/
        fs::path resourcesPath = fs::path(execDir) / ".." / "Resources" / "res";
        return fs::canonical(resourcesPath).string() + "/";
#else
        // Windows/Linux: res/ is next to executable
        fs::path resourcesPath = fs::path(execDir) / "res";
        return fs::canonical(resourcesPath).string() + "/";
#endif
    }

    // Development mode: assume running from build/ directory
    // Resources are in ../res/
    return "../res/";
}

std::string GetLogPath(const std::string &appName)
{
    if (!IsInstalledBundle())
    {
        // Development mode: logs next to build directory
        return "../logs/";
    }

#ifdef __APPLE__
    // macOS: ~/Library/Logs/<AppName>/
    const char *home = std::getenv("HOME");
    if (home)
    {
        fs::path logDir = fs::path(home) / "Library" / "Logs" / appName;
        fs::create_directories(logDir);
        return logDir.string() + "/";
    }
#endif

    // Windows/Linux: logs/ next to executable
    std::string execDir = GetExecutableDir();
    fs::path logDir = fs::path(execDir) / "logs";
    fs::create_directories(logDir);
    return logDir.string() + "/";
}

std::string GetBundledPythonHome()
{
    if (!IsInstalledBundle())
    {
        // Development mode - use system Python
        return "";
    }

    std::string execDir = GetExecutableDir();

#ifdef __APPLE__
    fs::path pythonPath = fs::path(execDir) / ".." / "Resources" / "python";
#else
    fs::path pythonPath = fs::path(execDir) / "python";
#endif

    if (fs::exists(pythonPath))
    {
        return fs::canonical(pythonPath).string();
    }

    // No bundled Python found
    return "";
}

} // namespace PathResolver
