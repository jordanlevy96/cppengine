-- Tetrimino shape and color definitions
-- Converted from res/conf/tetriminos.yaml for native Lua access
-- This eliminates the need for C++ to load and manage tetrimino data

local TetriminoData = {
    _contract = {
        role = "data"
    },
    I = {
        color = vec3(0, 0.9, 1.0),  -- Electric cyan
        shape = {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
        }
    },

    O = {
        color = vec3(1.0, 0.8, 0.0),  -- Neon yellow/gold
        shape = {
            {0, 1, 1, 0},
            {0, 1, 1, 0},
        }
    },

    T = {
        color = vec3(0.6, 0.1, 0.9),  -- Deep purple
        shape = {
            {0, 0, 1, 0},
            {0, 1, 1, 1}
        }
    },

    J = {
        color = vec3(0.4, 0.6, 1.0),  -- Bright blue
        shape = {
            {0, 1, 1, 1},
            {0, 0, 0, 1},
        }
    },

    L = {
        color = vec3(1.0, 0.3, 0.5),  -- Hot pink
        shape = {
            {1, 1, 1, 0},
            {1, 0, 0, 0}
        }
    },

    S = {
        color = vec3(1.0, 0.0, 0.8),  -- Neon magenta
        shape = {
            {0, 1, 1, 0},
            {1, 1, 0, 0}
        }
    },

    Z = {
        color = vec3(0.9, 0.2, 0.6),  -- Pink/magenta
        shape = {
            {0, 1, 1, 0},
            {0, 0, 1, 1}
        }
    }
}

return TetriminoData
