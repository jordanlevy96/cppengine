/**
 * @file ConfigLoader.h
 * @brief Utility for loading configuration from YAML files
 */

#pragma once

#include "util/Config.h"
#include <string>

/**
 * @brief Utility namespace for configuration loading
 *
 * Provides shared configuration loading functionality for both
 * the game engine and editor applications.
 */
namespace ConfigLoader
{
    /**
     * @brief Load configuration from YAML file
     *
     * Parses a YAML configuration file and populates the provided Config struct.
     * Supports sections: input, window, and game.
     *
     * @param configPath Path to YAML configuration file
     * @param config Reference to Config struct to populate
     * @return true if loaded successfully, false on error
     *
     * @note Logs error if file cannot be loaded or parsed
     * @see Config struct definition in controllers/Game.h
     *
     * @code
     * Config conf;
     * if (ConfigLoader::LoadConfig("../res/conf/settings.yaml", conf)) {
     *     // Use conf.WindowWidth, conf.AppName, etc.
     * }
     * @endcode
     */
    bool LoadConfig(const std::string &configPath, Config &config);

} // namespace ConfigLoader
