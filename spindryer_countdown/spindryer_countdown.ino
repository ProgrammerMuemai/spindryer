#include <Wire.h> // ไลบรารีสำหรับการสื่อสารผ่าน I2C
#include <LiquidCrystal_I2C.h> // ไลบรารีสำหรับจอ LCD I2C
#include <Keypad.h> // ไลบรารีสำหรับ Keypad

// ตั้งค่าหน้าจอ LCD (อาจต้องเปลี่ยนที่อยู่ I2C ถ้าไม่ตรงกัน)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// ตั้งค่า keypad
const byte ROWS = 4; // จำนวนแถวของ keypad
const byte COLS = 4; // จำนวนคอลัมน์ของ keypad
char keys[ROWS][COLS] = { // การแมพปุ่มใน keypad
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; // กำหนดขาที่เชื่อมต่อกับแถวของ keypad
byte colPins[COLS] = {5, 4, 3, 2}; // กำหนดขาที่เชื่อมต่อกับคอลัมน์ของ keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); // สร้างออบเจกต์ Keypad

int countdown_time = 0; // ตัวแปรเก็บเวลานับถอยหลัง
bool countdown_active = false; // สถานะของการนับถอยหลัง
bool countdown_paused = false; // สถานะของการหยุดชั่วคราว
unsigned long previousMillis = 0; // ตัวแปรเก็บเวลาล่าสุดที่อัปเดต
unsigned long interval = 1000; // หน่วงเวลา 1 วินาที
unsigned long resetTimeMillis = 0; // ตัวแปรเก็บเวลาที่เริ่มรีเซ็ต
bool resetInitiated = false; // สถานะของการรีเซ็ต
bool isMinutes = false; // สถานะสำหรับการแปลงเวลาเป็นนาที

const int relayPin = 11; // กำหนดขา relay

void setup() {
  lcd.init(); // เริ่มต้นจอ LCD
  lcd.backlight(); // เปิด backlight ของจอ LCD
  displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:" ที่กึ่งกลางแถวที่ 0

  pinMode(relayPin, OUTPUT); // กำหนดขา relay เป็นขาออก
  digitalWrite(relayPin, LOW); // ปิด relay ตั้งแต่เริ่มต้น (active high)

  Serial.begin(9600); // เริ่มต้น Serial สำหรับการ debug
}

void loop() {
  unsigned long currentMillis = millis(); // อ่านเวลาปัจจุบัน
  char key = keypad.getKey(); // อ่านค่าจาก keypad
  handleKeyInput(key); // จัดการค่าที่อ่านได้จาก keypad

  if (countdown_active && !countdown_paused) { // หากการนับถอยหลังทำงานและไม่อยู่ในสถานะหยุดชั่วคราว
    if (currentMillis - previousMillis >= interval) { // ตรวจสอบว่าผ่านไป 1 วินาทีหรือไม่
      previousMillis = currentMillis; // อัปเดตเวลาล่าสุด
      countdown_time--; // ลดค่าเวลานับถอยหลัง
      updateDisplay(); // อัปเดตการแสดงผล
      if (countdown_time <= 0) { // หากเวลานับถอยหลังหมด
        endCountdown(); // สิ้นสุดการนับถอยหลัง
      }
    }
  } else if (resetInitiated && (currentMillis - resetTimeMillis >= 2000)) { // หากการรีเซ็ตถูกเริ่มต้นและเวลาผ่านไป 2 วินาที
    resetCountdown(); // รีเซ็ตการนับถอยหลัง
  }
}

void handleKeyInput(char key) {
  if (key == 'D') { // หากกดปุ่ม D
    stopCountdown(); // หยุดการนับถอยหลัง
  } else if (key == 'C') { // หากกดปุ่ม C
    togglePause(); // สลับสถานะหยุดชั่วคราว
  } else if (!countdown_active && key) { // หากมีการกดปุ่มใด ๆ และการนับถอยหลังไม่ทำงาน
    if (key >= '0' && key <= '9') { // หากเป็นตัวเลข
      int new_time = countdown_time * 10 + (key - '0'); // คำนวณเวลานับถอยหลังใหม่
      if (isMinutes && new_time <= 60) { // หากอยู่ในโหมดนาทีและเวลาน้อยกว่าหรือเท่ากับ 60 นาที
        countdown_time = new_time; // อัปเดตเวลานับถอยหลัง
      } else if (!isMinutes && new_time <= 3600) { // หากอยู่ในโหมดวินาทีและเวลาน้อยกว่าหรือเท่ากับ 3600 วินาที
        countdown_time = new_time; // อัปเดตเวลานับถอยหลัง
      } else { // หากเกินขอบเขต
        displayOutOfRangeMessage(); // แสดงข้อความ "Out of range"
        return; // ยกเลิกการทำงาน
      }
      displayTime(); // แสดงเวลาปัจจุบัน
    } else if (key == '#') { // หากกดปุ่ม #
      if (countdown_time > 0) { // หากเวลานับถอยหลังมากกว่า 0
        startCountdown(); // เริ่มการนับถอยหลัง
      } else { // หากเวลานับถอยหลังเป็น 0
        displayMessageCentered("Set time first", 1); // แสดงข้อความ "Set time first"
        delay(2000); // หน่วงเวลา 2 วินาที
        lcd.clear(); // ล้างจอ LCD
        displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:"
      }
    } else if (key == '*') { // หากกดปุ่ม *
      clearTime(); // ล้างเวลา
    } else if (key == 'A') { // หากกดปุ่ม A
      if (countdown_time <= 3600) { // หากเวลานับถอยหลังน้อยกว่าหรือเท่ากับ 3600 วินาที
        isMinutes = false; // เปลี่ยนเป็นโหมดวินาที
        displayTime(); // แสดงเวลาปัจจุบัน
      } else { // หากเกินขอบเขต
        displayOutOfRangeMessage(); // แสดงข้อความ "Out of range"
      }
    } else if (key == 'B') { // หากกดปุ่ม B
      if (countdown_time <= 60) { // หากเวลานับถอยหลังน้อยกว่าหรือเท่ากับ 60 นาที
        isMinutes = true; // เปลี่ยนเป็นโหมดนาที
        displayTime(); // แสดงเวลาปัจจุบัน
      } else { // หากเกินขอบเขต
        displayOutOfRangeMessage(); // แสดงข้อความ "Out of range"
      }
    }
  }
}

void displayMessageCentered(String message, int row) {
  int pos = (16 - message.length()) / 2; // คำนวณตำแหน่งให้ข้อความอยู่กึ่งกลาง
  lcd.setCursor(pos, row); // ตั้งตำแหน่ง cursor
  lcd.print(message); // แสดงข้อความ
}

void displayOutOfRangeMessage() {
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Out of range", 1); // แสดงข้อความ "Out of range"
  delay(2000); // หน่วงเวลา 2 วินาที
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:"
  countdown_time = 0; // รีเซ็ตเวลานับถอยหลัง
}

void displayTime() {
  lcd.setCursor(0, 1); // ตั้งตำแหน่ง cursor ที่แถวที่ 1
  lcd.print("Time: "); // แสดงข้อความ "Time: "
  lcd.print(countdown_time); // แสดงเวลานับถอยหลัง
  lcd.print(isMinutes ? " min" : " s  "); // แสดงหน่วยของเวลา
}

void startCountdown() {
  if (isMinutes) {
    countdown_time *= 60; // แปลงนาทีเป็นวินาที
  }
  countdown_active = true; // ตั้งสถานะการนับถอยหลังเป็น true
  previousMillis = millis(); // เก็บเวลาปัจจุบัน
  lcd.clear(); // ล้างจอ LCD
  digitalWrite(relayPin, HIGH); // เปิด relay (active HIGH)
}

void stopCountdown() {
  countdown_active = false; // ตั้งสถานะการนับถอยหลังเป็น false
  countdown_paused = false; // ตั้งสถานะการหยุดชั่วคราวเป็น false
  countdown_time = 0; // รีเซ็ตเวลานับถอยหลัง
  isMinutes = false; // รีเซ็ตสถานะนาที
  digitalWrite(relayPin, LOW); // ปิด relay
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:"
}

void togglePause() {
  if (countdown_active) { // หากการนับถอยหลังทำงาน
    countdown_paused = !countdown_paused; // สลับสถานะการหยุดชั่วคราว
    if (countdown_paused) { // หากอยู่ในสถานะหยุดชั่วคราว
      digitalWrite(relayPin, LOW); // ปิด relay เมื่อหยุดชั่วคราว
      displayMessageCentered("Paused", 1); // แสดงข้อความ "Paused"
    } else { // หากไม่ได้อยู่ในสถานะหยุดชั่วคราว
      digitalWrite(relayPin, HIGH); // เปิด relay เมื่อเริ่มนับต่อ
      lcd.clear(); // ล้างจอ LCD
      updateDisplay(); // อัปเดตการแสดงผล
    }
  }
}

void clearTime() {
  countdown_time = 0; // รีเซ็ตเวลานับถอยหลัง
  isMinutes = false; // รีเซ็ตสถานะนาที
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:"
}

void updateDisplay() {
  lcd.clear(); // ล้างจอ LCD
  if (countdown_time > 0) { // หากเวลานับถอยหลังมากกว่า 0
    String time_str = String(countdown_time) + " s"; // แปลงเวลานับถอยหลังเป็น string พร้อมหน่วย
    int len = time_str.length(); // ความยาวของ string
    int pos = (16 - len) / 2; // คำนวณตำแหน่งให้ข้อความอยู่กึ่งกลาง
    displayMessageCentered("Time left:", 0); // แสดงข้อความ "Time left:"
    lcd.setCursor(pos, 1); // ตั้งตำแหน่ง cursor ที่แถวที่ 1
    lcd.print(time_str); // แสดงเวลานับถอยหลัง
  }
}

void endCountdown() {
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Time's up!", 0); // แสดงข้อความ "Time's up!"
  countdown_active = false; // ตั้งสถานะการนับถอยหลังเป็น false
  resetInitiated = true; // ตั้งสถานะการรีเซ็ตเป็น true
  resetTimeMillis = millis(); // เก็บเวลาปัจจุบัน
  digitalWrite(relayPin, LOW); // ปิด relay
}

void resetCountdown() {
  resetInitiated = false; // ตั้งสถานะการรีเซ็ตเป็น false
  lcd.clear(); // ล้างจอ LCD
  displayMessageCentered("Enter time:", 0); // แสดงข้อความ "Enter time:"
}
