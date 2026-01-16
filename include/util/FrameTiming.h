/**
 * @file FrameTiming.h
 * @brief Abstraction for frame timing and accumulator logic
 *
 * Provides unified timing for fixed timestep, variable speed, and simple delta-based loops.
 * Encapsulates accumulator logic to reduce duplication across Game and Editor.
 */

#pragma once

#include <chrono>

/**
 * @brief Frame timing modes
 *
 * Determines how frame timing and updates are managed.
 */
enum class FrameTimingMode
{
	FIXED,    ///< Fixed timestep with single accumulator (action games)
	VARIABLE, ///< Dual accumulators for decoupled sim/render (strategy games)
	SIMPLE    ///< Delta-only timing (editor, free-running)
};

/**
 * @brief Unified frame timing abstraction
 *
 * Manages:
 * - Delta time calculation between frames
 * - Accumulator logic for fixed timesteps (scripts)
 * - Dual accumulators for variable speed (sim + render)
 * - Simple delta-based timing (editor)
 *
 * **Usage:**
 *
 * ```cpp
 * FrameTiming timing(FrameTimingMode::FIXED, 60.0);  // 60 FPS target
 *
 * while (!shouldClose()) {
 *   glfwPollEvents();
 *
 *   // Calculate delta and accumulators
 *   timing.Update();
 *
 *   // For FIXED mode:
 *   while (timing.ShouldUpdateFixedStep()) {
 *     ScriptSystem::Update(timing.GetFixedDelta());
 *   }
 *
 *   // System updates
 *   TweenSystem::Update(timing.GetDelta());
 *   HierarchySystem::Update();
 *   Render();
 * }
 * ```
 *
 * **Thread Safety**: Not thread-safe; call Update() only from main loop thread.
 */
class FrameTiming
{
public:
	/**
	 * @brief Construct frame timer for specified mode
	 * @param mode Timing mode (FIXED, VARIABLE, or SIMPLE)
	 * @param targetFPS Target framerate for fixed/variable modes (60 default)
	 * @param renderFPS Target render rate for VARIABLE mode (same as targetFPS default)
	 */
	FrameTiming(FrameTimingMode mode = FrameTimingMode::FIXED, double targetFPS = 60.0, double renderFPS = 0.0);

	/**
	 * @brief Update timing state for current frame
	 *
	 * Must be called exactly once per frame. Calculates delta time and
	 * updates accumulators for the timing mode.
	 */
	void Update();

	/**
	 * @brief Get delta time since last frame in milliseconds
	 * @return Time elapsed in milliseconds
	 */
	double GetDelta() const { return m_delta; }

	/**
	 * @brief Get fixed timestep delta (used for deterministic updates)
	 * @return Fixed delta in milliseconds (e.g., 16.67 for 60 FPS)
	 */
	double GetFixedDelta() const { return m_fixedDelta; }

	/**
	 * @brief Check if a fixed timestep update should occur (FIXED mode only)
	 * @return true if accumulator >= fixedDelta, false otherwise
	 *
	 * Must be called in a while loop until returns false.
	 * Each call consumes one fixed timestep from accumulator.
	 */
	bool ShouldUpdateFixedStep();

	/**
	 * @brief Check if simulation should update (VARIABLE mode only)
	 * @return true if simulation accumulator >= fixed tick time
	 * @note Must be called in while loop; each call consumes one tick
	 */
	bool ShouldUpdateSimulation();

	/**
	 * @brief Check if rendering should occur (VARIABLE mode only)
	 * @return true if render accumulator >= render frame time
	 * @note Call once per frame to determine if Render() should execute
	 */
	bool ShouldRenderFrame();

	/**
	 * @brief Set simulation speed multiplier (VARIABLE mode only)
	 * @param multiplier Speed factor (0.0 = paused, 1.0 = normal, 5.0 = 5x)
	 * @note Ignored in FIXED and SIMPLE modes
	 */
	void SetSimulationMultiplier(float multiplier) { m_simMultiplier = multiplier; }

	/**
	 * @brief Get current simulation speed multiplier
	 * @return Multiplier value
	 */
	float GetSimulationMultiplier() const { return m_simMultiplier; }

	/**
	 * @brief Get elapsed milliseconds since timer start
	 * @return Total elapsed time in milliseconds
	 */
	double GetElapsedMS() const;

	/**
	 * @brief Reset timing state
	 * @note Useful for pause/resume or mode switches
	 */
	void Reset();

	/**
	 * @brief Enable/disable VSync (for fixed mode)
	 * @param enabled true to enable, false to disable
	 * @note Call before Update() loop begins
	 */
	void SetVSync(bool enabled) { m_vsyncEnabled = enabled; }

	/**
	 * @brief Get whether VSync is enabled
	 * @return true if VSync is active
	 */
	bool IsVSyncEnabled() const { return m_vsyncEnabled; }

	/**
	 * @brief Get elapsed time since frame started in milliseconds
	 * @return Current elapsed time (useful for measuring frame processing)
	 */
	double GetFrameElapsedMS() const;

private:
	FrameTimingMode m_mode;
	double m_targetFPS;       ///< Target FPS for fixed/render timing
	double m_fixedDelta;      ///< Fixed timestep in milliseconds
	double m_renderDelta;     ///< Render frame time in milliseconds
	double m_delta = 0.0;     ///< Current frame delta in milliseconds
	double m_accumulator = 0.0;       ///< Main accumulator (FIXED mode)
	double m_simAccumulator = 0.0;    ///< Simulation accumulator (VARIABLE mode)
	double m_renderAccumulator = 0.0; ///< Render accumulator (VARIABLE mode)
	float m_simMultiplier = 1.0f;     ///< Simulation speed (VARIABLE mode)
	bool m_vsyncEnabled = true;       ///< Whether VSync is active
	bool m_simulationStepConsumed = false;  ///< Tracks if ShouldUpdateSimulation consumed a step
	bool m_renderStepConsumed = false;      ///< Tracks if ShouldRenderFrame consumed a step

	std::chrono::high_resolution_clock::time_point m_prevTime;
	std::chrono::high_resolution_clock::time_point m_frameStart;
};
