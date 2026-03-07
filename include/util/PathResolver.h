/**
 * @file PathResolver.h
 * @brief Runtime path detection for development vs installed bundle modes
 * @lines ~45
 *
 * Purpose: Detect whether running from build directory or installed app bundle
 * and resolve resource paths accordingly.
 *
 * Quick-stats (Public API):
 * - GetExecutableDir() - Returns absolute path to executable's directory
 * - IsInstalledBundle() - True if running from installed bundle (.app on macOS, res/ next to exe on Win/Linux)
 * - GetResourcePath() - Resolves res/ location based on runtime environment (macOS bundle, Win/Linux flat dir, or dev mode)
 * - GetBundledPythonHome() - Returns bundled Python path or empty string
 */

#pragma once

#include <string>

namespace PathResolver
{
    /**
     * @brief Get the absolute path to the directory containing the executable
     * @return Absolute path ending with /
     */
    std::string GetExecutableDir();

    /**
     * @brief Check if running from an installed bundle
     * @return True if running from .app bundle (macOS) or res/ exists next to exe (Win/Linux)
     */
    bool IsInstalledBundle();

    /**
     * @brief Get the resource path based on runtime environment
     * @return Path to res/ directory (macOS bundle: ../Resources/res/, Win/Linux installed: ./res/, dev: ../res/)
     */
    std::string GetResourcePath();

    /**
     * @brief Get the bundled Python home directory if available
     * @return Path to bundled Python or empty string if not bundled/dev mode
     */
    std::string GetBundledPythonHome();

} // namespace PathResolver
