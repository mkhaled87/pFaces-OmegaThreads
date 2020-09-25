import arcade
import math
import sys

# insert at interface folder
sys.path.insert(1, '../../interface/python')
from OmegaInterface import Controller

# window settings
SCREEN_WIDTH = 500
SCREEN_HEIGHT = 500

# info for the robot arena
ZERO_BASE_X = 50
ZERO_BASE_Y = 50
ARENA_WIDTH = 400
ARENA_HIGHT = 400
X_GRID = 100
Y_GRID = 100
ROBOT_INITIAL_X = 2*ZERO_BASE_X
ROBOT_INITIAL_Y = 2*ZERO_BASE_Y

def draw_arena():
    # Draw the bounds
    arcade.draw_rectangle_filled(ZERO_BASE_X + ARENA_WIDTH/2, ZERO_BASE_Y + ARENA_HIGHT/2, ARENA_WIDTH, ARENA_HIGHT, arcade.color.DARK_GRAY)
    arcade.draw_rectangle_outline(ZERO_BASE_X + ARENA_WIDTH/2, ZERO_BASE_Y + ARENA_HIGHT/2, ARENA_WIDTH, ARENA_HIGHT, arcade.color.BLACK)

    # draw obstacles (RED) and target (GREEN)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 3*X_GRID, 2*ZERO_BASE_Y + 0*Y_GRID, X_GRID, Y_GRID, arcade.color.AVOCADO)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 3*X_GRID, 2*ZERO_BASE_Y + 1*Y_GRID, X_GRID, Y_GRID, arcade.color.AVOCADO)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 1*X_GRID, 2*ZERO_BASE_Y + 0*Y_GRID, X_GRID, Y_GRID, arcade.color.BARN_RED)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 2*X_GRID, 2*ZERO_BASE_Y + 0*Y_GRID, X_GRID, Y_GRID, arcade.color.BARN_RED)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 1*X_GRID, 2*ZERO_BASE_Y + 1*Y_GRID, X_GRID, Y_GRID, arcade.color.BARN_RED)
    arcade.draw_rectangle_filled(2*ZERO_BASE_X + 1*X_GRID, 2*ZERO_BASE_Y + 2*Y_GRID, X_GRID, Y_GRID, arcade.color.BARN_RED)

    # draw grid
    for i in range(1,4):
        arcade.draw_line(ZERO_BASE_X, ZERO_BASE_Y+i*Y_GRID, ZERO_BASE_X+ARENA_WIDTH, ZERO_BASE_Y+i*Y_GRID, arcade.color.BLACK)
        arcade.draw_line(ZERO_BASE_X+i*X_GRID, ZERO_BASE_Y, ZERO_BASE_X+i*X_GRID, ZERO_BASE_Y+ARENA_HIGHT, arcade.color.BLACK)

    # info
    arcade.draw_text('Specification: FG(target) & G(!avoids)', 50, 456, arcade.color.BLACK, 14)
    arcade.draw_text('target', 405, 470, arcade.color.BLACK, 10)
    arcade.draw_rectangle_filled(395, 477, 10, 10, arcade.color.AVOCADO)

    arcade.draw_text('avoids', 405, 455, arcade.color.BLACK, 10)
    arcade.draw_rectangle_filled(395, 461, 10, 10, arcade.color.BARN_RED)
    
    




