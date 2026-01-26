-- Terrain UI State
return {
    data = {
        -- Performance metrics (updated by Game::TrackFPS)
        fps = 0,
        frameTime = "0.00",

        -- Required by Game::TrackFPS (even if terrain scene doesn't display these)
        gameMode = "FIXED",
        simSpeed = "NORMAL",
        simMultiplier = "1.0x",

        -- Terrain-specific stats
        cpuTiles = 0,
        gpuSlots = 0,
    }
}
