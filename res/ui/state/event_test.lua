-- Event Test UI State
-- Tests new event system features: mouseenter/leave, mousedown/up, v-table

return {
    data = {
        -- Section 1: Click Test
        clickCount = 0,
        lastEvent = "No events yet",

        -- Section 2: Hover Tests
        hoverStatus1 = "Not hovering",
        hoverStatus2 = "Not hovering",
        hoverStatus3 = "Not hovering",

        -- Section 3: Mouse Down/Up Tests
        pressState = "Press and Hold",
        pressDuration = 0,
        pressStartTime = 0,

        -- Section 4: v-table Test Data
        playerData = {
            columns = {"Player", "Score", "Level", "Status"},
            rows = {
                {"Alice", 15420, 42, "Online"},
                {"Bob", 12350, 38, "Offline"},
                {"Charlie", 18900, 45, "Online"},
                {"Diana", 9870, 35, "Away"}
            }
        },

        metricsData = {
            headers = {"Metric", "Current", "Target", "% Complete"},
            data = {
                {"FPS", 60, 60, 100},
                {"Frame Time", "16.7ms", "16.7ms", 100},
                {"Memory", "124MB", "256MB", 48},
                {"Draw Calls", 245, 300, 82}
            }
        },

        -- Section 5: Combined Events (Interactive Card)
        cardStatus = "Idle",
        cardClicks = 0,
        cardHovers = 0,
        cardPresses = 0,

        -- Section 6: Event Log
        eventLog = {
            {time = "00:00:00", message = "Event test UI initialized"}
        }
    },

    methods = {
        -- Section 1: Click Event Handler
        onButtonClick = function(self)
            self.data.clickCount = self.data.clickCount + 1
            self.data.lastEvent = "Button clicked (count: " .. self.data.clickCount .. ")"

            -- Add to event log
            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "CLICK: Button clicked (total: " .. self.data.clickCount .. ")"
            }
            table.insert(self.data.eventLog, 1, logEntry)

            print("[EventTest] Button clicked! Count: " .. self.data.clickCount)
        end,

        -- Section 2: Mouse Enter/Leave Handlers
        onMouseEnter = function(self, boxName)
            print("[EventTest] Mouse entered: " .. boxName)

            if boxName == "Box 1" then
                self.data.hoverStatus1 = "HOVERING"
            elseif boxName == "Box 2" then
                self.data.hoverStatus2 = "HOVERING"
            end

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSEENTER: " .. boxName
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        onMouseLeave = function(self, boxName)
            print("[EventTest] Mouse left: " .. boxName)

            if boxName == "Box 1" then
                self.data.hoverStatus1 = "Not hovering"
            elseif boxName == "Box 2" then
                self.data.hoverStatus2 = "Not hovering"
            end

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSELEAVE: " .. boxName
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        -- Legacy hover events (for comparison)
        onMouseOver = function(self, boxName)
            print("[EventTest] Mouse over (legacy): " .. boxName)
            self.data.hoverStatus3 = "HOVERING (old event)"

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSEOVER (legacy): " .. boxName
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        onMouseOut = function(self, boxName)
            print("[EventTest] Mouse out (legacy): " .. boxName)
            self.data.hoverStatus3 = "Not hovering"

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSEOUT (legacy): " .. boxName
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        -- Section 3: Mouse Down/Up Handlers
        onMouseDown = function(self)
            print("[EventTest] Mouse down - button pressed")
            self.data.pressState = "PRESSING..."
            self.data.pressStartTime = os.clock() * 1000  -- Convert to milliseconds

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSEDOWN: Button pressed"
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        onMouseUp = function(self)
            local duration = 0
            if self.data.pressStartTime > 0 then
                local endTime = os.clock() * 1000
                duration = math.floor(endTime - self.data.pressStartTime)
            end

            print("[EventTest] Mouse up - button released (duration: " .. duration .. "ms)")
            self.data.pressState = "Press and Hold"
            self.data.pressDuration = duration

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "MOUSEUP: Button released (held for " .. duration .. "ms)"
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        -- Section 4: Table Data Refresh
        refreshTableData = function(self)
            print("[EventTest] Refreshing table data")

            -- Generate random scores for players
            local players = {"Alice", "Bob", "Charlie", "Diana"}
            local newRows = {}
            for i, name in ipairs(players) do
                local score = math.random(8000, 20000)
                local level = math.random(30, 50)
                local statuses = {"Online", "Offline", "Away"}
                local status = statuses[math.random(1, #statuses)]
                table.insert(newRows, {name, score, level, status})
            end

            self.data.playerData.rows = newRows

            -- Update metrics with random values
            local fps = math.random(55, 60)
            local frameTime = string.format("%.1fms", 1000.0 / fps)
            local memory = math.random(100, 200)
            local drawCalls = math.random(200, 300)

            self.data.metricsData.data = {
                {"FPS", fps, 60, math.floor((fps/60) * 100)},
                {"Frame Time", frameTime, "16.7ms", math.floor((60/fps) * 100)},
                {"Memory", memory .. "MB", "256MB", math.floor((memory/256) * 100)},
                {"Draw Calls", drawCalls, 300, math.floor((drawCalls/300) * 100)}
            }

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "TABLE: Data refreshed"
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        -- Section 5: Combined Events (Interactive Card)
        onCardClick = function(self)
            self.data.cardClicks = self.data.cardClicks + 1
            self.data.cardStatus = "Clicked!"

            print("[EventTest] Card clicked (total: " .. self.data.cardClicks .. ")")

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "CARD: Clicked (total: " .. self.data.cardClicks .. ")"
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        onCardEnter = function(self)
            self.data.cardHovers = self.data.cardHovers + 1
            self.data.cardStatus = "Hovering..."

            print("[EventTest] Card hover started")
        end,

        onCardLeave = function(self)
            self.data.cardStatus = "Idle"
            print("[EventTest] Card hover ended")
        end,

        onCardPress = function(self)
            self.data.cardPresses = self.data.cardPresses + 1
            self.data.cardStatus = "Pressed!"

            print("[EventTest] Card pressed (total: " .. self.data.cardPresses .. ")")

            local logEntry = {
                time = os.date("%H:%M:%S"),
                message = "CARD: Pressed (total: " .. self.data.cardPresses .. ")"
            }
            table.insert(self.data.eventLog, 1, logEntry)
        end,

        onCardRelease = function(self)
            self.data.cardStatus = "Released"
            print("[EventTest] Card released")
        end,

        -- Section 6: Clear Event Log
        clearLog = function(self)
            self.data.eventLog = {
                {time = os.date("%H:%M:%S"), message = "Event log cleared"}
            }
            print("[EventTest] Event log cleared")
        end
    }
}
