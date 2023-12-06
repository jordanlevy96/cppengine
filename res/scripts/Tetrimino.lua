-- Tetrimino
    
Tetrimino = {
    ready = function(self)
        print("Tetrimino ready!")
    end,

    moveTetrimino = function(self, delta, grid)
        self.timeSinceLastMove = self.timeSinceLastMove + delta
        if self.timeSinceLastMove >= self.moveDownInterval then
            if self:canMoveDown(grid) then
                self:moveDown(grid)
            else
                print('lock')
                -- self:placeTetrimino()
            end
            self.timeSinceLastMove = 0
        end
    end,

    placeTetrimino = function(grid, tetrimino, posX, posY)
        -- tetrimino.shape is a 2D array representing the shape of the Tetrimino
        -- posX and posY represent the Tetrimino's position on the grid
        for i = 1, #tetrimino.shape do
            for j = 1, #tetrimino.shape[i] do
                if tetrimino.shape[i][j] == 1 then
                    -- Check if the position is within the grid boundaries
                    if posX + j >= 1 and posX + j <= #grid[1] and posY + i >= 1 and posY + i <= #grid then
                        grid[posY + i][posX + j] = true
                    else
                        print("Error: Tetrimino is out of grid bounds")
                    end
                end
            end
        end
    end,

    canMoveDown = function(self, grid)
        -- Check grid for collision below the current Tetrimino
        -- Return true if it can move down, false otherwise
        return true
    end,

    moveDown = function(self, grid)
        print("gravity")
        -- Move Tetrimino down and update the grid accordingly
    end,
}