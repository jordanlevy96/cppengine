#include <Script.h>

#include <iostream>

void Script::Run(char *scriptSrc)
{
    std::cout << "Running script!" << std::endl;
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::os, sol::lib::math);

    lua.script_file(scriptSrc);
}
