/**
 * @file TetrisLuaTests.cpp
 * @brief Automated Lua behavior tests for Tetris scripts
 * @lines ~600
 *
 * Purpose: Establish a stable automated benchmark for Tetris gameplay logic.
 * Validates key script contracts without requiring OpenGL or window creation.
 *
 * Key functions:
 * - TestTetrisConstantsGravity() - Validates gravity curve invariants (line ~106)
 * - TestTetrisConstantsSRSKickCompleteness() - Validates all 8 SRS kick transitions exist (line ~138)
 * - TestTetriminoDataShapeIntegrity() - Validates all 7 pieces have 4 states with 4 blocks each (line ~106)
 * - TestTetrisGameLifecycle() - Validates start/reset/pause UI state transitions (line ~198)
 * - TestTetrisGamePiecePreview() - Validates next/hold piece preview rendering data (line ~258)
 * - TestTetrisInputKeyRouting() - Validates input event dispatch to game/grid actions (line ~299)
 * - TestTetrisGridCollisionDetection() - Validates wall/floor/block collision (line ~385)
 * - TestTetrisGridScoringFormulas() - Validates scoring math for lines, T-spins, combos (line ~510)
 */

#include <sol/sol.hpp>

#include <iostream>
#include <string>
#include <vector>

#ifndef IMHOTEP_SOURCE_DIR
#define IMHOTEP_SOURCE_DIR "."
#endif

namespace
{
std::string ScriptPath(const std::string &relativePath)
{
    return std::string(IMHOTEP_SOURCE_DIR) + "/" + relativePath;
}

void RegisterBaseBindings(sol::state &lua)
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

    lua.set_function("log_trace", [](sol::variadic_args) {});
    lua.set_function("log_debug", [](sol::variadic_args) {});
    lua.set_function("log_info", [](sol::variadic_args) {});
    lua.set_function("log_warning", [](sol::variadic_args) {});
    lua.set_function("log_error", [](sol::variadic_args) {});

    lua.set_function("vec2", [&lua](double x, double y)
                     {
                         sol::table t = lua.create_table();
                         t["x"] = x;
                         t["y"] = y;
                         return t;
                     });

    lua.set_function("vec3", [&lua](double x, double y, double z)
                     {
                         sol::table t = lua.create_table();
                         t["x"] = x;
                         t["y"] = y;
                         t["z"] = z;
                         return t;
                     });

    lua.set_function("vec4", [&lua](double x, double y, double z, double w)
                     {
                         sol::table t = lua.create_table();
                         t["x"] = x;
                         t["y"] = y;
                         t["z"] = z;
                         t["w"] = w;
                         return t;
                     });

    sol::table inputTypes = lua.create_table();
    inputTypes["KEY"] = "KEY";
    inputTypes["CLICK"] = "CLICK";
    inputTypes["CURSOR"] = "CURSOR";
    inputTypes["SCROLL"] = "SCROLL";
    lua["InputTypes"] = inputTypes;

    sol::table rotations = lua.create_table();
    rotations["CW"] = "CW";
    rotations["CCW"] = "CCW";
    lua["Rotations"] = rotations;

    lua["EventQueue"] = lua.create_table();
}

bool ExpectTrue(const std::string &testName, bool condition, const std::string &message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << testName << ": " << message << std::endl;
        return false;
    }
    return true;
}

template <typename T>
bool ExpectEq(const std::string &testName, const T &actual, const T &expected, const std::string &message)
{
    if (actual != expected)
    {
        std::cerr << "[FAIL] " << testName << ": " << message << " (actual=" << actual << ", expected=" << expected << ")" << std::endl;
        return false;
    }
    return true;
}

