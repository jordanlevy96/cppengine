#include <GameManager.h>

int main()
{
    GameManager &gm = GameManager::GetInstance();

    gm.Initialize();
    gm.Run();

    return 0;
}