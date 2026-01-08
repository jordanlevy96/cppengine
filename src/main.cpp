#include <controllers/Game.h>

int main()
{
    Game &game = Game::GetInstance();

    game.Initialize();
    game.Run();
    game.Shutdown();

    return 0;
}