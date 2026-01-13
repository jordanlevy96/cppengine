/**
 * @file ConfigLoader.cpp
 * @brief Implementation of configuration loading utility
 */

#include "util/ConfigLoader.h"
#include "util/Logger.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

namespace ConfigLoader
{

    bool LoadConfig(const std::string &configPath, Config &config)
    {
        std::cout << "[ConfigLoader] Starting config load from: " << configPath << std::endl;

        try
        {
            std::cout << "[ConfigLoader] Loading YAML file..." << std::endl;
            YAML::Node yamlConfig = YAML::LoadFile(configPath);
            std::cout << "[ConfigLoader] YAML file loaded successfully" << std::endl;

            if (!yamlConfig)
            {
                std::cerr << "[ConfigLoader] ERROR: Failed to read config from " << configPath << std::endl;
                return false;
            }

            // Parse window settings
            std::cout << "[ConfigLoader] Parsing window settings..." << std::endl;
            YAML::Node window = yamlConfig["window"];
            if (window)
            {
                if (window["windowWidth"])
                    config.WindowWidth = window["windowWidth"].as<float>();
                if (window["windowHeight"])
                    config.WindowHeight = window["windowHeight"].as<float>();
                if (window["targetFPS"])
                    config.targetFPS = window["targetFPS"].as<float>();
            }
            std::cout << "[ConfigLoader] Window settings parsed" << std::endl;

            // Parse game settings
            std::cout << "[ConfigLoader] Parsing game settings..." << std::endl;
            YAML::Node game = yamlConfig["game"];
            if (game)
            {
                if (game["appName"])
                    config.AppName = game["appName"].as<std::string>();
                if (game["logPath"])
                    config.LogPath = game["logPath"].as<std::string>();
                if (game["resourcePath"])
                    config.ResourcePath = game["resourcePath"].as<std::string>();
                if (game["scenePath"])
                    config.ScenePath = game["scenePath"].as<std::string>();
                if (game["htmlPath"])
                    config.HTMLPath = game["htmlPath"].as<std::string>();
                if (game["cssPath"])
                    config.CSSPath = game["cssPath"].as<std::string>();
                if (game["luaStatePath"])
                    config.LuaStatePath = game["luaStatePath"].as<std::string>();
                if (game["templateName"])
                    config.TemplateName = game["templateName"].as<std::string>();
            }
            std::cout << "[ConfigLoader] Game settings parsed" << std::endl;

            std::cout << "[ConfigLoader] Configuration loaded successfully from " << configPath << std::endl;
            return true;
        }
        catch (const YAML::Exception &e)
        {
            std::cerr << "[ConfigLoader] ERROR: YAML parsing error in " << configPath << ": " << e.what() << std::endl;
            return false;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ConfigLoader] ERROR: Exception loading config from " << configPath << ": " << e.what() << std::endl;
            return false;
        }
    }

} // namespace ConfigLoader
