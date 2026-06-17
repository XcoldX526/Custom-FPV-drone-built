#include <Wire.h>

// --- HARDWARE CONFIGURATION ---
const int MPU_ADDR = 0x68; // MPU6050 Gyro I2C Address
const int MOTOR_PINS[4] = {4, 5, 6, 7}; // Front-Left, Front-Right, Rear-Left, Rear-Right

// --- PID GAINS (Tweak these for your drone) ---
float Kp = 1.3, Ki = 0.04, Kd = 0.4;

// --- VARIABLES ---
float gyroX, gyroY, gyroZ;
float errorX, errorY, errorZ;
float integralX, integralY, integralZ;
float derivativeX, lastErrorX, lastErrorY, lastErrorZ;
float outX, outY, outZ;

// RadioMaster Input Targets (Assumes SBUS/ELRS mapped to 1000-2000ms parsed elsewhere)
// For Acro mode, sticks map directly to target rotation rates (degrees per second)
float targetRateX = 0, targetRateY = 0, targetRateZ = 0; 
int throttle = 1000; // Baseline collective power (1000 to 2000)

void setup() {
  Wire.begin();
  Serial.begin(115200);
  
  // Initialize MPU6050 Gyro
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // Power management register
  Wire.write(0x00); // Wake up MPU6050
  Wire.endTransmission();

  // Set motor pins as outputs
  for(int i = 0; i < 4; i++) pinMode(MOTOR_PINS[i], OUTPUT);
}

void loop() {
  // 1. Read Gyroscope Data (Raw Angular Rates)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43); // First register for Gyro Data (GYRO_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  
  gyroX = (Wire.read() << 8 | Wire.read()) / 65.5; // Convert to deg/s
  gyroY = (Wire.read() << 8 | Wire.read()) / 65.5;
  gyroZ = (Wire.read() << 8 | Wire.read()) / 65.5;

  // 2. Read RadioMaster Stick Inputs (Simulated placeholder - Map your RX channels here)
  // targetRateX = map(RadioMaster_Roll, 1000, 2000, -300, 300); // Max 300 deg/s roll

  // 3. ACRO PID LOOP (The core math stabilizing the quad)
  errorX = targetRateX - gyroX;
  errorY = targetRateY - gyroY;
  errorZ = targetRateZ - gyroZ;

  integralX += errorX * 0.004; // Assumes ~250Hz loop speed (4ms)
  derivativeX = (errorX - lastErrorX) / 0.004;
  outX = (Kp * errorX) + (Ki * integralX) + (Kd * derivativeX);
  
  // (Repeat math structure cleanly for Y and Z axes)
  outY = (Kp * errorY) + (Ki * integralY) + (Kd * (errorY - lastErrorY)) / 0.004;
  outZ = (Kp * errorZ) + (Ki * integralZ) + (Kd * (errorZ - lastErrorZ)) / 0.004;

  lastErrorX = errorX; lastErrorY = errorY; lastErrorZ = errorZ;

  // 4. MOTOR MIXER (Combines Throttle + PID corrections)
  int m1 = throttle + outY + outX - outZ; // Front-Left
  int m2 = throttle + outY - outX + outZ; // Front-Right
  int m3 = throttle - outY + outX + outZ; // Rear-Left
  int m4 = throttle - outY - outX - outZ; // Rear-Right

  // 5. Write Outputs to Motors (Constrained to standard ESC pulses)
  analogWrite(MOTOR_PINS[0], constrain(m1, 1000, 2000));
  analogWrite(MOTOR_PINS[1], constrain(m2, 1000, 2000));
  analogWrite(MOTOR_PINS[2], constrain(m3, 1000, 2000));
  analogWrite(MOTOR_PINS[3], constrain(m4, 1000, 2000));

  delay(4); // Maintain loop timing
}
