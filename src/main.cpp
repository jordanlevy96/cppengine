#include <controllers/Game.h>
#include <cstring>
#include <string>

int main(int argc, char* argv[])
{
    // Check for flags
    bool smokeTest = false;
    bool clickTest = false;
    std::string configPath = "../res/games/tetris/conf/settings.yaml";
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