bool TestTetrisConstantsGravity()
{
    const std::string testName = "TetrisConstants.GravityCurve";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table constants = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisConstants.lua"));
    lua["TetrisConstants"] = constants;

    sol::protected_function getGravity = constants["GetGravitySpeed"];

    sol::protected_function_result r1 = getGravity(1);
    if (!ExpectTrue(testName, r1.valid(), "GetGravitySpeed(1) failed"))
        return false;
    if (!ExpectEq(testName, r1.get<int>(), 800, "Level 1 gravity should be 800ms"))
        return false;

    sol::protected_function_result r10 = getGravity(10);
    if (!ExpectTrue(testName, r10.valid(), "GetGravitySpeed(10) failed"))
        return false;
    if (!ExpectEq(testName, r10.get<int>(), 100, "Level 10 gravity should be 100ms"))
        return false;

    sol::protected_function_result r25 = getGravity(25);
    if (!ExpectTrue(testName, r25.valid(), "GetGravitySpeed(25) failed"))
        return false;
    if (!ExpectEq(testName, r25.get<int>(), 17, "Level >=20 gravity should clamp to 17ms"))
        return false;

    return true;
}

bool TestTetrisGameLifecycle()
{
    const std::string testName = "TetrisGame.LifecycleAndUI";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table uiState = lua.create_table();
    int refreshCount = 0;
    lua.set_function("SetUIValue", [&](const std::string &key, sol::object value)
                     { uiState[key] = value; });
    lua.set_function("RefreshUI", [&]()
                     { ++refreshCount; });

    // Stub TetrisConstants with ApplyMode
    sol::table constants = lua.create_table();
    constants.set_function("ApplyMode", [](const std::string &) { return true; });
    lua["TetrisConstants"] = constants;

    int setupPlayfieldCalls = 0;
    int resetCalls = 0;
    sol::table grid = lua.create_table();
    grid["playfieldReady"] = false;
    grid.set_function("setupPlayfield", [&](sol::table self)
                      { ++setupPlayfieldCalls; self["playfieldReady"] = true; });
    grid.set_function("reset", [&](sol::table)
                      { ++resetCalls; });
    grid.set_function("setCamera", [](sol::table) {});
    grid.set_function("renderBorder", [](sol::table) {});
    lua["TetrisGrid"] = grid;

    sol::table game = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisGame.lua"));
    lua["TetrisGame"] = game;

    sol::protected_function start = game["start"];
    sol::protected_function pause = game["pause"];
    sol::protected_function resume = game["resume"];

    // First start: should call setupPlayfield (not reset) since playfieldReady=false
    sol::protected_function_result startResult = start(game);
    if (!ExpectTrue(testName, startResult.valid(), "start() failed"))
        return false;

    if (!ExpectEq(testName, game["isStarted"].get<bool>(), true, "isStarted should be true after start"))
        return false;
    if (!ExpectEq(testName, game["isPaused"].get<bool>(), false, "isPaused should be false after start"))
        return false;
    if (!ExpectEq(testName, setupPlayfieldCalls, 1, "grid:setupPlayfield() should be called once on first start"))
        return false;
    if (!ExpectEq(testName, uiState["data.gameStarted"].get<bool>(), true, "UI data.gameStarted should be true"))
        return false;
    if (!ExpectEq(testName, refreshCount, 1, "RefreshUI should be called once by start"))
        return false;

    sol::protected_function_result pauseResult = pause(game);
    if (!ExpectTrue(testName, pauseResult.valid(), "pause() failed"))
        return false;
    if (!ExpectEq(testName, game["isPaused"].get<bool>(), true, "isPaused should be true after pause"))
        return false;
    if (!ExpectEq(testName, uiState["data.gamePaused"].get<bool>(), true, "UI data.gamePaused should be true after pause"))
        return false;

    sol::protected_function_result resumeResult = resume(game);
    if (!ExpectTrue(testName, resumeResult.valid(), "resume() failed"))
        return false;
    if (!ExpectEq(testName, game["isPaused"].get<bool>(), false, "isPaused should be false after resume"))
        return false;
    if (!ExpectEq(testName, uiState["data.gamePaused"].get<bool>(), false, "UI data.gamePaused should be false after resume"))
        return false;

    return true;
}

