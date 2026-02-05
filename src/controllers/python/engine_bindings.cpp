#include <pybind11/pybind11.h>
#include <glm/glm.hpp>

#include "controllers/Registry.h"
#include "controllers/Game.h"
#include "components/Transform.h"
#include "util/PathResolver.h"

namespace py = pybind11;

PYBIND11_MODULE(engine, m)
{
    m.doc() = "Minimal C++ engine bindings for data export and analysis";

    // ========================================================================
    // GLM Types - for interfacing with numpy
    // ========================================================================

    py::class_<glm::vec2>(m, "vec2")
        .def(py::init<>())
        .def(py::init<float, float>())
        .def_readwrite("x", &glm::vec2::x)
        .def_readwrite("y", &glm::vec2::y)
        .def("__repr__", [](const glm::vec2 &v) {
            return "vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
        });

    py::class_<glm::vec3>(m, "vec3")
        .def(py::init<>())
        .def(py::init<float, float, float>())
        .def_readwrite("x", &glm::vec3::x)
        .def_readwrite("y", &glm::vec3::y)
        .def_readwrite("z", &glm::vec3::z)
        .def("__repr__", [](const glm::vec3 &v) {
            return "vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
                   std::to_string(v.z) + ")";
        });

    py::class_<glm::vec4>(m, "vec4")
        .def(py::init<>())
        .def(py::init<float, float, float, float>())
        .def_readwrite("x", &glm::vec4::x)
        .def_readwrite("y", &glm::vec4::y)
        .def_readwrite("z", &glm::vec4::z)
        .def_readwrite("w", &glm::vec4::w)
        .def("__repr__", [](const glm::vec4 &v) {
            return "vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " +
                   std::to_string(v.z) + ", " + std::to_string(v.w) + ")";
        });

    // ========================================================================
    // Entity & Component Types - for querying game state
    // ========================================================================

    py::class_<Transform>(m, "Transform")
        .def(py::init<>())
        .def_readwrite("Pos", &Transform::Pos)
        .def_readwrite("Color", &Transform::Color)
        .def_readwrite("Scale", &Transform::Scale)
        .def_readwrite("Rotation", &Transform::Rotation);

    // ========================================================================
    // Registry - for accessing entities and components
    // ========================================================================

    py::class_<Registry>(m, "Registry")
        .def_static("GetInstance", &Registry::GetInstance, py::return_value_policy::reference)
        .def("GetEntityByName", &Registry::GetEntityByName)
        .def("GetTransform", [](Registry &self, EntityID id) -> Transform& {
            return self.GetComponent<Transform>(id);
        }, py::return_value_policy::reference);

    // ========================================================================
    // Path and Environment Utilities
    // ========================================================================

    m.def("getResourcePath", []() -> std::string {
        return Game::GetInstance().conf.ResourcePath;
    }, "Get the engine's resource path");

    m.def("isInstalledBundle", []() -> bool {
        return PathResolver::IsInstalledBundle();
    }, "Check if running from installed app bundle");

    m.def("getExecutableDir", []() -> std::string {
        return PathResolver::GetExecutableDir();
    }, "Get the directory containing the executable");
}
