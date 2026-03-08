#include <controllers/Game.h>
#include "util/PathResolver.h"
#include <cstring>
#include <string>

int main(int argc, char* argv[])
{
    // Check for flags
    bool smokeTest = false;
    bool clickTest = false;
    std::string configPath;
    for (int i = 1; i < argc; i++)
    {
        if (std::strcmp(argv[i], "--smoke-test") == 0)
        {
            smokeTest = true;
        }
        else if (std::strcmp(argv[i], "--click-test") == 0)
        {
            clickTest = true;
        }
        else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc)
        {
            configPath = argv[++i];
        }
    }

    // Resolve config path if not explicitly specified
    if (configPath.empty())
    {
        // IMHOTEP_GAME_CONFIG is set by CMake; fallback uses PathResolver for both modes
        const char *gameConfig = "games/vaporqube/conf/settings.yaml";
#ifdef IMHOTEP_GAME_CONFIG
        gameConfig = IMHOTEP_GAME_CONFIG;
#endif
        configPath = PathResolver::GetResourcePath() + gameConfig;
    }

    Game &game = Game::GetInstance();

    if (smokeTest || clickTest)
    {
        game.SetTestMode(true);
    }

    if (!game.Initialize(configPath))
    {
        return 1;
    }

    if (clickTest)
    {
        bool passed = game.RunClickTest();
        game.Shutdown();
        return passed ? 0 : 1;
    }
    else if (smokeTest)
    {
        game.RunFrames(10);
    }
    else
    {
        game.Run();
    }

    game.Shutdown();
    return 0;
}