#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

struct GLFWwindow;
class GameManager
{
    // Singleton template yanked from https://stackoverflow.com/a/1008289/9028567

    public:
        static GameManager &GetInstance()
        {
            static GameManager instance;
            return instance;
        }

        GameManager(GameManager const &) = delete;
        void operator=(GameManager const &) = delete;

        // Note: Scott Meyers mentions in his Effective Modern
        //       C++ book, that deleted functions should generally
        //       be public as it results in better error messages
        //       due to the compilers behavior to check accessibility
        //       before deleted status

        int Initialize();

        void Run();
        GLFWwindow *window;

    private:
        GameManager(){};
    };

#endif