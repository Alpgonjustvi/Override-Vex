#include "main.h"

/////
// For installation, upgrading, documentations, and tutorials, check out our website!
// https://ez-robotics.github.io/EZ-Template/
/////

// Chassis constructor
ez::Drive chassis({-1, -4},  // Left Chassis Ports (negative port will reverse it!)
                  {2, 3},    // Right Chassis Ports (negative port will reverse it!)
                  7,         // IMU Port
                  4.125,     // Wheel Diameter
                  343        // Wheel RPM
);

// Motor Grupları (Sağ/Sol yön ayarları)
pros::MotorGroup Arm({-10, 6, 11, -13}, pros::MotorGearset::green);

// Pnömatikler
ez::Piston Claw('A');
ez::Piston Toggle('B');


// Değişkenler ve Sabitler
int arm_height_press_count = 0;
double target_position;  // cm
double current_position;

const float PIN_CM = 6.5;
const float TOLERANCE = 1.0;
const float CUP_CM = 6.48;

const float MINI_GOAL_CM = 8.25;
const float MIDDLE_GOAL_CM = 14.65;
const float LARGE_GOAL_CM = 22.27;

const float REALARMHEIGHTFROMFLOOR = 8.0;
const float DEG_PER_CM = 140.0 / 32.5;

const int MAX_ARM_STEP = 6;
const float ARM_VELOCITY = 127;

double current_goal_cm = MINI_GOAL_CM;  // Başlangıçta güvenli bir hedef atandı

// Kol Hedef Hesaplama ve Harekete Geçirme Fonksiyonu
void alignToGoal() {
  if (current_goal_cm <= 0) return;

  float target_cm = TOLERANCE - REALARMHEIGHTFROMFLOOR + current_goal_cm + (PIN_CM / 2.0) + (CUP_CM * arm_height_press_count);
  float target_degrees = target_cm * DEG_PER_CM;

  Arm.move_absolute(target_degrees, ARM_VELOCITY);
}

void increaseStep() {
  if (current_goal_cm <= 0) return;
  if (arm_height_press_count >= MAX_ARM_STEP) return;
  arm_height_press_count++;
  alignToGoal();
}

void decreaseStep() {
  if (current_goal_cm <= 0) return;
  if (arm_height_press_count <= 0) return;  // Negatif adıma düşmeyi engeller
  arm_height_press_count--;
  alignToGoal();
}

void selectGoal(float goal_cm) {
  current_goal_cm = goal_cm;
  arm_height_press_count = 0;  // Hedef değişince adımı sıfırla
  alignToGoal();
}

void initialize() {
  ez::ez_template_print();
  pros::delay(500);

  // Kol motorlarının fren modunu hold yapıp pozisyonu sıfırlıyoruz
  Arm.set_brake_mode(pros::MotorBrake::hold);
  Arm.tare_position();

  chassis.opcontrol_drive_activebrake_set(0.0);
  chassis.opcontrol_curve_default_set(0.0, 0.0);

  default_constants();

  // Otonom Seçici (Auton Selector)
  ez::as::auton_selector.autons_add({
      {"Drive\n\nDrive forward and come back", drive_example},
      {"Turn\n\nTurn 3 times.", turn_example},
      {"Drive and Turn\n\nDrive forward, turn, come back", drive_and_turn},
      {"Drive and Turn\n\nSlow down during drive", wait_until_change_speed},
      {"Swing Turn\n\nSwing in an 'S' curve", swing_example},
      {"Motion Chaining\n\nDrive forward, turn, and come back, but blend everything together :D", motion_chaining},
      {"Combine all 3 movements", combining_movements},
      {"Interference\n\nAfter driving forward, robot performs differently if interfered or not", interfered_example},
      {"Simple Odom\n\nThis is the same as the drive example, but it uses odom instead!", odom_drive_example},
      {"Pure Pursuit\n\nGo to (0, 30) and pass through (6, 10) on the way.  Come back to (0, 0)", odom_pure_pursuit_example},
      {"Pure Pursuit Wait Until\n\nGo to (24, 24) but start running an intake once the robot passes (12, 24)", odom_pure_pursuit_wait_until_example},
      {"Boomerang\n\nGo to (0, 24, 45) then come back to (0, 0, 0)", odom_boomerang_example},
      {"Boomerang Pure Pursuit\n\nGo to (0, 24, 45) on the way to (24, 24) then come back to (0, 0, 0)", odom_boomerang_injected_pure_pursuit_example},
      {"Measure Offsets\n\nThis will turn the robot a bunch of times and calculate your offsets for your tracking wheels.", measure_offsets},
  });

  chassis.initialize();
  ez::as::initialize();
  master.rumble(chassis.drive_imu_calibrated() ? "." : "---");
}

void disabled() {}

void competition_initialize() {}

