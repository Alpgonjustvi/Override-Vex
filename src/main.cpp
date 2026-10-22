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
#define waitUntil(condition) \
  do                         \
  {                          \
    wait(5, msec);           \
  } while (!(condition))

#define repeat(iterations) \
  for (int iterator = 0; iterator < iterations; iterator++)
// END V5 MACROS



// generating and setting random seed
void initializeRandomSeed()
{
  int systemTime = Brain.Timer.systemHighResolution();
  double batteryCurrent = Brain.Battery.current();
  double batteryVoltage = Brain.Battery.voltage(voltageUnits::mV);

  // Combine these values into a single integer
  int seed = int(batteryVoltage + batteryCurrent * 100) + systemTime;

  // Set the seed
  srand(seed);
}

void vexcodeInit()
{

  // Initializing random seed.
  initializeRandomSeed();
}

// Helper to make playing sounds from the V5 in VEXcode easier and
// keeps the code cleaner by making it clear what is happening.
void playVexcodeSound(const char *soundName)
{
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

// Arm motors AYARLA!!!
motor ArmMotor1 = motor(PORT12, ratio36_1, true);
motor ArmMotor2 = motor(PORT14, ratio36_1, false);

// Drivetrain motors
motor TopLeft = motor(PORT1, ratio18_1, false);
motor BottomLeft = motor(PORT4, ratio18_1, false);

motor TopRight = motor(PORT2, ratio18_1, true);
motor BottomRight = motor(PORT3, ratio18_1, true);

// Motor groups
motor_group LeftDrive = motor_group(TopLeft, BottomLeft);
motor_group RightDrive = motor_group(TopRight, BottomRight);


// Pneumatic
digital_out Claw = digital_out(Brain.ThreeWirePort.A);

//CONSTS

float PIN_CM = 6.5;
float TOLERANCE = 1.0;
float CUP_CM = 6.48;


float MINI_GOAL_CM=8.25;
float MIDDLE_GOAL_CM=14.65;
float LARGE_GOAL_CM=22.27;

float MINARMREALHEIGHT = 8.0;

float DEGREE_TO_CM= 120.0/25.0;

float current_arm_height = 0;
float current_goal_cm = 0;
int arm_height_press_count = 0;

bool ISCLAWEXTENDED = false;

bool isSlowMode = false;

//TOLERANCE - h + G_current + pipe/2 + [ cup + (armstep-1) ]

motor_group Arm = motor_group(
    ArmMotor1,
    ArmMotor2
);

//func 
float getDegrees() {
  return Arm.position(rotationUnits::deg);
}
void alignToGoal() {
  float target_degrees = TOLERANCE - MINARMREALHEIGHT + current_goal_cm + (PIN_CM / 2) + (CUP_CM * (arm_height_press_count - 0));
  float catchDegrees = getDegrees();
  float error = target_degrees - catchDegrees;
    Arm.spinFor(vex::directionType::fwd, error, vex::rotationUnits::deg, true);

  /*
  if (catchDegrees < target_degrees) {
    Arm.spinFor(vex::directionType::fwd, target_degrees - catchDegrees, vex::rotationUnits::deg, false);
  } else if (catchDegrees > target_degrees) {
    Arm.spinFor(vex::directionType::rev, catchDegrees - target_degrees, vex::rotationUnits::deg, false);
  } else if (catchDegrees == target_degrees) {
    return;
  }
    daha gereksiz hali*/
}


void increaseArmStep() {
  //UYARI, KAC STEP ARTABİLECEĞİ BELLİ DEĞİL, OGRENDİKTEN SONRA LİMİTLEMEK LAZIM.
  arm_height_press_count++;
  alignToGoal();
  //Arm.spinFor(vex::directionType::fwd, CUP_CM * DEGREE_TO_CM, vex::rotationUnits::deg, false);
}
void decreaseArmStep() {
  if (arm_height_press_count > 0) {
    arm_height_press_count--; 
    alignToGoal();
    //Arm.spinFor(vex::directionType::rev, CUP_CM * DEGREE_TO_CM, vex::rotationUnits::deg, false);
} else {
    return;
  }
}
void resetArm() {
  float catchDegrees = getDegrees();
  Arm.spinFor(vex::directionType::rev, catchDegrees, vex::rotationUnits::deg, false);
  arm_height_press_count = 0;
}
void toggleClaw() {
  ISCLAWEXTENDED = !ISCLAWEXTENDED;
  Claw.set(ISCLAWEXTENDED);
}
void toggleSlowMode() {
    isSlowMode = !isSlowMode;
}

void configureBindings() {
  Controller1.ButtonB.pressed(
[]() {
    current_goal_cm = MINI_GOAL_CM;
  });
  Controller1.ButtonA.pressed(
[]() {
    current_goal_cm = MIDDLE_GOAL_CM;
  });
  Controller1.ButtonX.pressed(
[]() {
    current_goal_cm = LARGE_GOAL_CM;
  });
  Controller1.ButtonY.pressed(
    []() {
      toggleSlowMode();
    }
  );
  Controller1.ButtonL2.pressed(decreaseArmStep);
  Controller1.ButtonL1.pressed(increaseArmStep);
  Controller1.ButtonR1.pressed(resetArm);
  Controller1.ButtonR2.pressed(toggleClaw);
}

int main() {
    // Initialize robot
    //pin+cup 25cm
    //test. 90 derece == 33cm (-8 cm)
    // Set braking modes
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    Arm.setStopping(hold);
    Claw.set(false);
    configureBindings();
    
    


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

        // Apply slow mode scale (40% speed)
        if (isSlowMode) {
            leftSpeed *= 0.4;
            rightSpeed *= 0.4;
        }

        // Move drivetrain
        LeftDrive.spin(vex::forward, leftSpeed, percent);
        RightDrive.spin(vex::forward, rightSpeed, percent);

        
        

        // Small delay
        wait(20, msec);
    }
}
