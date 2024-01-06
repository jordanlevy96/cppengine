#include <pybind11/pybind11.h>

#include "controllers/App.h"

namespace py = pybind11;

PYBIND11_MODULE(app_module, m)
{
    py::class_<App>(m, "App")
        .def_static(
            "test", []()
            { 
                App &app = App::GetInstance();
                return app.test; },
            py::return_value_policy::reference)
        .def_readwrite("delta", &App::delta)
        .def_readwrite("camera", &App::cam)
        .def_readwrite("conf", &App::conf);

    py::class_<Config>(m, "Config")
        .def_readwrite("resPath", &Config::ResourcePath);

    py::class_<Registry>(m, "Registry")
        .def_static("GetInstance", &Registry::GetInstance, py::return_value_policy::reference)
        .def("GetEntityByName", &Registry::GetEntityByName);

    py::class_<WindowManager, std::unique_ptr<WindowManager, py::nodelete>>(m, "Window")
        .def_property_readonly_static(
            "instance", [](py::object /* self */)
            { return &WindowManager::GetInstance(); },
            py::return_value_policy::reference)
        .def("GetSize", &WindowManager::GetSize)
        .def("CloseWindow", &WindowManager::CloseWindow);

    py::class_<InputEvent>(m, "InputEvent")
        .def(py::init<>())
        .def_readwrite("type", &InputEvent::type)
        .def_property(
            "input",
            [](const InputEvent &event) -> py::object
            {
                // Getter: convert std::variant to Python object
                if (std::holds_alternative<std::string>(event.input))
                {
                    return py::cast(std::get<std::string>(event.input));
                }
                else if (std::holds_alternative<glm::vec2>(event.input))
                {
                    return py::cast(std::get<glm::vec2>(event.input));
                }
                return py::none();
            },
            [](InputEvent &event, py::object obj)
            {
                // Setter: convert Python object to std::variant
                if (py::isinstance<py::str>(obj))
                {
                    event.input = obj.cast<std::string>();
                }
                else if (py::isinstance<py::tuple>(obj) || py::isinstance<py::list>(obj))
                {
                    event.input = obj.cast<glm::vec2>();
                }
                else
                {
                    throw std::runtime_error("Invalid type for input");
                }
            });

    py::class_<glm::vec2>(m, "vec2") // return type of Window.GetSize()
        .def(py::init<float, float>())
        .def_readwrite("x", &glm::vec2::x)
        .def_readwrite("y", &glm::vec2::y);
}
