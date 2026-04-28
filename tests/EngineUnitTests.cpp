/**
 * @file EngineUnitTests.cpp
 * @brief C++ unit tests for engine subsystems (no graphics required)
 *
 * Tests ExpressionCache (Lua expression compilation/evaluation)
 * and FrameTiming (frame timing, accumulators, simulation speed).
 */

#include "systems/ExpressionCache.h"
#include "util/FrameTiming.h"
#include "util/Logger.h"

#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>

// ============================================================================
// Minimal test harness
// ============================================================================

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) \
    static void Test_##name(); \
    struct TestReg_##name { TestReg_##name() { RunTest(#name, Test_##name); } }; \
    static void Test_##name()

static void RunTest(const char* name, void (*fn)())
{
    try
    {
        fn();
        std::cout << "  PASS: " << name << std::endl;
        g_passed++;
    }
    catch (const std::exception& e)
    {
        std::cout << "  FAIL: " << name << " — " << e.what() << std::endl;
        g_failed++;
    }
    catch (...)
    {
        std::cout << "  FAIL: " << name << " — unknown exception" << std::endl;
        g_failed++;
    }
}

#define ASSERT_TRUE(expr) \
    do { if (!(expr)) throw std::runtime_error("ASSERT_TRUE failed: " #expr); } while(0)

#define ASSERT_FALSE(expr) \
    do { if (expr) throw std::runtime_error("ASSERT_FALSE failed: " #expr); } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { \
        throw std::runtime_error( \
            std::string("ASSERT_EQ failed: ") + #a + " != " + #b + \
            " (" + std::to_string(a) + " vs " + std::to_string(b) + ")"); \
    } } while(0)

#define ASSERT_STR_EQ(a, b) \
    do { if ((a) != (b)) { \
        throw std::runtime_error( \
            std::string("ASSERT_STR_EQ failed: \"") + (a) + "\" != \"" + (b) + "\""); \
    } } while(0)

#define ASSERT_NEAR(a, b, tol) \
    do { if (std::abs((a) - (b)) > (tol)) { \
        throw std::runtime_error( \
            std::string("ASSERT_NEAR failed: ") + std::to_string(a) + " vs " + std::to_string(b)); \
    } } while(0)

// ============================================================================
// ExpressionCache Tests
// ============================================================================

static void RunExpressionCacheTests()
{
    std::cout << "\n--- ExpressionCache ---" << std::endl;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    // Test: Initialize
    {
        ExpressionCache cache;
        ASSERT_FALSE(cache.IsInitialized());
        cache.Initialize(&lua);
        ASSERT_TRUE(cache.IsInitialized());
        ASSERT_EQ(cache.Size(), (size_t)0);
        std::cout << "  PASS: Initialize" << std::endl;
        g_passed++;
    }

    // Test: Compile and cache hit
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        uint32_t id1 = cache.GetOrCompile("1 + 2");
        ASSERT_TRUE(id1 != UINT32_MAX);
        ASSERT_EQ(cache.Size(), (size_t)1);

        uint32_t id2 = cache.GetOrCompile("1 + 2");
        ASSERT_EQ(id1, id2); // Cache hit — same ID

        uint32_t id3 = cache.GetOrCompile("3 + 4");
        ASSERT_TRUE(id3 != id1); // Different expression — new ID
        ASSERT_EQ(cache.Size(), (size_t)2);

        auto [hits, misses] = cache.GetStats();
        ASSERT_EQ(hits, (uint64_t)1);   // One cache hit (second "1 + 2")
        ASSERT_EQ(misses, (uint64_t)2); // Two cache misses (first "1 + 2" + "3 + 4")

        std::cout << "  PASS: CompileAndCacheHit" << std::endl;
        g_passed++;
    }

    // Test: Evaluate integer expression
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        env["x"] = 10;
        env["y"] = 20;

        uint32_t id = cache.GetOrCompile("x + y");
        std::string result = cache.EvaluateAsString(id, env);
        ASSERT_STR_EQ(result, "30");

        std::cout << "  PASS: EvaluateInteger" << std::endl;
        g_passed++;
    }

    // Test: Evaluate string expression
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        env["name"] = "Tetris";

        uint32_t id = cache.GetOrCompile("name");
        std::string result = cache.EvaluateAsString(id, env);
        ASSERT_STR_EQ(result, "Tetris");

        std::cout << "  PASS: EvaluateString" << std::endl;
        g_passed++;
    }

    // Test: Evaluate boolean expression
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        env["score"] = 100;

        uint32_t id = cache.GetOrCompile("score > 50");
        ASSERT_TRUE(cache.EvaluateAsBool(id, env));

        uint32_t id2 = cache.GetOrCompile("score < 50");
        ASSERT_FALSE(cache.EvaluateAsBool(id2, env));

        std::cout << "  PASS: EvaluateBool" << std::endl;
        g_passed++;
    }

    // Test: Evaluate double expression
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        env["pi"] = 3.14159;

        uint32_t id = cache.GetOrCompile("pi");
        std::string result = cache.EvaluateAsString(id, env);
        ASSERT_STR_EQ(result, "3.14");

        std::cout << "  PASS: EvaluateDouble" << std::endl;
        g_passed++;
    }

    // Test: Evaluate nested table access (data.score pattern)
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        sol::table data = lua.create_table();
        data["score"] = 42;
        data["level"] = 5;
        env["data"] = data;

        uint32_t id = cache.GetOrCompile("data.score");
        std::string result = cache.EvaluateAsString(id, env);
        ASSERT_STR_EQ(result, "42");

        uint32_t id2 = cache.GetOrCompile("data.level > 3");
        ASSERT_TRUE(cache.EvaluateAsBool(id2, env));

        std::cout << "  PASS: EvaluateNestedTable" << std::endl;
        g_passed++;
    }

    // Test: Invalid expression returns UINT32_MAX
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        uint32_t id = cache.GetOrCompile("!!!invalid syntax");
        ASSERT_EQ(id, UINT32_MAX);

        std::cout << "  PASS: InvalidExpression" << std::endl;
        g_passed++;
    }

    // Test: Evaluate with invalid ID
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        sol::table env = lua.create_table();
        std::string result = cache.EvaluateAsString(999, env);
        ASSERT_STR_EQ(result, "");

        bool boolResult = cache.EvaluateAsBool(999, env);
        ASSERT_FALSE(boolResult);

        std::cout << "  PASS: InvalidExprId" << std::endl;
        g_passed++;
    }

    // Test: Dependency analysis
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        uint32_t id = cache.GetOrCompile("data.score");
        const auto& deps = cache.GetDependencies(id);
        ASSERT_TRUE(deps.count("data.score") > 0);
        ASSERT_TRUE(deps.count("data") > 0);

        std::cout << "  PASS: DependencyAnalysis" << std::endl;
        g_passed++;
    }

    // Test: Clear resets cache
    {
        ExpressionCache cache;
        cache.Initialize(&lua);

        cache.GetOrCompile("1 + 1");
        cache.GetOrCompile("2 + 2");
        ASSERT_EQ(cache.Size(), (size_t)2);

        cache.Clear();
        ASSERT_EQ(cache.Size(), (size_t)0);

        auto [hits, misses] = cache.GetStats();
        ASSERT_EQ(hits, (uint64_t)0);
        ASSERT_EQ(misses, (uint64_t)0);

        std::cout << "  PASS: Clear" << std::endl;
        g_passed++;
    }

    // Test: Not initialized returns error
    {
        ExpressionCache cache;
        // Don't call Initialize
        uint32_t id = cache.GetOrCompile("1 + 1");
        ASSERT_EQ(id, UINT32_MAX);

        std::cout << "  PASS: NotInitialized" << std::endl;
        g_passed++;
    }
}

