-- TetrisGrid

TetrisGrid = {
    -- read from YAML
    model = nil,
    borderShader = nil,
    gameObjectShader = nil,

    -- initialized in ready
    borderCube = nil,
    cube = nil,

    -- static
    borderColor = vec3(0.471, 0.471, 0.471),
    cubeSize = 1,
    
    ready = function(self)
        self.borderCube = CreateRenderComponent(self.borderShader, self.model)
        self.cube = CreateRenderComponent(self.gameObjectShader, self.model)
        CreateCube(self.borderCube, vec3(0), self.borderColor)
        print("created cube")
        -- TetrisGrid:renderBorder()
    end,

    renderBorder = function(self)
        -- Create the left and right borders
        for y = 0, self.gridHeight do
            CreateCube(-self.cubeSize, y * self.cubeSize, 0) -- Left border
            CreateCube(self.gridWidth * self.cubeSize, y * self.cubeSize, 0) -- Right border
        end
        -- Create the top border
        for x = -1, self.gridWidth do
            CreateCube(x * self.cubeSize, self.gridHeight * self.cubeSize, 0) -- Top border
        end
    end,

    process = function(self, delta)
        -- print("TetrisGrid process")
    end
}