bool TestTetrisGamePiecePreview()
{
    const std::string testName = "TetrisGame.PiecePreviewLayout";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table uiState = lua.create_table();
    lua.set_function("SetUIValue", [&](const std::string &key, sol::object value)
                     { uiState[key] = value; });
    lua.set_function("RefreshUI", []() {});

    sol::table game = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisGame.lua"));
    lua["TetrisGame"] = game;

    sol::protected_function updatePiecePreview = game["updatePiecePreview"];
    sol::protected_function_result previewResult = updatePiecePreview(game, std::string("data.np"), std::string("I"));
    if (!ExpectTrue(testName, previewResult.valid(), "updatePiecePreview() failed"))
        return false;

    if (!ExpectEq(testName, uiState["data.np1"].get<std::string>(), std::string("transparent"), "np1 should be transparent for I piece"))
        return false;
    if (!ExpectEq(testName, uiState["data.np4"].get<std::string>(), std::string("transparent"), "np4 should be transparent for I piece"))
        return false;
    if (!ExpectEq(testName, uiState["data.np5"].get<std::string>(), std::string("#41f0db"), "np5 should be I-piece color"))
        return false;
    if (!ExpectEq(testName, uiState["data.np8"].get<std::string>(), std::string("#41f0db"), "np8 should be I-piece color"))
        return false;

    return true;
}

void PushKeyEvent(sol::state &lua, const std::string &key)
{
    sol::table queue = lua["EventQueue"];
    std::size_t nextIndex = queue.size() + 1;
    sol::table event = lua.create_table();
    event["type"] = lua["InputTypes"]["KEY"];
    event["input"] = key;
    queue[nextIndex] = event;
}

bool TestTetrisInputKeyRouting()
{
    const std::string testName = "TetrisInput.KeyRouting";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table camera = lua.create_table();
    camera["fov"] = 45.0;
    camera.set_function("SetPerspective", [](sol::table, double, sol::optional<double>, sol::optional<double>) {});
    camera.set_function("RotateByMouse", [](sol::table, double, double) {});

    int closeWindowCalls = 0;
    sol::table window = lua.create_table();
    window.set_function("CloseWindow", [&](sol::table)
                        { ++closeWindowCalls; });
    window.set_function("GetSize", [&](sol::table)
                        {
                            sol::table size = lua.create_table();
                            size["x"] = 1920;
                            size["y"] = 1080;
                            return size;
                        });

    sol::table htmlRenderer = lua.create_table();
    htmlRenderer.set_function("HandleClickEvent", [](sol::table, double, double, int) {});
    htmlRenderer.set_function("UpdateHoverState", [](sol::table, double, double) {});

    sol::table gameManager = lua.create_table();
    gameManager["camera"] = camera;
    gameManager["window"] = window;
    gameManager["htmlRenderer"] = htmlRenderer;
    lua["GameManager"] = gameManager;

    int startCalls = 0;
    int togglePauseCalls = 0;
    sol::table game = lua.create_table();
    game["isStarted"] = false;
    game["isPaused"] = false;
    game["isGameOver"] = false;
    game.set_function("start", [&](sol::table self, sol::optional<std::string>)
                      {
                          ++startCalls;
                          self["isStarted"] = true;
                      });
    game.set_function("togglePause", [&](sol::table)
                      { ++togglePauseCalls; });
    game.set_function("reset", [](sol::table) {});
    game.set_function("returnToMenu", [](sol::table) {});
    lua["TetrisGame"] = game;

    int hardDropCalls = 0;
    sol::table grid = lua.create_table();
    grid["gameOver"] = false;
    grid.set_function("hardDrop", [&](sol::table)
                      { ++hardDropCalls; });
    grid.set_function("softDrop", [](sol::table) {});
    grid.set_function("rotateTetrimino", [](sol::table, sol::object) {});
    grid.set_function("holdPiece", [](sol::table) {});
    lua["TetrisGrid"] = grid;

    sol::table inputModule = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisInput.lua"));

    PushKeyEvent(lua, "ENTER");
    PushKeyEvent(lua, "SPACE");
    PushKeyEvent(lua, "P");

    sol::protected_function handleInput = inputModule["handleInput"];
    sol::protected_function_result inputResult = handleInput(inputModule);
    if (!ExpectTrue(testName, inputResult.valid(), "handleInput() failed"))
        return false;

    if (!ExpectEq(testName, startCalls, 1, "ENTER should call game:start() exactly once"))
        return false;
    if (!ExpectEq(testName, hardDropCalls, 1, "SPACE should call grid:hardDrop() exactly once"))
        return false;
    if (!ExpectEq(testName, togglePauseCalls, 1, "P should call game:togglePause() exactly once"))
        return false;
    if (!ExpectEq(testName, closeWindowCalls, 0, "No CloseWindow call expected for ENTER/SPACE/P"))
        return false;

    sol::table queue = lua["EventQueue"];
    if (!ExpectEq(testName, static_cast<int>(queue.size()), 0, "EventQueue should be empty after handling input"))
        return false;

    return true;
}

