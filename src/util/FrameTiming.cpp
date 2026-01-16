/**
 * @file FrameTiming.cpp
 * @brief Implementation of unified frame timing abstraction
 */

#include "util/FrameTiming.h"
#include "util/Logger.h"

FrameTiming::FrameTiming(FrameTimingMode mode, double targetFPS, double renderFPS)
	: m_mode(mode), m_targetFPS(targetFPS)
{
	m_fixedDelta = 1000.0 / targetFPS;
	m_renderDelta = (renderFPS > 0.0) ? (1000.0 / renderFPS) : m_fixedDelta;

	m_prevTime = std::chrono::high_resolution_clock::now();
	m_frameStart = m_prevTime;

	LOG_DEBUG("FrameTiming initialized: mode={}, targetFPS={}, fixedDelta={}ms",
		(mode == FrameTimingMode::FIXED ? "FIXED" : (mode == FrameTimingMode::VARIABLE ? "VARIABLE" : "SIMPLE")),
		targetFPS, m_fixedDelta);
}

void FrameTiming::Update()
{
	auto currTime = std::chrono::high_resolution_clock::now();
	m_delta = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_prevTime).count();
	m_prevTime = currTime;
	m_frameStart = currTime;

	switch (m_mode)
	{
	case FrameTimingMode::FIXED:
		// Simple accumulation for fixed timestep
		m_accumulator += m_delta;
		break;

	case FrameTimingMode::VARIABLE:
	{
		// Dual accumulators for decoupled sim/render
		float multiplier = m_simMultiplier;

		if (multiplier == -1.0f)
		{
			// UNCAPPED: Mark that simulation should run many times
			// (handled in ShouldUpdateSimulation)
		}
		else if (multiplier > 0.0f)
		{
			m_simAccumulator += m_delta * multiplier;
		}
		// If multiplier == 0 (paused), don't increment sim accumulator

		m_renderAccumulator += m_delta;
		break;
	}

	case FrameTimingMode::SIMPLE:
		// Delta-only, no accumulators
		break;
	}

	m_simulationStepConsumed = false;
	m_renderStepConsumed = false;
}

bool FrameTiming::ShouldUpdateFixedStep()
{
	if (m_mode != FrameTimingMode::FIXED)
	{
		return false;
	}

	if (m_accumulator >= m_fixedDelta)
	{
		m_accumulator -= m_fixedDelta;
		return true;
	}

	return false;
}

bool FrameTiming::ShouldUpdateSimulation()
{
	if (m_mode != FrameTimingMode::VARIABLE)
	{
		return false;
	}

	float multiplier = m_simMultiplier;

	// UNCAPPED mode: Run as many updates as possible
	if (multiplier == -1.0f)
	{
		// Only allow a limited number of sim steps per frame to prevent infinite loops
		static int uncappedStepCount = 0;
		static auto lastFrameStart = std::chrono::high_resolution_clock::now();

		auto now = std::chrono::high_resolution_clock::now();
		if (now != lastFrameStart)
		{
			// New frame, reset counter
			uncappedStepCount = 0;
			lastFrameStart = now;
		}

		const int MAX_UNCAPPED_STEPS = 100;
		if (uncappedStepCount < MAX_UNCAPPED_STEPS)
		{
			uncappedStepCount++;
			return true;
		}
		return false;
	}

	// Normal mode: Check accumulator
	if (m_simAccumulator >= m_fixedDelta)
	{
		m_simAccumulator -= m_fixedDelta;
		return true;
	}

	return false;
}

bool FrameTiming::ShouldRenderFrame()
{
	if (m_mode != FrameTimingMode::VARIABLE)
	{
		return true; // Always render in FIXED and SIMPLE modes
	}

	if (m_renderAccumulator >= m_renderDelta)
	{
		m_renderAccumulator -= m_renderDelta;
		return true;
	}

	return false;
}

double FrameTiming::GetElapsedMS() const
{
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_prevTime).count();
}

double FrameTiming::GetFrameElapsedMS() const
{
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_frameStart).count();
}

void FrameTiming::Reset()
{
	m_accumulator = 0.0;
	m_simAccumulator = 0.0;
	m_renderAccumulator = 0.0;
	m_delta = 0.0;
	m_simMultiplier = 1.0f;
	m_prevTime = std::chrono::high_resolution_clock::now();
	m_frameStart = m_prevTime;
	m_simulationStepConsumed = false;
	m_renderStepConsumed = false;
}
