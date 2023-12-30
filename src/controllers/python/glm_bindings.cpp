#include <pybind11/pybind11.h>
#include <glm/glm.hpp>

#include "Tetris.h"

namespace py = pybind11;

PYBIND11_MODULE(glm, m)
{
    py::class_<glm::vec2>(m, "vec2")
        .def(py::init<float, float>())
        .def_readwrite("x", &glm::vec2::x)
        .def_readwrite("y", &glm::vec2::y);

    py::class_<glm::vec3>(m, "vec3")
        .def(py::init<float, float, float>())
        .def_readwrite("x", &glm::vec3::x)
        .def_readwrite("y", &glm::vec3::y)
        .def_readwrite("z", &glm::vec3::z);

    py::class_<glm::mat4>(m, "mat4")
        .def("get", &Tetris::mat4_get);
}