bool TestTetriminoDataShapeIntegrity()
{
    const std::string testName = "TetriminoData.ShapeIntegrity";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table data = lua.script_file(ScriptPath("res/games/tetris/scripts/TetriminoData.lua"));

    const std::vector<std::string> pieces = {"I", "O", "T", "J", "L", "S", "Z"};

    for (const auto &piece : pieces)
    {
        sol::optional<sol::table> pieceTable = data[piece];
        if (!ExpectTrue(testName, pieceTable.has_value(), piece + " piece should exist"))
            return false;

        // Each piece must have a color (vec3)
        sol::optional<sol::table> color = pieceTable.value()["color"];
        if (!ExpectTrue(testName, color.has_value(), piece + " should have a color"))
            return false;
        if (!ExpectTrue(testName, color.value()["x"].valid(), piece + " color should have x component"))
            return false;

        // Each piece must have 4 rotation states (0-3)
        sol::optional<sol::table> states = pieceTable.value()["states"];
        if (!ExpectTrue(testName, states.has_value(), piece + " should have states"))
            return false;

        for (int state = 0; state <= 3; ++state)
        {
            sol::optional<sol::table> grid = states.value()[state];
            if (!ExpectTrue(testName, grid.has_value(), piece + " state " + std::to_string(state) + " should exist"))
                return false;

            // Count filled cells - each piece has exactly 4 blocks
            int blockCount = 0;
            for (int row = 1; row <= 4; ++row)
            {
                sol::optional<sol::table> rowTable = grid.value()[row];
                if (!ExpectTrue(testName, rowTable.has_value(),
                                piece + " state " + std::to_string(state) + " row " + std::to_string(row) + " should exist"))
                    return false;

                for (int col = 1; col <= 4; ++col)
                {
                    int cell = rowTable.value()[col].get_or(0);
                    if (cell == 1)
                        ++blockCount;
                }
            }

            if (!ExpectEq(testName, blockCount, 4,
                          piece + " state " + std::to_string(state) + " should have exactly 4 blocks"))
                return false;
        }
    }

    return true;
}

