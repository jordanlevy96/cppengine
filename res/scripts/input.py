import sys
sys.path.append('../res/scripts')

import app_module
import camera
import enums
import tetris
import TetrisGrid

# GameManager = app_module.App.get_instance()
Window = app_module.Window.instance
CameraRotateFlag = False
event_queue = []

def handle_input():
    global CameraRotateFlag  
    global event_queue
    
    while len(event_queue) > 0:
        event = event_queue.pop(0)
        if event.type == enums.InputTypes.KEY:
            on_key_press(event.input)
        elif event.type == enums.InputTypes.CURSOR:
            if CameraRotateFlag:
                on_cursor(event.input)
        elif event.type == enums.InputTypes.SCROLL:
            on_scroll(event.input)
    
def on_key_press(key):
    global CameraRotateFlag  
    if key is None:
        return

    # camera = GameManager.camera

    if key == "ESCAPE":
        print('# # #')
        print('New App created :(' if app_module.App.test() == 'test string' else 'SUCCESS')
        print(app_module.App.test())
        print('===')
        print(Window.GetSize())
        Window.CloseWindow()
        print('* * * * *')
    elif key == "SPACE":
        CameraRotateFlag = not CameraRotateFlag
    elif key == "W":
        camera.move(enums.CameraDirections.FORWARD, GameManager.delta)
    elif key == "S":
        camera.move(enums.CameraDirections.BACK, GameManager.delta)
    elif key == "A":
        camera.move(enums.CameraDirections.LEFT, GameManager.delta)
    elif key == "D":
        camera.move(enums.CameraDirections.RIGHT, GameManager.delta)
    # elif key == "Z":
    #     TetrisGrid.RotateTetrimino(enums.Rotations.CCW)
    # elif key in ["X", "UP"]:
    #     TetrisGrid.RotateTetrimino(enums.Rotations.CW)
    # elif key == "LEFT":
    #     tetris.MoveTetrimino(TetrisGrid.active_piece, {'x': -1, 'y': 0})
    # elif key == "RIGHT":
    #     tetris.MoveTetrimino(TetrisGrid.active_piece, {'x': 1, 'y': 0})
    # elif key == "P": 
        # TODO: pause

def on_scroll(input):
    camera = GameManager.camera
    size = Window.GetSize()
    fov = camera.fov - input['y']
    fov = max(1, min(fov, 45))  # Clamp the fov between 1 and 45

    camera.SetPerspective(fov, size['x'], size['y'])

def on_cursor(input):
    GameManager.camera.RotateByMouse(input['x'], input['y'])
