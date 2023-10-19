#include <GameManager.h>

int main()
{
    GameManager &gm = GameManager::GetInstance();

    gm.Initialize();
    gm.Run();
    gm.Shutdown();

    return 0;
}