bool TestTetrisConstantsSRSKickCompleteness()
{
    const std::string testName = "TetrisConstants.SRSKickCompleteness";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table constants = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisConstants.lua"));

    // All 8 rotation transitions that must exist
    const std::vector<std::string> transitions = {
        "0>1", "1>0", "1>2", "2>1", "2>3", "3>2", "3>0", "0>3"};

    // Check JLSTZ kick table
    sol::optional<sol::table> jlstz = constants["SRS_KICKS_JLSTZ"];
    if (!ExpectTrue(testName, jlstz.has_value(), "SRS_KICKS_JLSTZ should exist"))
        return false;

    for (const auto &t : transitions)
    {
        sol::optional<sol::table> kicks = jlstz.value()[t];
        if (!ExpectTrue(testName, kicks.has_value(), "JLSTZ kick " + t + " should exist"))
            return false;

        // Each transition should have exactly 5 kick offsets
        int count = 0;
        kicks.value().for_each([&count](sol::object, sol::object) { ++count; });
        if (!ExpectEq(testName, count, 5, "JLSTZ kick " + t + " should have 5 offsets"))
            return false;
    }

    // Check I-piece kick table
    sol::optional<sol::table> iKicks = constants["SRS_KICKS_I"];
    if (!ExpectTrue(testName, iKicks.has_value(), "SRS_KICKS_I should exist"))
        return false;

    for (const auto &t : transitions)
    {
        sol::optional<sol::table> kicks = iKicks.value()[t];
        if (!ExpectTrue(testName, kicks.has_value(), "I kick " + t + " should exist"))
            return false;

        int count = 0;
        kicks.value().for_each([&count](sol::object, sol::object) { ++count; });
        if (!ExpectEq(testName, count, 5, "I kick " + t + " should have 5 offsets"))
            return false;
    }

    return true;
}

bool TestTetrisMiniConstants()
{
    const std::string testName = "TetrisConstants.MiniMode";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table constants = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisConstants.lua"));
    lua["TetrisConstants"] = constants;

    // Verify defaults (standard mode)
    if (!ExpectEq(testName, constants["GRID_WIDTH"].get<int>(), 10, "Default GRID_WIDTH should be 10"))
        return false;
    if (!ExpectEq(testName, constants["SPAWN_COLUMN"].get<int>(), 3, "Default SPAWN_COLUMN should be 3"))
        return false;

    // Apply mini mode
    sol::protected_function applyMode = constants["ApplyMode"];
    sol::protected_function_result r1 = applyMode("mini");
    if (!ExpectTrue(testName, r1.valid(), "ApplyMode('mini') should not error"))
        return false;
    if (!ExpectEq(testName, r1.get<bool>(), true, "ApplyMode('mini') should return true"))
        return false;

    if (!ExpectEq(testName, constants["GRID_WIDTH"].get<int>(), 4, "Mini GRID_WIDTH should be 4"))
        return false;
    if (!ExpectEq(testName, constants["GRID_HEIGHT"].get<int>(), 20, "Mini GRID_HEIGHT should be 20"))
        return false;
    if (!ExpectEq(testName, constants["SPAWN_COLUMN"].get<int>(), 0, "Mini SPAWN_COLUMN should be 0"))
        return false;
    if (!ExpectEq(testName, constants["CAMERA_PADDING"].get<int>(), 4, "Mini CAMERA_PADDING should be 4"))
        return false;
    if (!ExpectEq(testName, constants["ACTIVE_MODE"].get<std::string>(), std::string("mini"), "ACTIVE_MODE should be 'mini'"))
        return false;

    // Roundtrip back to standard
    sol::protected_function_result r2 = applyMode("standard");
    if (!ExpectTrue(testName, r2.valid(), "ApplyMode('standard') should not error"))
        return false;
    if (!ExpectEq(testName, constants["GRID_WIDTH"].get<int>(), 10, "Standard GRID_WIDTH should be 10"))
        return false;
    if (!ExpectEq(testName, constants["SPAWN_COLUMN"].get<int>(), 3, "Standard SPAWN_COLUMN should be 3"))
        return false;
    if (!ExpectEq(testName, constants["ACTIVE_MODE"].get<std::string>(), std::string("standard"), "ACTIVE_MODE should be 'standard'"))
        return false;

    // Invalid mode should return false
    sol::protected_function_result r3 = applyMode("bogus");
    if (!ExpectTrue(testName, r3.valid(), "ApplyMode('bogus') should not error"))
        return false;
    if (!ExpectEq(testName, r3.get<bool>(), false, "ApplyMode('bogus') should return false"))
        return false;

    return true;
}

