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
    cubeSize = 2,
    gridWidth = 10,
    gridHeight = 20,
    
    ready = function(self)
        self.borderCube = CreateRenderComponent(self.borderShader, self.model)
        self.cube = CreateRenderComponent(self.gameObjectShader, self.model)
        TetrisGrid:setCamera()
        TetrisGrid:renderBorder()
    end,

    setCamera = function(self)
        
        local camera = GameManager.camera
        local maxDimension = math.max(self.gridWidth + 2, self.gridHeight + 2)
        camera:SetPerspective(45)
        local fovRadians = math.rad(camera.fov) -- Convert FOV to radians
        local distance = -(maxDimension / math.tan(fovRadians / 2))

        camera.transform.Pos = vec3(self.gridWidth - 1, self.gridHeight - 1, distance - 1) 
    end,

    renderBorder = function(self)
        -- Create the left and right borders
        for y = 0, self.gridHeight do
            CreateCube(self.borderCube, vec3(-self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Left border
            CreateCube(self.borderCube, vec3(self.gridWidth * self.cubeSize, y * self.cubeSize, 0), self.borderColor) -- Right border
        end
        -- Create the top border
        for x = -1, self.gridWidth do
            CreateCube(self.borderCube, vec3(x * self.cubeSize, self.gridHeight * self.cubeSize, 0), self.borderColor) -- Top border
            CreateCube(self.borderCube, vec3(x * self.cubeSize, -self.cubeSize, 0), self.borderColor) -- Bottom border
        end
    end,

    process = function(self, delta)
        -- print("TetrisGrid process")
        local camera = GameManager.camera
        local pos = camera.transform.Pos
        print("Camera Pos: ", pos.x, pos.y, pos.z)
        print("fov: ", camera.fov)
    end
}
