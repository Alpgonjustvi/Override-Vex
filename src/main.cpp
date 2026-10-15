#pragma region VEXcode Generated Robot Configuration
// Make sure all required headers are included.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>


#include "vex.h"

using namespace vex;

// Brain should be defined by default
brain Brain;


// START V5 MACROS
#define waitUntil(condition)                                                   \
  do {                                                                         \
    wait(5, msec);                                                             \
  } while (!(condition))

#define repeat(iterations)                                                     \
  for (int iterator = 0; iterator < iterations; iterator++)
// END V5 MACROS


// Robot configuration code.
controller Controller1 = controller(primary);


// generating and setting random seed
void initializeRandomSeed(){
  int systemTime = Brain.Timer.systemHighResolution();
  double batteryCurrent = Brain.Battery.current();
  double batteryVoltage = Brain.Battery.voltage(voltageUnits::mV);

  // Combine these values into a single integer
  int seed = int(batteryVoltage + batteryCurrent * 100) + systemTime;

  // Set the seed
  srand(seed);
}



void vexcodeInit() {

  //Initializing random seed.
  initializeRandomSeed(); 
}


// Helper to make playing sounds from the V5 in VEXcode easier and
// keeps the code cleaner by making it clear what is happening.
void playVexcodeSound(const char *soundName) {
  printf("VEXPlaySound:%s\n", soundName);
  wait(5, msec);
}



// define variable for remote controller enable/disable
bool RemoteControlCodeEnabled = true;

#pragma endregion VEXcode Generated Robot Configuration

#include "vex.h"

using namespace vex;

// Controller
controller Controller1 = controller(primary);

// Arm motors
motor ArmMotor1 = motor(PORT11, ratio18_1, true);
motor ArmMotor2 = motor(PORT12, ratio18_1, false);
motor ArmMotor3 = motor(PORT13, ratio18_1, false);
motor ArmMotor4 = motor(PORT14, ratio18_1, true);

// Drivetrain motors
motor TopLeft = motor(PORT1, ratio18_1, false);
motor BottomLeft = motor(PORT4, ratio18_1, false);

motor TopRight = motor(PORT2, ratio18_1, true);
motor BottomRight = motor(PORT3, ratio18_1, true);

// Motor groups
motor_group LeftDrive = motor_group(TopLeft, BottomLeft);
motor_group RightDrive = motor_group(TopRight, BottomRight);



//CONSTS

int MINI_GOAL_CM=3.25;
int MIDDLE_GOAL_CM=3.25;
int LARGE_GOAL_CM=3.25;

int PIPE_CM=6.5
int TOLERANCE=1.0
int CUP_CM=6.48

int current_goal_cm = 0
int arm_height_press_count = 0


motor_group Arm = motor_group(
    ArmMotor1,
    ArmMotor2,
    ArmMotor3,
    ArmMotor4
);

int main() {
    // Initialize robot
    //pin+cup 25cm
    //test. 90 derece == 33cm
    int ayar = 0; 
    // Set braking modes
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    Arm.setStopping(hold);







    while (true) {

        // =========================
        // ARCADE DRIVETRAIN
        // =========================

        // Left joystick: forward/backward (inverted)
        int driveSpeed = -Controller1.Axis3.position(percent);

        // Right joystick: turning (inverted)
        int turnSpeed = -Controller1.Axis1.position(percent);

        // Deadband
        if (abs(driveSpeed) < 5) driveSpeed = 0;
        if (abs(turnSpeed) < 5) turnSpeed = 0;

        // Calculate motor speeds
        int leftSpeed = driveSpeed + turnSpeed;
        int rightSpeed = driveSpeed - turnSpeed;

        // Clamp speeds to -100% to 100%
        if (leftSpeed > 100) leftSpeed = 100;
        if (leftSpeed < -100) leftSpeed = -100;

        if (rightSpeed > 100) rightSpeed = 100;
        if (rightSpeed < -100) rightSpeed = -100;

        // Move drivetrain
        LeftDrive.spin(vex::forward, leftSpeed, percent);
        RightDrive.spin(vex::forward, rightSpeed, percent);

        // =========================
        // ARM CONTROLS
        // =========================

        
        else if (Controller1.ButtonDown.pressing()) {
          if (ayar > 0)
              ayar =-1;
              ArmMotor1.spinFor(reverse, 44, degrees);
        else if (Controller1.ButtonA.pressed() {

          Arm.spinFor(forward, 82, degrees)
        
        }
        else {
            // Hold arm position
            Arm.stop(hold);
        }

        // Small delay
        wait(20, msec);
    }
}