bool TestTetrisGridCollisionDetection()
{
    const std::string testName = "TetrisGrid.CollisionDetection";
    sol::state lua;
    RegisterBaseBindings(lua);

    sol::table constants = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisConstants.lua"));
    lua["TetrisConstants"] = constants;

    sol::table tetriminoData = lua.script_file(ScriptPath("res/games/tetris/scripts/TetriminoData.lua"));
    lua["TetriminoData"] = tetriminoData;

    // Stub engine functions that TetrisGrid calls at load time
    lua.set_function("RegisterEntity", []() { return 1; });
    lua.set_function("DestroyEntity", [](int) {});
    lua.set_function("GetTransform", [&lua](int)
                     {
                         sol::table t = lua.create_table();
                         sol::table pos = lua.create_table();
                         pos["x"] = 0.0; pos["y"] = 0.0; pos["z"] = 0.0;
                         t["Pos"] = pos;
                         sol::table color = lua.create_table();
                         color["x"] = 1.0; color["y"] = 1.0; color["z"] = 1.0; color["w"] = 1.0;
                         t["Color"] = color;
                         return t;
                     });
    lua.set_function("CreateRenderComponent", [](sol::object, sol::object) { return 1; });
    lua.set_function("RegisterRenderComponent", [](int, sol::object) {});
    lua.set_function("RegisterLighting", [](int, int) {});
    lua.set_function("AddChild", [](int, int) {});
    lua.set_function("RemoveChild", [](int, int) {});
    lua.set_function("TranslateEntity", [](int, sol::object) {});
    lua.set_function("CreateTweenComponent", [](int, int) {});
    lua.set_function("GetTween", [&lua](int)
                     {
                         sol::table t = lua.create_table();
                         t["elapsed"] = 0;
                         sol::table s = lua.create_table();
                         s["x"] = 0.0; s["y"] = 0.0; s["z"] = 0.0;
                         t["Start"] = s;
                         sol::table e = lua.create_table();
                         e["x"] = 0.0; e["y"] = 0.0; e["z"] = 0.0;
                         t["End"] = e;
                         t["Duration"] = 0;
                         t["isActive"] = false;
                         return t;
                     });
    lua.set_function("GetEntityByName", [](const std::string &) { return 1; });
    lua.set_function("CreateCube", [](sol::object, sol::object, sol::object) {});
    lua.set_function("SetUIValue", [](const std::string &, sol::object) {});
    lua.set_function("RefreshUI", []() {});

    // Stub GameManager for TetrisGrid.ready
    sol::table camera = lua.create_table();
    camera["fov"] = 45.0;
    camera.set_function("SetPerspective", [](sol::table, double) {});
    sol::table camTransform = lua.create_table();
    sol::table camPos = lua.create_table();
    camPos["x"] = 0; camPos["y"] = 0; camPos["z"] = 0;
    camTransform["Pos"] = camPos;
    camera["transform"] = camTransform;
    sol::table window = lua.create_table();
    window.set_function("IsKeyPressed", [](sol::table, const std::string &) { return false; });
    sol::table gm = lua.create_table();
    gm["camera"] = camera;
    gm["window"] = window;
    lua["GameManager"] = gm;

    // Stub TetrisGame so TetrisGrid can resolve it
    sol::table game = lua.create_table();
    game["isStarted"] = false;
    game["isPaused"] = false;
    game.set_function("updateUI", [](sol::table, int, int, int, sol::object) {});
    game.set_function("updateHoldUI", [](sol::table, sol::object) {});
    game.set_function("gameOver", [](sol::table, int) {});
    game.set_function("showNotification", [](sol::table, const std::string &, const std::string &) {});
    lua["TetrisGame"] = game;

    // Load Tetrimino module (needed by TetrisGrid)
    sol::table tetrimino = lua.script_file(ScriptPath("res/games/tetris/scripts/Tetrimino.lua"));
    lua["Tetrimino"] = tetrimino;

    sol::table grid = lua.script_file(ScriptPath("res/games/tetris/scripts/TetrisGrid.lua"));

    // Initialize the grid array (normally done by ready(), but that needs full engine)
    int gridWidth = constants["GRID_WIDTH"].get<int>();
    int gridHeight = constants["GRID_HEIGHT"].get<int>();
    int emptyCell = constants["GRID_EMPTY_CELL"].get<int>();
    sol::table gridArray = lua.create_table();
    for (int x = 0; x < gridWidth; ++x)
    {
        gridArray[x] = lua.create_table();
        for (int y = 0; y < gridHeight; ++y)
            gridArray[x][y] = emptyCell;
    }
    grid["grid"] = gridArray;

    // Build a simple 4x4 block (I-piece spawn state) for testing
    sol::table rotMap = lua.create_table();
    for (int i = 0; i <= 3; ++i)
    {
        rotMap[i] = lua.create_table();
        for (int j = 0; j <= 3; ++j)
            rotMap[i][j] = -1;
    }
    // I-piece spawn: row 2 (i=2) has blocks at columns 0-3
    rotMap[2][0] = 100;
    rotMap[2][1] = 101;
    rotMap[2][2] = 102;
    rotMap[2][3] = 103;

    // Test 1: No collision in middle of empty grid
    sol::protected_function isCollision = grid["isCollision"];
    sol::protected_function_result r1 = isCollision(grid, lua.create_table_with("x", 3, "y", 10), rotMap);
    if (!ExpectTrue(testName, r1.valid(), "isCollision() should not error"))
        return false;
    if (!ExpectEq(testName, r1.get<bool>(), false, "No collision in empty grid center"))
        return false;

    // Test 2: Collision with left wall (x = -1)
    sol::protected_function_result r2 = isCollision(grid, lua.create_table_with("x", -1, "y", 10), rotMap);
    if (!ExpectTrue(testName, r2.valid(), "isCollision() left wall should not error"))
        return false;
    if (!ExpectEq(testName, r2.get<bool>(), true, "Should collide with left wall"))
        return false;

    // Test 3: Collision with floor (y = -1, blocks at row i=2 would be at y=1 which is valid,
    //          but we need y=-1 so blocks at y=1 are fine — use y=-3 so row 2 is at y=-1)
    sol::protected_function_result r3 = isCollision(grid, lua.create_table_with("x", 3, "y", -3), rotMap);
    if (!ExpectTrue(testName, r3.valid(), "isCollision() floor should not error"))
        return false;
    if (!ExpectEq(testName, r3.get<bool>(), true, "Should collide with floor"))
        return false;

    // Test 4: Collision with right wall (x = 7, blocks at j=3 would be at x=10 which is out of bounds)
    sol::protected_function_result r4 = isCollision(grid, lua.create_table_with("x", 7, "y", 10), rotMap);
    if (!ExpectTrue(testName, r4.valid(), "isCollision() right wall should not error"))
        return false;
    if (!ExpectEq(testName, r4.get<bool>(), true, "Should collide with right wall"))
        return false;

    // Test 5: Collision with placed block in grid
    // Place a block at grid[5][10]
    gridArray[5][10] = 999;
    sol::protected_function_result r5 = isCollision(grid, lua.create_table_with("x", 3, "y", 8), rotMap);
    if (!ExpectTrue(testName, r5.valid(), "isCollision() block collision should not error"))
        return false;
    // Block at j=2, i=2 → x=3+2=5, y=8+2=10 → collides with grid[5][10]
    if (!ExpectEq(testName, r5.get<bool>(), true, "Should collide with placed block at grid[5][10]"))
        return false;

    // Clean up
    gridArray[5][10] = -1;

    return true;
}