void autonomous() {
  chassis.pid_targets_reset();
  chassis.drive_imu_reset();
  chassis.drive_sensor_reset();
  chassis.odom_xyt_set(0_in, 0_in, 0_deg);
  chassis.drive_brake_set(MOTOR_BRAKE_HOLD);

  ez::as::auton_selector.selected_auton_call();
}

void screen_print_tracker(ez::tracking_wheel* tracker, std::string name, int line) {
  std::string tracker_value = "", tracker_width = "";
  if (tracker != nullptr) {
    tracker_value = name + " tracker: " + ez::util::to_string_with_precision(tracker->get());
    tracker_width = "  width: " + ez::util::to_string_with_precision(tracker->distance_to_center_get());
  }
  ez::screen_print(tracker_value + tracker_width, line);
}

void ez_motor_temperatures() {
  if (chassis.left_motors.size() * 2 < 12) {
    for (int i = 0; i < chassis.left_motors.size(); i++) {
      ez::screen_print("Motor " + std::to_string(chassis.left_motors[i].get_port()) + ": " +
                           std::to_string(static_cast<int>(chassis.left_motors[i].get_temperature())) + " *C*\t" + "Motor " +
                           std::to_string(chassis.right_motors[i].get_port()) + ": " +
                           std::to_string(static_cast<int>(chassis.right_motors[i].get_temperature())) + " *C",
                       i);
    }
  }
}

void ez_screen_task() {
  while (true) {
    if (!pros::competition::is_connected()) {
      if (chassis.odom_enabled() && !chassis.pid_tuner_enabled()) {
        if (ez::as::page_blank_is_on(0)) {
          ez::screen_print("x: " + ez::util::to_string_with_precision(chassis.odom_x_get()) +
                               "\ny: " + ez::util::to_string_with_precision(chassis.odom_y_get()) +
                               "\na: " + ez::util::to_string_with_precision(chassis.odom_theta_get()),
                           1);
          screen_print_tracker(chassis.odom_tracker_left, "l", 4);
          screen_print_tracker(chassis.odom_tracker_right, "r", 5);
          screen_print_tracker(chassis.odom_tracker_back, "b", 6);
          screen_print_tracker(chassis.odom_tracker_front, "f", 7);
        } else if (ez::as::page_blank_is_on(1))
          ez_motor_temperatures();
      }
    } else {
      if (ez::as::page_blank_amount() > 0) ez::as::page_blank_remove_all();
    }
    pros::delay(ez::util::DELAY_TIME);
  }
}
pros::Task ezScreenTask(ez_screen_task);

void ez_template_extras() {
  if (!pros::competition::is_connected()) {
    if (master.get_digital_new_press(DIGITAL_X)) chassis.pid_tuner_toggle();

    if (master.get_digital(DIGITAL_B) && master.get_digital(DIGITAL_DOWN)) {
      pros::motor_brake_mode_e_t preference = chassis.drive_brake_get();
      autonomous();
      chassis.drive_brake_set(preference);
    }
    chassis.pid_tuner_iterate();
  } else {
    if (chassis.pid_tuner_enabled()) chassis.pid_tuner_disable();
  }
}

void configureBindings() {
  // Adım kontrolü (L1 / L2)
  if (master.get_digital_new_press(DIGITAL_L1)) {
    increaseStep();
  }
  if (master.get_digital_new_press(DIGITAL_L2)) {
    decreaseStep();
  }
  /*
  // Hedef Boyut Seçimi (B, A, X tuşları)
  if (master.get_digital_new_press(DIGITAL_B)) {
      selectGoal(MINI_GOAL_CM);
  }
  if (master.get_digital_new_press(DIGITAL_A)) {
      selectGoal(MIDDLE_GOAL_CM);
  }
  if (master.get_digital_new_press(DIGITAL_X)) {
      selectGoal(LARGE_GOAL_CM);
  }
      */
}

void opcontrol() {
  chassis.drive_brake_set(MOTOR_BRAKE_BRAKE);
  Arm.set_brake_mode(pros::MotorBrake::hold);

  // Başlangıçta garanti olsun diye Mini Goal seçili başlatalım
  if (current_goal_cm <= 0) {
    current_goal_cm = MINI_GOAL_CM;
  }

  while (true) {
    configureBindings();
    ez_template_extras();
    chassis.opcontrol_arcade_standard(ez::SPLIT);

    // --- CANLI HESAPLAMA VE TERMİNAL LOGLAMA ---
    float target_cm = TOLERANCE - REALARMHEIGHTFROMFLOOR + current_goal_cm + (PIN_CM / 2.0) + (CUP_CM * arm_height_press_count);
    float target_degrees = target_cm * DEG_PER_CM;
    double current_deg = Arm.get_position();

    // Sadece terminale yazdırıyoruz (LVGL çökmesini önler)
    printf("AHedef CM: %.1f | Adim: %d | Hedef Deg: %.1f | Mevcut Deg: %.1f\n", current_goal_cm, arm_height_press_count, target_degrees, current_deg);

    pros::delay(ez::util::DELAY_TIME);
  }
}
