#pragma once

#include <glm/glm.hpp>

#include <iostream>

#define DEBUG_PRINT std::cout << "!!! " << __FILE__ << ":" << __LINE__ << " !!!" << std::endl;
#define PRINT(x) std::cout << x << std::endl;

static void PrintVec3(const glm::vec3 &vec)
{
    std::cout << vec.x << " " << vec.y << " " << vec.z << std::endl;
}

static void PrintMat4(const glm::mat4 &matrix)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

#ifdef USE_PYTHON_SCRIPTING
#include <pybind11/pybind11.h>
#include <iomanip>

namespace py = pybind11;

static void PrintFormatted(const std::string &key, const py::handle &value, int indent = 0);

static void PrintDict(const py::dict &dict, int indent)
{
    for (auto item : dict)
    {
        PrintFormatted(py::str(item.first).cast<std::string>(), item.second, indent);
    }
}

static void PrintPyObject(const py::object &obj)
{
    PrintDict(obj.attr("__dict__").cast<py::dict>(), 0);
}

static void PrintList(const py::list &list, int indent)
{
    for (auto item : list)
    {
        PrintFormatted("", item, indent);
    }
}

static void PrintFormatted(const std::string &key, const py::handle &value, int indent)
{
    std::string indentation(indent, ' '); // Creates a string with 'indent' number of spaces

    if (py::isinstance<py::dict>(value))
    {
        if (!key.empty())
        {
            std::cout << indentation << std::left << std::setw(20) << key << " : " << std::endl;
        }
        PrintDict(py::reinterpret_borrow<py::dict>(value), indent + 4);
    }
    else if (py::isinstance<py::list>(value))
    {
        if (!key.empty())
        {
            std::cout << indentation << std::left << std::setw(20) << key << " : " << std::endl;
        }
        PrintList(py::reinterpret_borrow<py::list>(value), indent + 4);
    }
    else
    {
        std::string value_str = py::str(value).cast<std::string>();
        std::cout << indentation << std::left << std::setw(20) << key << " : " << value_str << std::endl;
    }
}

#endif