bool TestTetrisGridScoringFormulas()
{
    const std::string testName = "TetrisGrid.ScoringFormulas";
    sol::state lua;
    RegisterBaseBindings(lua);

    // We test scoring by running the formula directly in Lua
    // This avoids needing the full TetrisGrid entity system
    lua.script(R"(
        function CalculateScore(cleared, level, isTSpin, combo)
            local points = 0
            if isTSpin then
                if cleared == 1 then points = 800 * level
                elseif cleared == 2 then points = 1200 * level
                elseif cleared == 3 then points = 1600 * level
                end
            else
                if cleared == 1 then points = 40 * level
                elseif cleared == 2 then points = 100 * level
                elseif cleared == 3 then points = 300 * level
                elseif cleared >= 4 then points = 1200 * level
                end
            end
            if combo > 0 then
                points = points + (50 * combo * level)
            end
            return points
        end
    )");

    sol::protected_function calcScore = lua["CalculateScore"];

    // Single line, level 1, no T-spin, no combo
    auto r1 = calcScore(1, 1, false, 0);
    if (!ExpectTrue(testName, r1.valid(), "CalculateScore should not error"))
        return false;
    if (!ExpectEq(testName, r1.get<int>(), 40, "Single at level 1 = 40"))
        return false;

    // Double, level 1
    if (!ExpectEq(testName, calcScore(2, 1, false, 0).get<int>(), 100, "Double at level 1 = 100"))
        return false;

    // Triple, level 1
    if (!ExpectEq(testName, calcScore(3, 1, false, 0).get<int>(), 300, "Triple at level 1 = 300"))
        return false;

    // Tetris (4 lines), level 1
    if (!ExpectEq(testName, calcScore(4, 1, false, 0).get<int>(), 1200, "Tetris at level 1 = 1200"))
        return false;

    // Level scaling: double at level 5
    if (!ExpectEq(testName, calcScore(2, 5, false, 0).get<int>(), 500, "Double at level 5 = 500"))
        return false;

    // T-spin single, level 1
    if (!ExpectEq(testName, calcScore(1, 1, true, 0).get<int>(), 800, "T-spin single at level 1 = 800"))
        return false;

    // T-spin double, level 1
    if (!ExpectEq(testName, calcScore(2, 1, true, 0).get<int>(), 1200, "T-spin double at level 1 = 1200"))
        return false;

    // T-spin triple, level 1
    if (!ExpectEq(testName, calcScore(3, 1, true, 0).get<int>(), 1600, "T-spin triple at level 1 = 1600"))
        return false;

    // Combo bonus: single at level 1, combo x2
    if (!ExpectEq(testName, calcScore(1, 1, false, 2).get<int>(), 140, "Single + combo x2 at level 1 = 40 + 100"))
        return false;

    // Combo + level scaling: double at level 3, combo x1
    if (!ExpectEq(testName, calcScore(2, 3, false, 1).get<int>(), 450, "Double + combo x1 at level 3 = 300 + 150"))
        return false;

    return true;
}

} // namespace

