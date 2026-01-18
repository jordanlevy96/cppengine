-- Tetrimino shape and color definitions
-- Converted from res/conf/tetriminos.yaml for native Lua access
-- This eliminates the need for C++ to load and manage tetrimino data

TetriminoData = {
    I = {
        color = vec3(0, 0.8, 0.8),  -- Cyan
        shape = {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
        }
    },

    O = {
        color = vec3(0.8, 0.8, 0),  -- Yellow
        shape = {
            {0, 1, 1, 0},
            {0, 1, 1, 0},
        }
    },

    T = {
        color = vec3(0.6, 0, 0.8),  -- Purple
        shape = {
            {0, 0, 1, 0},
            {0, 1, 1, 1}
        }
    },

    J = {
        color = vec3(0, 0, 0.8),  -- Blue
        shape = {
            {0, 1, 1, 1},
            {0, 0, 0, 1},
        }
    },

    L = {
        color = vec3(0.8, 0.4, 0),  -- Orange
        shape = {
            {1, 1, 1, 0},
            {1, 0, 0, 0}
        }
    },

    S = {
        color = vec3(0, 0.8, 0),  -- Green
        shape = {
            {0, 1, 1, 0},
            {1, 1, 0, 0}
        }
    },

    Z = {
        color = vec3(0.8, 0, 0),  -- Red
        shape = {
            {0, 1, 1, 0},
            {0, 0, 1, 1}
        }
    }
}

return TetriminoData
