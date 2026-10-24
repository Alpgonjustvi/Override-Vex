const readline = require('readline');

// Sabitler (C++ kodundaki güncel değerler)
const MINI_GOAL_CM = 3;
const MIDDLE_GOAL_CM = 4;
const LARGE_GOAL_CM = 5;

const PIN_CM = 6;
const TOLERANCE = 1.0;
const CUP_CM = 6;
const MINARMREALHEIGHT = 8.0;

// Değişkenler
let current_goal_cm = 0;
let arm_height_press_count = 0;

function calculateTarget() {
  return TOLERANCE - MINARMREALHEIGHT + current_goal_cm + (PIN_CM / 2) + (CUP_CM * (arm_height_press_count - 0));
}

function printState() {
  const height = calculateTarget();
  console.log(`[GOAL: ${current_goal_cm} cm] | [COUNT: ${arm_height_press_count}] | [ARM HEIGHT/TARGET: ${height.toFixed(2)}]`);
}""

// Konsol tuş okuma ayarları
readline.emitKeypressEvents(process.stdin);
if (process.stdin.isTTY) {
  process.stdin.setRawMode(true);
}

console.log("--- VEX Arm Simülatörü ---");
console.log("1: Mini Goal, 2: Middle Goal, 3: Large Goal");
console.log("Yukarı Ok: Count++ | Aşağı Ok: Count--");
console.log("0: Reset Arm");
console.log("Çıkış için Ctrl+C\n");

printState();

process.stdin.on('keypress', (str, key) => {
  if (key.ctrl && key.name === 'c') {
    process.exit();
  }

  if (str === '1') {
    current_goal_cm = MINI_GOAL_CM;
  } else if (str === '2') {
    current_goal_cm = MIDDLE_GOAL_CM;
  } else if (str === '3') {
    current_goal_cm = LARGE_GOAL_CM;
  } else if (str === '0') {
    arm_height_press_count = 0;
    current_goal_cm = 0;
  } else if (key.name === 'up') {
    arm_height_press_count++;
  } else if (key.name === 'down') {
    if (arm_height_press_count > 0) {
      arm_height_press_count--;
    }
  }

  printState();
});