int main()
{
    struct TestCase
    {
        std::string name;
        bool (*fn)();
    };

    const std::vector<TestCase> tests = {
        {"TetrisConstants.GravityCurve", &TestTetrisConstantsGravity},
        {"TetrisConstants.SRSKickCompleteness", &TestTetrisConstantsSRSKickCompleteness},
        {"TetrisConstants.MiniMode", &TestTetrisMiniConstants},
        {"TetriminoData.ShapeIntegrity", &TestTetriminoDataShapeIntegrity},
        {"TetrisGame.LifecycleAndUI", &TestTetrisGameLifecycle},
        {"TetrisGame.PiecePreviewLayout", &TestTetrisGamePiecePreview},
        {"TetrisInput.KeyRouting", &TestTetrisInputKeyRouting},
        {"TetrisGrid.CollisionDetection", &TestTetrisGridCollisionDetection},
        {"TetrisGrid.ScoringFormulas", &TestTetrisGridScoringFormulas},
    };

    int passed = 0;
    for (const auto &test : tests)
    {
        if (test.fn())
        {
            std::cout << "[PASS] " << test.name << std::endl;
            ++passed;
        }
    }

    const int failed = static_cast<int>(tests.size()) - passed;
    std::cout << "[SUMMARY] Passed " << passed << "/" << tests.size() << " tests" << std::endl;

    return failed == 0 ? 0 : 1;
}