class My2dRobot(arcade.Window):

    def __init__(self, width, height, title):
        super().__init__(width, height, title)
        arcade.set_background_color(arcade.color.WHITE)

    def load_controller(self):
        filename = "robot.mdf"
        self.controller = Controller(filename)

    def setup(self):
        self.initial_delay = 100
        self.time_elapsed = 0.0
        self.avg_delta = 0.0
        self.robot_state = "stopped"
        self.step_time = 0.5
        self.move_step_x = 0.0
        self.move_step_y = 0.0
        self.robot_current_x = ROBOT_INITIAL_X
        self.robot_current_y = ROBOT_INITIAL_Y
        self.robot_next_x = ROBOT_INITIAL_X
        self.robot_next_y = ROBOT_INITIAL_Y
        self.last_action = -1
        self.robot = arcade.Sprite("robot.png", 0.05)
        self.robot.center_x = self.robot_current_x
        self.robot.center_y = self.robot_current_y
        self.load_controller()

    def get_current_symbol(self):
        x_sym = math.floor((self.robot_current_x-ROBOT_INITIAL_X)/X_GRID)
        y_sym = math.floor((self.robot_current_y-ROBOT_INITIAL_Y)/Y_GRID)
        return x_sym + y_sym*4

    def get_next_symbol(self):
        x_sym = math.floor((self.robot_next_x-ROBOT_INITIAL_X)/X_GRID)
        y_sym = math.floor((self.robot_next_y-ROBOT_INITIAL_Y)/Y_GRID)
        return x_sym + y_sym*4
    
    def print_info(self):
        txt  = "Status: " + self.robot_state
        txt += " | Time (sec.): " + str(round(self.time_elapsed))
        txt += " | FPS: " + str(round(1/self.avg_delta))
        txt += "\nCurrent: x_" + str(self.get_current_symbol()) + " = (" + str(round(self.robot_current_x)) + ", " + str(round(self.robot_current_y))
        txt += ") | Last action: "
        if self.last_action == 0:
            txt += "left"
        elif self.last_action == 1:
            txt += "up"
        elif self.last_action == 2:
            txt += "right"
        elif self.last_action == 3:
            txt += "down"
        else:
            txt += "none"
        txt += "\nNext: x_" + str(self.get_next_symbol()) + " = (" + str(round(self.robot_next_x)) + ", " + str(round(self.robot_next_y)) + ")"
        
        arcade.draw_text(txt, 50, 6, arcade.color.BLACK, 10)

    def on_draw(self):
        arcade.start_render()
        draw_arena()
        self.robot.draw()
        self.print_info()

    def reached(self):
        x_diff = abs(self.robot_current_x-self.robot_next_x)
        y_diff = abs(self.robot_current_y-self.robot_next_y)
        if x_diff <= abs(self.move_step_x)+1 and y_diff <= abs(self.move_step_y)+1:
            self.robot_current_x = self.robot_next_x
            self.robot_current_y = self.robot_next_y
            return True
        else:
            return False

    def update_move_steps(self):
        time_frac = self.step_time/self.avg_delta
        self.move_step_x = (self.robot_next_x-self.robot_current_x)/time_frac
        self.move_step_y = (self.robot_next_y-self.robot_current_y)/time_frac

    def move_left(self):
        self.robot_next_x = self.robot_current_x - X_GRID
        self.update_move_steps()
        self.robot_state = "moving"

    def move_up(self):
        self.robot_next_y = self.robot_current_y + Y_GRID
        self.update_move_steps()
        self.robot_state = "moving"

    def move_right(self):
        self.robot_next_x = self.robot_current_x + X_GRID
        self.update_move_steps()
        self.robot_state = "moving"

    def move_down(self):
        self.robot_next_y = self.robot_current_y - Y_GRID
        self.update_move_steps()
        self.robot_state = "moving"

#    def on_key_press(self, key, modifiers):
#        if self.robot_state == "stopped":
#            if key == arcade.key.UP:
#                self.move_up()
#            elif key == arcade.key.DOWN:
#                self.move_down()
#            elif key == arcade.key.LEFT:
#                self.move_left()
#            elif key == arcade.key.RIGHT:
#                self.move_right()

    def update_robot(self):

        if self.robot_state == "stopped":
            robot_sym_state = self.get_current_symbol()
            actions_list = self.controller.get_control_actions(robot_sym_state)
            self.last_action = actions_list[0]
            
            if self.last_action == 0:
                self.move_left()
            elif self.last_action == 1:
                self.move_up()
            elif self.last_action == 2:
                self.move_right()
            elif self.last_action == 3:
                self.move_down()
            else:
                pass

        self.robot_current_x += self.move_step_x
        self.robot_current_y += self.move_step_y
        self.robot.center_x = self.robot_current_x
        self.robot.center_y = self.robot_current_y

        if self.reached():
            self.robot_state = "stopped"         

    def update(self, delta_time):
    
        if self.avg_delta == 0.0 or self.initial_delay > 0:
            if self.avg_delta == 0.0:
                self.avg_delta = delta_time
            self.initial_delay -= 1
        else:
            self.update_robot()
            
        self.time_elapsed += delta_time
        self.avg_delta = (self.avg_delta + delta_time)/2.0


def main():
    game = My2dRobot(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Robot Example")
    game.setup()
    arcade.run()

if __name__ == "__main__":
    main()