#include <pybind11/pybind11.h>

#include "Camera.h"

namespace py = pybind11;

PYBIND11_MODULE(camera, m)
{
    py::class_<Camera>(m, "Camera")
        .def_readwrite("transform", &Camera::transform)
        .def_readwrite("fov", &Camera::fov)
        .def_readwrite("front", &Camera::front)
        .def("SetPerspective", static_cast<void (Camera::*)(float)>(&Camera::SetPerspective))
        .def("Move", &Camera::Move)
        .def("RotateByMouse", &Camera::RotateByMouse);
}