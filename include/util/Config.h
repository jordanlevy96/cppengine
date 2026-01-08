/**
 * @file Config.h
 * @brief Application configuration structure
 */

#pragma once

#include <string>

/**
 * @brief Application configuration loaded from YAML
 *
 * Shared configuration structure used by both Game and Editor applications.
 * Populated by ConfigLoader::LoadConfig() from settings.yaml files.
 */
struct Config
{
    float WindowWidth = 800;              ///< Initial window width (default: 800)
    float WindowHeight = 600;             ///< Initial window height (default: 600)
    float targetFPS = 60;                 ///< Target frames per second (default: 60)
    std::string ResourcePath = "../res/"; ///< Base path for resources
    std::string LogPath = "../logs/";     ///< Base path for logs
    std::string AppName = "Imhotep";      ///< Application name (default: "Imhotep")
    std::string ScenePath = "";           ///< Optional scene file to load (relative to res/)
    std::string HTMLPath = "";            ///< Optional HTML template file to load (relative to res/)
    std::string CSSPath = "";             ///< Optional CSS file to load (relative to res/)
    std::string LuaStatePath = "";        ///< Optional Lua state file to load (relative to res/)
    std::string TemplateName = "game";    ///< Optional name of template to register with ReactiveUI
    bool Fullscreen = false;              ///< Start in fullscreen mode
    bool Debug = false;                   ///< Enable debug mode
};
