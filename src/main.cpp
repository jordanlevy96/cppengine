#include <controllers/Game.h>
#include <cstring>

int main(int argc, char* argv[])
{
    // Check for test flags
    bool smokeTest = false;
    bool clickTest = false;
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
    }

    Game &game = Game::GetInstance();

    if (smokeTest || clickTest)
    {
        game.SetTestMode(true);
    }

    if (!game.Initialize())
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