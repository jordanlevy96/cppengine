#include <pybind11/pybind11.h>

#include "controllers/App.h"

namespace py = pybind11;

PYBIND11_MODULE(app_module, m)
{
    py::class_<App>(m, "App")
        .def_static("GetInstance", &App::GetInstance, py::return_value_policy::reference)
        .def_readwrite("delta", &App::delta)
        .def_readwrite("camera", &App::cam)
        .def_readwrite("conf", &App::conf)
        .def_readwrite("registry", &App::registry)
        .def_readwrite("window", &App::windowManager);

    py::class_<Config>(m, "Config")
        .def_readwrite("resPath", &Config::ResourcePath);

    py::class_<Registry>(m, "Registry")
        .def_static("GetInstance", &Registry::GetInstance)
        .def("GetEntityByName", &Registry::GetEntityByName);

    py::class_<WindowManager>(m, "Window")
        .def("CloseWindow", &WindowManager::CloseWindow)
        .def("GetSize", &WindowManager::GetSize);
}
