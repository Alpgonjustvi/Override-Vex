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
motor ArmMotor1 = motor(PORT10, ratio36_1, true);
//motor ArmMotor2 = motor(PORT14, ratio36_1, false);

// Drivetrain motors
motor TopLeft = motor(PORT1, ratio18_1, false);
motor BottomLeft = motor(PORT4, ratio18_1, false);

motor TopRight = motor(PORT2, ratio18_1, true);
motor BottomRight = motor(PORT3, ratio18_1, true);

// Motor groups
motor_group LeftDrive = motor_group(TopLeft, BottomLeft);
motor_group RightDrive = motor_group(TopRight, BottomRight);

motor_group Arm = motor_group(ArmMotor1);//, ArmMotor2);

// Pneumatic
digital_out Claw = digital_out(Brain.ThreeWirePort.A);

// =========================
// CONSTS
// =========================

const float PIN_CM = 6.5;
const float TOLERANCE = 1.0;
const float CUP_CM = 6.48;

const float MINI_GOAL_CM = 8.25;
const float MIDDLE_GOAL_CM = 14.65;
const float LARGE_GOAL_CM = 22.27;

const float ARCADE_SLOW_SUPPLIER = 0.28;
const float MINARMREALHEIGHT = 8.0;

const float DEG_PER_CM = 90.0 / 16.0;

const int MAX_ARM_STEP = 6;
const float MAX_ARM_DEG = 600.0;
const float ARM_VELOCITY = 80.0; // percent

float arcade_supplier = 1.0;

float current_goal_cm = 0;
int arm_height_press_count = 0;

bool ISCLAWEXTENDED = false;
bool isSlowMode = false;

// =========================
// FUNCS
// =========================

float getDegrees()
{
  return Arm.position(rotationUnits::deg);
}

// TOLERANCE - MINARMREALHEIGHT + goal + pin/2 + cup*stepSayisi
void alignToGoal()
{
  if (current_goal_cm <= 0)
    return;

  float target_cm = TOLERANCE - MINARMREALHEIGHT + current_goal_cm + (PIN_CM / 2.0) + (CUP_CM * arm_height_press_count);
  float target_degrees = target_cm * DEG_PER_CM;
  // Limitle
  /*
  if (target_degrees < 0)
    target_degrees = 0;
  if (target_degrees > MAX_ARM_DEG)
    target_degrees = MAX_ARM_DEG;
*/
  Controller1.Screen.clearScreen();
  wait(50, msec); 
  Controller1.Screen.setCursor(1, 1);
  Controller1.Screen.print("T: %.3fcm %.3fdeg", target_cm + MINARMREALHEIGHT, target_degrees);

  Arm.spinToPosition(target_degrees, rotationUnits::deg, false); // spinFor
}

void increaseArmStep()
{
  if (current_goal_cm <= 0)
    return; // Önce goal seç
  if (arm_height_press_count >= MAX_ARM_STEP)
    return;
  arm_height_press_count++;
  alignToGoal();
}

void decreaseArmStep()
{
  if (current_goal_cm <= 0)
    return;
  arm_height_press_count--;
  alignToGoal();
}

void resetArm()
{
  arm_height_press_count = 0;
  current_goal_cm = 0;
  Arm.spinToPosition(0, rotationUnits::deg, false);
}

void toggleClaw()
{
  ISCLAWEXTENDED = !ISCLAWEXTENDED;
  Claw.set(ISCLAWEXTENDED);
}

void toggleSlowMode() {
    isSlowMode = !isSlowMode;
}

void configureBindings() {
  Controller1.ButtonB.pressed(
[]() {
    alignToGoal(MINI_GOAL_CM);
  });
  Controller1.ButtonA.pressed(
[]() {
    alignToGoal(MIDDLE_GOAL_CM);
  });
  Controller1.ButtonX.pressed(
[]() {
    alignToGoal(LARGE_GOAL_CM);
  });
  Controller1.ButtonY.pressed(toggleSlowMode);
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
    vexcodeInit();
    Arm.resetPosition();
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
            leftSpeed *= 0.28;
            rightSpeed *= 0.28;
        }

        // Move drivetrain
        LeftDrive.spin(vex::forward, leftSpeed, percent);
        RightDrive.spin(vex::forward, rightSpeed, percent);

        
        

        // Small delay
        wait(20, msec);
    }
}
