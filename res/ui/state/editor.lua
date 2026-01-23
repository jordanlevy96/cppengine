-- Editor UI state
return {
    viewportImage = "",
    entities = {},
    selectedEntityId = nil,
    selectedEntityName = "",
    selectedEntityPosition = { x = 0, y = 0, z = 0 },
    selectedEntityRotation = { x = 0, y = 0, z = 0 },
    selectedEntityScale = { x = 1, y = 1, z = 1 },

    -- Hover state for entity items (using @mouseenter/@mouseleave)
    hoveredEntityId = nil,
    showEntityTooltip = false,
    tooltipText = "",

    -- Button press state (using @mousedown/@mouseup)
    pressedButton = "",

    -- Transform table for v-table directive
    transformTable = {
        columns = {"Property", "X", "Y", "Z"},
        rows = {
            {"Position", 0, 0, 0},
            {"Rotation", 0, 0, 0},
            {"Scale", 1, 1, 1}
        }
    },

    -- UI Editor state
    uiEditor = {
        currentTemplate = "",
        templates = {},
        htmlCode = "",
        cssCode = "",
        luaCode = "",
        previewHTML = "",
        isDirty = false,
        activeTab = "html",  -- "html", "css", or "lua"
        errorMessage = "",
        newTemplateName = ""
    },

    -- Methods section: event handlers called from UI interactions
    -- Note: C++ code may add additional handlers (e.g., selectEntity) at runtime
    methods = {
        -- Entity hover handlers (dogfooding @mouseenter/@mouseleave)
        onEntityHoverEnter = function(self, entityId, entityName)
            self.data.hoveredEntityId = entityId
            self.data.showEntityTooltip = true
            self.data.tooltipText = "Entity ID: " .. tostring(entityId)
            print("[Editor] Hover enter: " .. entityName .. " (ID: " .. entityId .. ")")
        end,

        onEntityHoverLeave = function(self)
            self.data.hoveredEntityId = nil
            self.data.showEntityTooltip = false
            self.data.tooltipText = ""
            print("[Editor] Hover leave")
        end,

        -- Tab hover handlers
        onTabHover = function(self, tabName)
            print("[Editor] Tab hover: " .. tabName)
        end,

        onTabLeave = function(self)
            -- Could show/hide tab previews here
        end,

        switchTab = function(self, tabName)
            self.data.uiEditor.activeTab = tabName
            print("[Editor] Switched to tab: " .. tabName)
        end,

        -- Button press handlers (dogfooding @mousedown/@mouseup)
        onButtonPress = function(self, buttonName)
            self.data.pressedButton = buttonName
            print("[Editor] Button pressed: " .. buttonName)
        end,

        onButtonRelease = function(self)
            self.data.pressedButton = ""
            print("[Editor] Button released")
        end,

        -- Update transform table when entity is selected
        updateTransformTable = function(self)
            self.data.transformTable.rows = {
                {"Position",
                 self.data.selectedEntityPosition.x,
                 self.data.selectedEntityPosition.y,
                 self.data.selectedEntityPosition.z},
                {"Rotation",
                 self.data.selectedEntityRotation.x,
                 self.data.selectedEntityRotation.y,
                 self.data.selectedEntityRotation.z},
                {"Scale",
                 self.data.selectedEntityScale.x,
                 self.data.selectedEntityScale.y,
                 self.data.selectedEntityScale.z}
            }
        end
    }
}