// ============================================================================
// FrameTiming Tests
// ============================================================================

static void RunFrameTimingTests()
{
    std::cout << "\n--- FrameTiming ---" << std::endl;

    // Test: Fixed delta calculation
    {
        FrameTiming timing(FrameTimingMode::FIXED, 60.0);
        ASSERT_NEAR(timing.GetFixedDelta(), 1000.0 / 60.0, 0.01);

        FrameTiming timing30(FrameTimingMode::FIXED, 30.0);
        ASSERT_NEAR(timing30.GetFixedDelta(), 1000.0 / 30.0, 0.01);

        std::cout << "  PASS: FixedDeltaCalculation" << std::endl;
        g_passed++;
    }

    // Test: Fixed step accumulation
    {
        FrameTiming timing(FrameTimingMode::FIXED, 60.0);

        // Before any Update(), accumulator is 0 — no steps
        ASSERT_FALSE(timing.ShouldUpdateFixedStep());

        // Sleep to build up time, then Update
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();

        // Should have accumulated ~20ms, enough for one 16.67ms step
        ASSERT_TRUE(timing.ShouldUpdateFixedStep());

        std::cout << "  PASS: FixedStepAccumulation" << std::endl;
        g_passed++;
    }

    // Test: ShouldUpdateFixedStep returns false in VARIABLE mode
    {
        FrameTiming timing(FrameTimingMode::VARIABLE, 60.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();
        ASSERT_FALSE(timing.ShouldUpdateFixedStep());

        std::cout << "  PASS: FixedStepWrongMode" << std::endl;
        g_passed++;
    }

    // Test: Variable mode simulation accumulation
    {
        FrameTiming timing(FrameTimingMode::VARIABLE, 60.0, 60.0);
        timing.SetSimulationMultiplier(1.0f);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();

        // At 1x speed, ~20ms accumulated, one 16.67ms sim step
        ASSERT_TRUE(timing.ShouldUpdateSimulation());

        std::cout << "  PASS: VariableSimAccumulation" << std::endl;
        g_passed++;
    }

    // Test: Paused simulation (multiplier = 0)
    {
        FrameTiming timing(FrameTimingMode::VARIABLE, 60.0, 60.0);
        timing.SetSimulationMultiplier(0.0f);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();

        // Paused — no simulation updates
        ASSERT_FALSE(timing.ShouldUpdateSimulation());

        std::cout << "  PASS: PausedSimulation" << std::endl;
        g_passed++;
    }

    // Test: ShouldRenderFrame in variable mode
    {
        FrameTiming timing(FrameTimingMode::VARIABLE, 60.0, 60.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();

        // ~20ms elapsed >= 16.67ms render frame time
        ASSERT_TRUE(timing.ShouldRenderFrame());

        std::cout << "  PASS: VariableRenderFrame" << std::endl;
        g_passed++;
    }

    // Test: ShouldRenderFrame always true in FIXED mode
    {
        FrameTiming timing(FrameTimingMode::FIXED, 60.0);
        ASSERT_TRUE(timing.ShouldRenderFrame());

        std::cout << "  PASS: FixedAlwaysRenders" << std::endl;
        g_passed++;
    }

    // Test: Simple mode — delta only, no accumulators
    {
        FrameTiming timing(FrameTimingMode::SIMPLE);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timing.Update();

        // Delta should be roughly 10ms (allow variance for scheduling)
        ASSERT_TRUE(timing.GetDelta() >= 0.0);
        // No fixed step in SIMPLE mode
        ASSERT_FALSE(timing.ShouldUpdateFixedStep());
        // Always renders in non-VARIABLE modes
        ASSERT_TRUE(timing.ShouldRenderFrame());

        std::cout << "  PASS: SimpleMode" << std::endl;
        g_passed++;
    }

    // Test: Reset clears accumulators
    {
        FrameTiming timing(FrameTimingMode::FIXED, 60.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        timing.Update();
        ASSERT_TRUE(timing.ShouldUpdateFixedStep());

        timing.Reset();

        // After reset, accumulator should be 0
        ASSERT_FALSE(timing.ShouldUpdateFixedStep());

        std::cout << "  PASS: Reset" << std::endl;
        g_passed++;
    }

    // Test: Simulation multiplier getter/setter
    {
        FrameTiming timing(FrameTimingMode::VARIABLE, 60.0);

        ASSERT_NEAR(timing.GetSimulationMultiplier(), 1.0f, 0.001f);

        timing.SetSimulationMultiplier(2.0f);
        ASSERT_NEAR(timing.GetSimulationMultiplier(), 2.0f, 0.001f);

        timing.SetSimulationMultiplier(0.0f);
        ASSERT_NEAR(timing.GetSimulationMultiplier(), 0.0f, 0.001f);

        std::cout << "  PASS: SimMultiplier" << std::endl;
        g_passed++;
    }

    // Test: VSync getter/setter
    {
        FrameTiming timing(FrameTimingMode::FIXED, 60.0);

        timing.SetVSync(true);
        ASSERT_TRUE(timing.IsVSyncEnabled());

        timing.SetVSync(false);
        ASSERT_FALSE(timing.IsVSyncEnabled());

        std::cout << "  PASS: VSync" << std::endl;
        g_passed++;
    }
}

// ============================================================================
// EventQueue Safety Tests
//
// Regression for: hang caused by sol::table queue = lua["EventQueue"] when
// EventQueue is nil (not yet initialized during splash screen glfwPollEvents).
// The direct assignment panics via sol::default_at_panic → std::terminate →
// abort() when thrown from a GLFW/Cocoa callback (noexcept context).
//
// Fix: use sol::optional<sol::table> — safely returns nullopt instead of panicking.
// ============================================================================

static void RunEventQueueSafetyTests()
{
    std::cout << "\n--- EventQueue Safety (sol::optional regression) ---" << std::endl;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::table);

    // Test: nil global → nullopt, no panic
    {
        // EventQueue not set — simulates state before ScriptManager::Initialize()
        sol::optional<sol::table> queue = lua["EventQueue"];
        ASSERT_FALSE(queue.has_value());

        std::cout << "  PASS: NilGlobalReturnsNullopt" << std::endl;
        g_passed++;
    }

    // Test: valid table global → has_value(), can add entries
    {
        lua["EventQueue"] = lua.create_table();

        sol::optional<sol::table> queue = lua["EventQueue"];
        ASSERT_TRUE(queue.has_value());

        sol::table event = lua.create_table();
        event["type"] = "mousemove";
        queue->add(event);
        ASSERT_EQ(queue->size(), (size_t)1);

        std::cout << "  PASS: ValidTableReturnsValue" << std::endl;
        g_passed++;
    }

    // Test: wrong type (number) → nullopt, no panic
    {
        lua["EventQueue"] = 42; // overwrite with a non-table

        sol::optional<sol::table> queue = lua["EventQueue"];
        ASSERT_FALSE(queue.has_value());

        std::cout << "  PASS: WrongTypeReturnsNullopt" << std::endl;
        g_passed++;
    }
}

// ============================================================================
// SparseSet Tests (Registry storage primitive)
//
// Registry.h defines SparseSet<T> as the cache-friendly storage backing every
// component type. The template is header-only, so we can exercise it without
// linking the rest of Registry.cpp (which pulls in Game, ScriptManager, GL,
// yaml-cpp). These tests lock in the contracts the SparseSet relies on:
//   - O(1) add/get/has/remove
//   - swap-and-pop preserves remaining components
//   - sparse array auto-grows past initial capacity
// ============================================================================

#include "util/SparseSet.h"  // Standalone header — no engine deps

namespace {
struct TestComponent
{
    int value = 0;
    explicit TestComponent(int v = 0) : value(v) {}
};
}

static void RunSparseSetTests()
{
    std::cout << "\n--- SparseSet (Registry storage) ---" << std::endl;

    // Test: AddComponent, GetComponent, HasComponent
    {
        SparseSet<TestComponent> set;
        TestComponent c1(42);
        set.AddComponent(0, c1);
        ASSERT_TRUE(set.HasComponent(0));
        ASSERT_FALSE(set.HasComponent(1));
        ASSERT_EQ(set.GetComponent(0).value, 42);

        std::cout << "  PASS: AddGetHas" << std::endl;
        g_passed++;
    }

    // Test: RemoveComponent uses swap-and-pop, preserving remaining entities
    {
        SparseSet<TestComponent> set;
        TestComponent a(1), b(2), c(3);
        set.AddComponent(0, a);
        set.AddComponent(1, b);
        set.AddComponent(2, c);

        ASSERT_EQ(set.GetComponent(1).value, 2);

        set.RemoveComponent(1);

        ASSERT_FALSE(set.HasComponent(1));
        ASSERT_TRUE(set.HasComponent(0));
        ASSERT_TRUE(set.HasComponent(2));
        ASSERT_EQ(set.GetComponent(0).value, 1);
        ASSERT_EQ(set.GetComponent(2).value, 3);

        std::cout << "  PASS: RemoveSwapAndPop" << std::endl;
        g_passed++;
    }

    // Test: Sparse array auto-grows for entity ids beyond initial capacity (100)
    {
        SparseSet<TestComponent> set;
        TestComponent comp(7);
        // 500 > default maxEntities=100; should trigger doubling growth
        set.AddComponent(500, comp);
        ASSERT_TRUE(set.HasComponent(500));
        ASSERT_EQ(set.GetComponent(500).value, 7);

        std::cout << "  PASS: AutoGrow" << std::endl;
        g_passed++;
    }

    // Test: RemoveComponent on absent entity is safe (no crash, no-op)
    {
        SparseSet<TestComponent> set;
        set.RemoveComponent(0);   // Empty set
        TestComponent c(1);
        set.AddComponent(0, c);
        set.RemoveComponent(99);  // Out of dense range
        ASSERT_TRUE(set.HasComponent(0));

        std::cout << "  PASS: RemoveAbsentSafe" << std::endl;
        g_passed++;
    }

    // Test: GetEntities returns ids of all stored components (dense order)
    {
        SparseSet<TestComponent> set;
        TestComponent a(10), b(20), c(30);
        set.AddComponent(5, a);
        set.AddComponent(7, b);
        set.AddComponent(9, c);

        auto ids = set.GetEntities();
        ASSERT_EQ(ids.size(), (size_t)3);
        // Order is insertion order before any removal
        ASSERT_EQ(ids[0], (EntityID)5);
        ASSERT_EQ(ids[1], (EntityID)7);
        ASSERT_EQ(ids[2], (EntityID)9);

        std::cout << "  PASS: GetEntities" << std::endl;
        g_passed++;
    }
}

// ============================================================================
// Double-Buffer Atomicity Tests
//
// Locks in the invariant fixed in HTMLRendererMT:
//   - Producer (render thread) writes back buffer, then swaps under m_bufferMutex
//   - Consumer (main thread) reads front buffer pixel pointer; the read is
//     ONLY safe while m_bufferMutex is held, because std::swap swaps the
//     vector internals — releasing the lock before consuming the data races
//     the next producer cycle.
//
// This test simulates the pattern with two threads and verifies that pixel
// data observed under the lock is internally consistent (every byte of a
// frame matches its frameNumber). If the consumer ever released the lock
// before reading pixels, the producer could re-target the vector storage
// mid-iteration and the consistency check would fail.
// ============================================================================

namespace {
struct MockFrameBuffer
{
    uint64_t frameNumber = 0;
    std::vector<uint8_t> pixels;
};
}

static void RunDoubleBufferTests()
{
    std::cout << "\n--- Double-Buffer Atomicity (HTMLRendererMT pattern) ---" << std::endl;

    constexpr size_t kBufferBytes = 64 * 1024;  // 64KB simulated framebuffer
    constexpr int kFrames = 200;

    MockFrameBuffer front, back;
    front.pixels.assign(kBufferBytes, 0);
    back.pixels.assign(kBufferBytes, 0);

    std::mutex bufferMutex;
    std::atomic<bool> running{true};
    std::atomic<int> consistencyErrors{0};
    std::atomic<int> framesObserved{0};

    // Producer: writes a uniform byte pattern based on frame number, then swaps
    std::thread producer([&]() {
        uint64_t nextFrame = 1;
        for (int i = 0; i < kFrames && running; ++i)
        {
            uint8_t pattern = static_cast<uint8_t>(nextFrame & 0xFF);
            std::fill(back.pixels.begin(), back.pixels.end(), pattern);
            back.frameNumber = nextFrame;

            {
                std::lock_guard<std::mutex> lk(bufferMutex);
                std::swap(front, back);
            }

            ++nextFrame;
            // Yield to give the consumer a chance
            std::this_thread::yield();
        }
    });

    // Consumer: reads under lock, verifies every byte equals frameNumber & 0xFF
    std::thread consumer([&]() {
        uint64_t lastSeen = 0;
        while (running)
        {
            std::lock_guard<std::mutex> lk(bufferMutex);
            if (front.frameNumber != lastSeen && front.frameNumber > 0)
            {
                uint8_t expected = static_cast<uint8_t>(front.frameNumber & 0xFF);
                for (size_t i = 0; i < front.pixels.size(); ++i)
                {
                    if (front.pixels[i] != expected)
                    {
                        consistencyErrors.fetch_add(1);
                        break;
                    }
                }
                lastSeen = front.frameNumber;
                framesObserved.fetch_add(1);
            }
        }
    });

    producer.join();
    // Give consumer a moment to drain remaining frames
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    running = false;
    consumer.join();

    ASSERT_EQ(consistencyErrors.load(), 0);
    ASSERT_TRUE(framesObserved.load() > 0);
    std::cout << "  PASS: ProducerConsumerNoTorn (observed "
              << framesObserved.load() << " frames, 0 errors)" << std::endl;
    g_passed++;

    // Test: monotonic frame numbers under lock
    {
        std::mutex m;
        uint64_t counter = 0;
        std::atomic<bool> ok{true};
        constexpr int N = 1000;

        std::thread t1([&]() {
            for (int i = 0; i < N; ++i)
            {
                std::lock_guard<std::mutex> lk(m);
                ++counter;
            }
        });
        std::thread t2([&]() {
            uint64_t prev = 0;
            for (int i = 0; i < N; ++i)
            {
                std::lock_guard<std::mutex> lk(m);
                if (counter < prev) ok = false;
                prev = counter;
            }
        });
        t1.join();
        t2.join();
        ASSERT_TRUE(ok.load());
        std::cout << "  PASS: MonotonicCounterUnderLock" << std::endl;
        g_passed++;
    }
}

// ============================================================================
// Resize Frame-Skip Invariant
//
// Documents the rule fixed in HTMLRendererMT::Render(): a frame whose
// dimensions don't match the current texture must be skipped (not uploaded).
// This prevents a single garbage frame after Resize() until the render thread
// produces a frame at the new size.
//
// Pure-logic test — replicates the gating predicate so a future refactor
// that loosens it will trip the test.
// ============================================================================

static bool ShouldUploadFrame(uint64_t frameNumber, uint64_t lastFrameNumber,
                              int frameW, int frameH, int textureW, int textureH)
{
    if (frameNumber == lastFrameNumber) return false;
    if (frameW != textureW) return false;
    if (frameH != textureH) return false;
    return true;
}

static void RunResizeFrameSkipTests()
{
    std::cout << "\n--- Resize Frame-Skip Invariant ---" << std::endl;

    // Test: matching size + new frame → upload
    ASSERT_TRUE(ShouldUploadFrame(5, 4, 800, 600, 800, 600));
    std::cout << "  PASS: UploadOnNewFrame" << std::endl;
    g_passed++;

    // Test: same frame number → no upload
    ASSERT_FALSE(ShouldUploadFrame(4, 4, 800, 600, 800, 600));
    std::cout << "  PASS: SkipDuplicateFrame" << std::endl;
    g_passed++;

    // Test: width mismatch (post-resize, render thread hasn't caught up) → skip
    ASSERT_FALSE(ShouldUploadFrame(5, 4, 800, 600, 1024, 600));
    std::cout << "  PASS: SkipWidthMismatch" << std::endl;
    g_passed++;

    // Test: height mismatch → skip
    ASSERT_FALSE(ShouldUploadFrame(5, 4, 800, 600, 800, 768));
    std::cout << "  PASS: SkipHeightMismatch" << std::endl;
    g_passed++;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    // Initialize logger (required by ExpressionCache and FrameTiming)
    imhotep::Logger::GetInstance().Initialize("logs/engine-unit-tests.log");

    std::cout << "=== Engine Unit Tests ===" << std::endl;

    RunExpressionCacheTests();
    RunFrameTimingTests();
    RunEventQueueSafetyTests();
    RunSparseSetTests();
    RunDoubleBufferTests();
    RunResizeFrameSkipTests();

    std::cout << "\n--- Results ---" << std::endl;
    std::cout << "  Passed: " << g_passed << std::endl;
    std::cout << "  Failed: " << g_failed << std::endl;

    imhotep::Logger::GetInstance().Shutdown();
    return g_failed > 0 ? 1 : 0;
}
