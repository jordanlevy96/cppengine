/**
 * @file debug.h
 * @brief Debug utilities for printing GLM types and Python objects
 */

#pragma once

#include <glm/glm.hpp>

#include <iostream>

/**
 * @brief Print file and line number for quick debugging
 *
 * Outputs: "!!! /path/to/file.cpp:123 !!!"
 *
 * Usage:
 * @code
 * void MyFunction() {
 *     DEBUG_PRINT  // Prints current location
 *     // ... rest of code
 * }
 * @endcode
 */
#define DEBUG_PRINT std::cout << "!!! " << __FILE__ << ":" << __LINE__ << " !!!" << std::endl;

/**
 * @brief Print expression to stdout with newline
 * @param x Any type supported by std::ostream operator<<
 *
 * Usage:
 * @code
 * PRINT("Debug value: " << myVar);
 * PRINT(entity.GetID());
 * @endcode
 */
#define PRINT(x) std::cout << x << std::endl;

/**
 * @brief Print glm::vec3 to stdout
 * @param vec 3D vector to print
 * @note Format: "x y z" (space-separated, with newline)
 *
 * Usage:
 * @code
 * glm::vec3 pos(1.0f, 2.0f, 3.0f);
 * PrintVec3(pos);  // Outputs: "1 2 3"
 * @endcode
 */
static void PrintVec3(const glm::vec3 &vec)
{
    std::cout << vec.x << " " << vec.y << " " << vec.z << std::endl;
}

/**
 * @brief Print glm::mat4 to stdout in row-major format
 * @param matrix 4x4 matrix to print
 * @note Prints 4 lines with 4 space-separated floats each
 *
 * Usage:
 * @code
 * glm::mat4 view = camera.GetViewMatrix();
 * PrintMat4(view);
 * // Outputs:
 * // 1.0 0.0 0.0 0.0
 * // 0.0 1.0 0.0 0.0
 * // 0.0 0.0 1.0 0.0
 * // 0.0 0.0 0.0 1.0
 * @endcode
 */
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

#include <pybind11/pybind11.h>
#include <iomanip>

namespace py = pybind11;

/**
 * @brief Internal: Print Python value with key and indentation
 * @param key Key name (empty for list items)
 * @param value Python object handle
 * @param indent Indentation level (4 spaces per level)
 * @note Recursively handles dicts and lists
 */
static void PrintFormatted(const std::string &key, const py::handle &value, int indent = 0);

/**
 * @brief Print Python dictionary with indentation
 * @param dict Python dictionary to print
 * @param indent Indentation level (4 spaces per level)
 * @note Recursively prints nested dicts and lists
 */
static void PrintDict(const py::dict &dict, int indent)
{
    for (auto item : dict)
    {
        PrintFormatted(py::str(item.first).cast<std::string>(), item.second, indent);
    }
}

/**
 * @brief Print Python object's __dict__ attribute
 * @param obj Python object to inspect
 * @note Useful for debugging Python class instances
 *
 * Usage:
 * @code
 * py::object player = pythonModule.attr("Player")();
 * PrintPyObject(player);  // Prints all attributes
 * @endcode
 */
static void PrintPyObject(const py::object &obj)
{
    PrintDict(obj.attr("__dict__").cast<py::dict>(), 0);
}

/**
 * @brief Print Python list with indentation
 * @param list Python list to print
 * @param indent Indentation level (4 spaces per level)
 * @note Recursively prints nested structures
 */
static void PrintList(const py::list &list, int indent)
{
    for (auto item : list)
    {
        PrintFormatted("", item, indent);
    }
}

/**
 * @brief Print Python value with formatting (implementation)
 * @param key Key name (or empty string for list items)
 * @param value Python value (dict, list, or scalar)
 * @param indent Number of spaces to indent
 *
 * Output format:
 * - Dicts: "key : " followed by indented contents
 * - Lists: Indented items
 * - Scalars: "key : value"
 */
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