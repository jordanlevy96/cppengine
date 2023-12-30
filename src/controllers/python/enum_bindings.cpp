#include <pybind11/pybind11.h>

#include "Camera.h"
#include "Tetris.h"
#include "controllers/WindowManager.h"

namespace py = pybind11;

PYBIND11_MODULE(enums, m)
{
    py::enum_<InputTypes>(m, "InputTypes")
        .value("KEY", InputTypes::Key)
        .value("CLICK", InputTypes::Click)
        .value("CURSOR", InputTypes::Cursor)
        .value("RESIZE", InputTypes::Resize)
        .value("SCROLL", InputTypes::Scroll)
        .export_values();

    py::enum_<CameraDirections>(m, "CameraDirections")
        .value("FORWARD", CameraDirections::FORWARD)
        .value("BACK", CameraDirections::BACK)
        .value("LEFT", CameraDirections::LEFT)
        .value("RIGHT", CameraDirections::RIGHT)
        .export_values();

    py::enum_<Tetris::Rotations>(m, "Rotations")
        .value("CW", Tetris::Rotations::CW)
        .value("CCW", Tetris::Rotations::CCW)
        .export_values();
}