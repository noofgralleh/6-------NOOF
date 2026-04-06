// ===== الحساسات =====
int sensors[6] = {A0, A1, A2, A3, A4, A5};
int s[6];

// ===== المعايرة =====
int minValues[6];
int maxValues[6];

// ===== المحركات =====
int in1 = 7;
int in2 = 8;
int in3 = 9;
int in4 = 10;

int ena = 5;
int enb = 6;

// ===== PID =====
float Kp = 30;
float Kd = 25;

int error = 0;
int lastError = 0;

// ===== السرعة =====
int baseSpeed = 230;

// ===== تعويض =====
float rightFactor = 0.80;

// ===== التمييز =====
unsigned long blackStartTime = 0;
bool onBlack = false;
int stopTime = 200; // إذا زاد عن هذا → مربع أسود

// ===== GAP =====
unsigned long lastSeenTime = 0;
int gapDelay = 130;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 6; i++) pinMode(sensors[i], INPUT);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(ena, OUTPUT);
  pinMode(enb, OUTPUT);

  calibrate();
}

void loop() {

  // ===== قراءة =====
  for (int i = 0; i < 6; i++) {
    int val = analogRead(sensors[i]);
    int th = (minValues[i] + maxValues[i]) / 2;
    s[i] = (val > th) ? 1 : 0;
  }

  // ===== عدد الأسود =====
  int blackCount = 0;
  for (int i = 0; i < 6; i++) {
    if (s[i]) blackCount++;
  }

  // =========================
  // ⬛ / ➕ التمييز
  // =========================
  if (blackCount >= 5) {

    if (!onBlack) {
      onBlack = true;
      blackStartTime = millis();
    }

    // إذا طول → مربع
    if (millis() - blackStartTime > stopTime) {
      moveMotors(0, 0);
      while (true);
    }

    // غير ذلك → تقاطع → كمل
    moveMotors(200, 200);
    return;

  } else {
    onBlack = false;
  }

  // =========================
  // GAP
  // =========================
  bool lost = true;
  for (int i = 0; i < 6; i++) {
    if (s[i]) lost = false;
  }

  if (!lost) lastSeenTime = millis();

  if (lost) {
    if (millis() - lastSeenTime < gapDelay) {
      moveMotors(200, 200);
    } else {
      if (lastError > 0)
        moveMotors(200, -150);
      else
        moveMotors(-150, 200);
    }
    return;
  }

  // =========================
  // PID
  // =========================
  error =
    (-5*s[0]) + (-3*s[1]) + (-1*s[2]) +
    ( 1*s[3]) + ( 3*s[4]) + ( 5*s[5]);

  int correction = Kp * error + Kd * (error - lastError);
  lastError = error;

  int left = baseSpeed + correction;
  int right = baseSpeed - correction;

  right *= rightFactor;

  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  moveMotors(left, right);
}

// ===== المحركات =====
void moveMotors(int leftSpeed, int rightSpeed) {

  if (leftSpeed >= 0) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
    leftSpeed = -leftSpeed;
  }

  if (rightSpeed >= 0) {
    digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  } else {
    digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
    rightSpeed = -rightSpeed;
  }

  analogWrite(ena, leftSpeed);
  analogWrite(enb, rightSpeed);
}

// ===== المعايرة =====
void calibrate() {
  for (int i = 0; i < 6; i++) {
    minValues[i] = 1023;
    maxValues[i] = 0;
  }

  for (int t = 0; t < 300; t++) {
    for (int i = 0; i < 6; i++) {
      int val = analogRead(sensors[i]);
      if (val < minValues[i]) minValues[i] = val;
      if (val > maxValues[i]) maxValues[i] = val;
    }
    delay(10);
  }
}
