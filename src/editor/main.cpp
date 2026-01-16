/**
 * @file main.cpp (editor)
 * @brief Imhotep Editor entry point
 */

#include "editor/Editor.h"
#include "util/Logger.h"

#include <iostream>

int main(int argc, char *argv[])
{
    // Initialize logger with separate file for editor (must be done before EngineCore initialization)
    imhotep::Logger::GetInstance().Initialize("../logs/imhotep-editor.log");

    Editor &editor = Editor::GetInstance();

    if (!editor.Initialize())
    {
        std::cerr << "Editor initialization failed" << std::endl;
        return 1;
    }

    editor.Run();
    editor.Shutdown();

    LOG_INFO("Editor exited cleanly");
    return 0;
}
