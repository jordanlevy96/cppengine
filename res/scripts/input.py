import app_module
import camera
import enums
import tetris_funcs
import TetrisGrid

GameManager = app_module.App.GetInstance()
RES_PATH = GameManager.conf.resPath
CameraRotateFlag = False

def handle_input():
    global CameraRotateFlag  
    while len(event_queue) > 0:
        event = event_queue.pop(0)
        if event['type'] == enums.InputTypes.KEY:
            on_key_press(event['input'])
        elif event['type'] == enums.InputTypes.CURSOR:
            if CameraRotateFlag:
                on_cursor(event['input'])
        elif event['type'] == enums.InputTypes.SCROLL:
            on_scroll(event['input'])

def on_scroll(input):
    camera = GameManager.camera
    size = GameManager.window.get_size()
    fov = camera.fov - input['y']
    fov = max(1, min(fov, 45))  # Clamp the fov between 1 and 45

    camera.set_perspective(fov, size['x'], size['y'])

def on_cursor(input):
    GameManager.camera.rotate_by_mouse(input['x'], input['y'])

def on_key_press(key):
    global CameraRotateFlag  
    if key is None:
        return

    camera = GameManager.camera

    if key == "ESCAPE":
        GameManager.window.close_window()
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
    elif key == "Z":
        TetrisGrid.rotate_tetrimino(enums.Rotations.CCW)
    elif key in ["X", "UP"]:
        TetrisGrid.rotate_tetrimino(enums.Rotations.CW)
    elif key == "LEFT":
        tetris_funcs.move_tetrimino(TetrisGrid.active_piece, {'x': -1, 'y': 0})
    elif key == "RIGHT":
        tetris_funcs.move_tetrimino(TetrisGrid.active_piece, {'x': 1, 'y': 0})
    # elif key == "P": 
        # TODO: pause

