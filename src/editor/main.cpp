/**
 * @file main.cpp (editor)
 * @brief Imhotep Editor entry point
 */

#include "editor/Editor.h"
#include "util/Logger.h"

#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "==================================" << std::endl;
    std::cout << "   Imhotep Editor - Phase 1" << std::endl;
    std::cout << "==================================" << std::endl;

    // Initialize logger first (required before any LOG_* macros)
    imhotep::Logger::GetInstance().Initialize("logs/editor.log");

    Editor& editor = Editor::GetInstance();

    if (!editor.Initialize())
    {
        LOG_CRITICAL("Editor initialization failed");
        return 1;
    }

    editor.Run();
    editor.Shutdown();

    LOG_INFO("Editor exited cleanly");
    return 0;
}
