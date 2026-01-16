-- Editor UI state
return {
    viewportImage = "",
    entities = {},
    selectedEntityId = nil,
    selectedEntityName = "",
    selectedEntityPosition = { x = 0, y = 0, z = 0 },
    selectedEntityRotation = { x = 0, y = 0, z = 0 },
    selectedEntityScale = { x = 1, y = 1, z = 1 },

    -- Methods section: event handlers called from UI interactions
    -- Note: C++ code may add additional handlers (e.g., selectEntity) at runtime
    methods = {}
}
