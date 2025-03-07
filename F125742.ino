#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <MemoryFree.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define DEBUG  // Commenting out this line will disable the debug function

#ifdef DEBUG
#define debug_print(x) \
  Serial.print("DEBUG: "); \
  Serial.print(x);
// Prints "DEBUG: " at the first debug statement
#define debug_valprint(x) Serial.print(x);
// Prints values without "DEBUG: " in front of them
#define debug_println(x) \
  Serial.print("DEBUG: "); \
  Serial.println(x);
#define debug_valprintln(x) Serial.println(x);
#else
#endif

// The codes that didn't work but did my best to try start and end with "// *"

enum states { SYNC,
              STANDBY,
              READ,
              BUTTON,
              DISP };  // The payroll account system consists of 5 states

struct payroll_account {  // An account consists of ID, Grade, Job Title, Salary and Pension
  unsigned long id;
  char grade;
  String title;
  float salary = 0.00;
  bool pension = true;
} emp;

const uint8_t maxEmp = 28;        // Maximum number of employees
payroll_account empData[maxEmp];  // Initialising the employee account array
uint8_t empIndex = 0;             // Initialising employee account array index
static int8_t currentIndex = 0;   // Initialising current index to work with buttons and display

String strId;  // Initialising string values obtained from string input
String strGrd;
String strTit;
String strPen;
String strSal;

String input = "";  // Input from serial monitor stored in String values

bool idCheck(String strId) {  // Checks whether the ID is exactly 7 digits and numeric
  if (strId.length() != 7) {
    Serial.println(F("ERROR: ID should be exactly 7 digits"));
    return false;
  }
  for (uint8_t i = 0; i < strId.length(); i++) {
    if (!isDigit(strId[i])) {
      Serial.println(F("ERROR: ID should be numeric"));
      return false;
    }
  }
  return true;
}

bool gradeCheck(String strGrd) {  // Checks whether the grade is single number between 1-9
  if (strGrd.length() != 1 || !isDigit(strGrd.charAt(0)) || strGrd == "0") {
    Serial.println(F("ERROR: Job grade should be 1-9"));
    return false;
  }
  return true;
}

bool titleCheck(String strTit) {  // Checks whether the job title is 3-17 characters long and consists of A-Z, 0-9, ., _
  if (strTit.length() < 3 || strTit.length() > 17) {
    Serial.println(F("ERROR: Job title should be 3-17 characters long"));
    return false;
  }
  for (uint8_t i = 0; i < strTit.length(); i++) {
    char t = strTit[i];
    if (!isAlphaNumeric(t) && t != '.' && t != '_') {
      Serial.println(F("ERROR: Job title can only contain A-Z, 0-9, '.' and '_'"));
      return false;
    }
  }
  return true;
}

bool salaryCheck(String strSal) {  // Checks whether the salary is within the range of 0.00-99999.99
  float sal = strSal.toFloat();
  if (sal >= 0.00 && sal <= 99999.99) {
    return true;
  } else {
    Serial.println(F("ERROR: Salary range should be 0.00 to 99999.99"));
    return false;
  }
}

void ADD(String input, int8_t firstDash, int8_t secondDash, int8_t thirdDash) {  // Adds new employee account such as ID, Grade and Job title
  if (empIndex < maxEmp) {
    if (firstDash == -1 || secondDash == -1 || thirdDash == -1) {
      Serial.println(F("ERROR: Invalid format -> Change to ADD-ID-GRD-TITLE"));
      return;
    }

    strId = input.substring(firstDash + 1, secondDash);
    if (!idCheck(strId)) {
      return;
    }

    strGrd = input.substring(secondDash + 1, thirdDash);
    if (!gradeCheck(strGrd)) {
      return;
    }

    strTit = input.substring(thirdDash + 1);
    if (!titleCheck(strTit)) {
      return;
    }

    unsigned long id = strId.toInt();

    for (uint8_t i = 0; i < empIndex; i++) {
      if (empData[i].id == id) {
        Serial.println(F("ERROR: You are adding an existing ID"));
        return;
      }
    }

    emp.id = strId.toInt();
    emp.grade = strGrd.charAt(0);
    emp.title = strTit;
    empData[empIndex] = emp;
    empIndex++;

    for (uint8_t i = 0; i < empIndex - 1; i++) {  // Using bubble sort to sort the ID in numerical order
      for (uint8_t j = 0; j < empIndex - i - 1; j++) {
        if (empData[j].id > empData[j + 1].id) {
          payroll_account temp = empData[j];
          empData[j] = empData[j + 1];
          empData[j + 1] = temp;
        }
      }
    }

    debug_println(F("<Current Employee List (Sorted by ID)>"));  // Prints the sorted ID list on serial monitor
    for (uint8_t i = 0; i < empIndex; i++) {
      debug_print(F("ID: "));
      debug_valprint(empData[i].id);
      debug_valprint(F(", Grade: "));
      debug_valprint(empData[i].grade);
      debug_valprint(F(", Title: "));
      debug_valprint(empData[i].title);
      debug_valprint(F(", Pension: "));
      debug_valprint(empData[i].pension);
      debug_valprint(F(", Salary: "));
      debug_valprintln(empData[i].salary);
    }
    Serial.println(F("DONE!"));
  } else {
    Serial.println(F("ERROR: EmpData is full"));
  }
}

void PST(String input, int8_t firstDash, int8_t secondDash) {  // Updates pension status of an ID that user wants to change
  if (firstDash == -1 || secondDash == -1) {
    Serial.println(F("ERROR: Invalid format -> Change to PST-ID-PEN"));
    return;
  }

  strId = input.substring(firstDash + 1, secondDash);
  if (!idCheck(strId)) {
    return;
  }

  strPen = input.substring(secondDash + 1);
  bool penSts;

  if (strPen == "PEN") {
    penSts = true;
  } else if (strPen == "NPEN") {
    penSts = false;
  } else {
    Serial.println(F("ERROR: Wrong pension input"));
    return;
  }

  bool match = false;
  unsigned long id = strId.toInt();

  for (uint8_t i = 0; i < maxEmp; i++) {
    if (empData[i].id == id) {
      if (empData[i].pension == penSts) {
        Serial.println(F("ERROR: You have put the same pension"));
        return;
      } else if (empData[i].salary == 0.00) {
        Serial.println(F("ERROR: The employee's salary is still 0.00"));
        return;
      } else {
        empData[i].pension = penSts;
        match = true;
        debug_print(F("Updated Pension Status for ID "));  // Prints the updated pension status for the ID on serial monitor
        debug_valprint(empData[i].id);
        debug_valprint(F(": "));
        debug_valprintln(empData[i].pension ? "PEN" : "NPEN");
        Serial.println(F("DONE!"));
        return;
      }
    }
  }
  if (!match) {
    Serial.println(F("ERROR: The ID doesn't exist"));
    return;
  }
}

void GRD(String input, int8_t firstDash, int8_t secondDash) {  // Updates grade of an ID that user wants to change
  if (firstDash == -1 || secondDash == -1) {
    Serial.println(F("ERROR: Invalid format -> Change to GRD-ID-GRD"));
    return;
  }

  strId = input.substring(firstDash + 1, secondDash);
  if (!idCheck(strId)) {
    return;
  }

  strGrd = input.substring(secondDash + 1);
  if (!gradeCheck(strGrd)) {
    return;
  }

  unsigned long id = strId.toInt();
  bool match = false;

  for (uint8_t i = 0; i < maxEmp; i++) {
    if (empData[i].id == id) {
      if (strGrd.charAt(0) <= empData[i].grade) {
        Serial.println(F("ERROR: Changed grade can't be equal or lower than the previous one"));
        return;
      } else {
        empData[i].grade = strGrd.charAt(0);
        match = true;
        debug_print(F("Updated Grade for ID "));  // Prints the updated grade for the ID on serial monitor
        debug_valprint(empData[i].id);
        debug_valprint(F(": "));
        debug_valprintln(empData[i].grade);
        Serial.println(F("DONE!"));
        return;
      }
    }
  }
  if (!match) {
    Serial.println(F("ERROR: The ID doesn't exist"));
    return;
  }
}

void SAL(String input, int8_t firstDash, int8_t secondDash) {  // Updates salary of an ID that user wants to change
  if (firstDash == -1 || secondDash == -1) {
    Serial.println(F("ERROR: Invalid format -> Change to SAL-ID-SAL"));
    return;
  }

  strId = input.substring(firstDash + 1, secondDash);
  if (!idCheck(strId)) {
    return;
  }

  strSal = input.substring(secondDash + 1);
  if (!salaryCheck(strSal)) {
    return;
  }

  unsigned long id = strId.toInt();
  bool match = false;
  float salary = strSal.toFloat();
  salary = round(salary * 100) / 100.0;

  for (uint8_t i = 0; i < maxEmp; i++) {
    if (empData[i].id == id) {
      empData[i].salary = strSal.toFloat();
      match = true;
      debug_print(F("Updated Salary for ID "));  // Prints the updated salary for the ID on serial monitor
      debug_valprint(empData[i].id);
      debug_valprint(F(": "));
      debug_valprintln(empData[i].salary);
      Serial.println(F("DONE!"));
      return;
    }
  }
  if (!match) {
    Serial.println(F("ERROR: The ID doesn't exist"));
    return;
  }
}

void CJT(String input, int8_t firstDash, int8_t secondDash) {  // Updates job title of an ID that user wants to change
  if (firstDash == -1 || secondDash == -1) {
    Serial.println(F("ERROR: Invalid format -> Change to CJT-ID-JT"));
    return;
  }

  strId = input.substring(firstDash + 1, secondDash);
  if (!idCheck(strId)) {
    return;
  }

  strTit = input.substring(secondDash + 1);
  if (!titleCheck(strTit)) {
    return;
  }

  unsigned long id = strId.toInt();
  bool match = false;

  for (uint8_t i = 0; i < maxEmp; i++) {
    if (empData[i].id == id) {
      empData[i].title = strTit;
      match = true;
      debug_print(F("Updated Job Title for ID "));  // Prints the updated job title for the ID on serial monitor
      debug_valprint(empData[i].id);
      debug_valprint(F(": "));
      debug_valprintln(empData[i].title);
      Serial.println(F("DONE!"));
      return;
    }
  }
  if (!match) {
    Serial.println(F("ERROR: The ID doesn't exist"));
    return;
  }
}

void DEL(String input, int8_t firstDash) {  // Deletes a selected employee's payroll account
  if (firstDash == -1) {
    Serial.println(F("ERROR: Invalid format -> Change to DEL-ID"));
    return;
  }

  strId = input.substring(firstDash + 1);
  if (!idCheck(strId)) {
    return;
  }

  if (empIndex == 1) {  // Check if only one account exists
    Serial.println(F("ERROR: You can't delete the last account"));
    return;
  }

  unsigned long id = strId.toInt();
  bool match = false;

  for (uint8_t i = 0; i < maxEmp; i++) {  // If the ID is existing, then remove the employee's account from the empData array and shift the rest of the accounts by 1
    if (empData[i].id == id) {
      match = true;
      for (uint8_t j = i; j < empIndex - 1; j++) {
        empData[j] = empData[j + 1];
      }
      empIndex--;
      if (currentIndex >= empIndex) {
        currentIndex = empIndex - 1;
      }
      break;
    }
  }

  if (match) {
    for (uint8_t i = 0; i < empIndex - 1; i++) {  // Using bubble sort to sort the ID in numerical order
      for (uint8_t j = 0; j < empIndex - i - 1; j++) {
        if (empData[j].id > empData[j + 1].id) {
          payroll_account temp = empData[j];
          empData[j] = empData[j + 1];
          empData[j + 1] = temp;
        }
      }
    }


    debug_println(F("<Current Employee List After Deleting(Sorted by ID)>"));  // Prints the sorted ID list after deleting on serial monitor
    for (uint8_t i = 0; i < empIndex; i++) {
      debug_print(F("ID: "));
      debug_valprint(empData[i].id);
      debug_valprint(F(", Grade: "));
      debug_valprint(empData[i].grade);
      debug_valprint(F(", Title: "));
      debug_valprint(empData[i].title);
      debug_valprint(F(", Pension: "));
      debug_valprint(empData[i].pension);
      debug_valprint(F(", Salary: "));
      debug_valprintln(empData[i].salary);
    }
    Serial.println(F("DONE!"));
  }

  if (!match) {
    Serial.println(F("ERROR: The ID doesn't exist"));
    return;
  }
}

byte customUp[8] = {  // UDCHARS nice looking UP arrow
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};
byte customDown[8] = {  // UDCHARS nice looking DOWN arrow
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
}

void loop() {
  static uint8_t state = SYNC;  // When the program is booted, the first state is SYNC
  unsigned long myTime = millis();
  static unsigned long previousTime = 0;
  const unsigned int freq = 2000;  // 2 seconds
  uint8_t buttons = lcd.readButtons();
  static bool isFirstMsgAdd = false;
  static bool holdingSelect = true;
  // * unsigned long pressStartTime = 0;  // To store the press start time
  static bool up = false;
  static bool down = false;
  static bool select = false;

  switch (state) {
    case SYNC:  // Initial synchronization state
      {
        lcd.setBacklight(3);
        if (myTime - previousTime >= freq) {
          previousTime = myTime;
          Serial.print(F("R"));
        }
        if (Serial.available() > 0) {
          String magicWord = Serial.readString();
          if (magicWord.indexOf("\n") != -1 || magicWord.indexOf("\r") != -1) {
            Serial.print(F("ERROR: '\\n' or '\\r' are invalid input"));
          }
          if (magicWord == "BEGIN") {
            state = STANDBY;
          }
        }
        break;
      }

    case STANDBY:  // Waiting for further input
      {
        static bool ready = false;
        if (!ready) {
          Serial.print(F("UDCHARS,FREERAM,SCROLL"));  // Prints implemented extensions
          Serial.print(F("\n"));
          lcd.setBacklight(7);
          ready = true;
        } else if (Serial.available() > 0) {
          state = READ;
        }
        break;
      }

    case READ:  // Main processing state. Reads the serial inputs and processes
      {
        if (Serial.available() > 0) {
          input = Serial.readStringUntil('\n');
          input.trim();
          int8_t firstDash = input.indexOf('-');
          int8_t secondDash = input.indexOf('-', firstDash + 1);
          int8_t thirdDash = input.indexOf('-', secondDash + 1);

          if (!isFirstMsgAdd) {  // Ensures the very first input to starts with "ADD"
            if (!input.startsWith("ADD")) {
              Serial.println(F("ERROR: The first serial command should be 'ADD'"));
              return;  // Wait for the next valid input
            } else {
              ADD(input, firstDash, secondDash, thirdDash);
              isFirstMsgAdd = true;  // Flag that ADD has been processed
            }
          } else {  // Processes serial commands from serial input
            if (input.startsWith("ADD")) {
              ADD(input, firstDash, secondDash, thirdDash);
            } else if (input.startsWith("PST")) {
              PST(input, firstDash, secondDash);
            } else if (input.startsWith("GRD")) {
              GRD(input, firstDash, secondDash);
            } else if (input.startsWith("SAL")) {
              SAL(input, firstDash, secondDash);
            } else if (input.startsWith("CJT")) {
              CJT(input, firstDash, secondDash);
            } else if (input.startsWith("DEL")) {
              DEL(input, firstDash);
            }
          }
          state = DISP;  // Display the outputs of serial commands
        }


        if (buttons & BUTTON_UP) {  // UP button clicked
          up = true;
          state = BUTTON;
        }
        if (buttons & BUTTON_DOWN) {  // DOWN button clicked
          down = true;
          state = BUTTON;
        }
        if (buttons & BUTTON_SELECT) {  // SELECT button clicked
          select = true;
          state = BUTTON;
        }

        break;
      }

    case BUTTON:  // Handling button inputs
      {
        if (up) {  // Scrolls UP the list of accounts
          if (currentIndex > 0) {
            currentIndex--;
          }
          up = false;
          state = DISP;
        }

        if (down) {  // Scrolls DOWN the list of accounts
          if (currentIndex < empIndex - 1) {
            currentIndex++;
          }
          down = false;
          state = DISP;
        }

        if (select) {  // While holding SELECT button, it displays the backlight to purple, my student ID and Free SRAM
          lcd.clear();
          if (!holdingSelect) {
            // Mark the button as being held down
            holdingSelect = true;
            // * pressStartTime = millis();  // Start timing the button press
          }
          // * if (millis() - pressStartTime > 1000) {  // If SELECT button is held more than 1 second
          while (holdingSelect && (buttons & BUTTON_SELECT)) {
            lcd.setBacklight(5);  // Purple
            lcd.setCursor(0, 0);
            lcd.print(F("F125742"));
            lcd.setCursor(0, 1);
            lcd.print(F("Free SRAM: "));
            lcd.print(freeMemory());
            buttons = lcd.readButtons();  // Re-read the button state to check if it's released

            if (!(buttons & BUTTON_SELECT)) {  // If SELECT button is released from holding, it displays back the previous lcd screen
              holdingSelect = false;
              select = false;
              state = DISP;
              break;
            }
          }
          // * }
        }
      }

    case DISP:  // Displaying employee data on LCD screen
      {
        // * static uint8_t scrollIndex = 0;
        // * unsigned long currentMillis = millis();
        // * static unsigned long scrollTime = 0;

        lcd.clear();
        if (currentIndex == 0) {  // The first account has no UP arrow
          lcd.setCursor(0, 0);
          lcd.print("");
        } else {  // UP arrow
          lcd.createChar(0, customUp);
          lcd.setCursor(0, 0);
          lcd.write((uint8_t)0);
        }
        if (currentIndex == empIndex - 1) {  // The last account has no DOWN arrow
          lcd.setCursor(0, 1);
          lcd.print("");
        } else {  // DOWN arrow
          lcd.createChar(1, customDown);
          lcd.setCursor(0, 1);
          lcd.write((uint8_t)1);
        }
        lcd.setCursor(1, 0);
        lcd.print(empData[currentIndex].grade);
        lcd.setCursor(3, 0);
        lcd.print(empData[currentIndex].pension ? " PEN" : "NPEN");
        if (empData[currentIndex].pension == true) {  // If the pension status is PEN, sets backlight to GREEN
          lcd.setBacklight(2);
        } else if (empData[currentIndex].pension == false) {  // If the pension status is NPEN, sets backlight to RED
          lcd.setBacklight(1);
        }
        lcd.setCursor(8, 0);
        lcd.print(empData[currentIndex].salary);
        lcd.setCursor(1, 1);
        lcd.print(empData[currentIndex].id);
        lcd.setCursor(9, 1);
        lcd.print(empData[currentIndex].title);

        // * String title = empData[currentIndex].title;
        // if (title.length() > 7) {
        //   if (currentMillis - scrollTime >= 1000) {
        //     scrollTime = currentMillis;
        //     scrollIndex += 2;  // Scroll left by 2 characters
        //     if (scrollIndex >= title.length() - 7) {
        //       scrollIndex = 0;  // Return to start when done
        //     }
        //     debug_valprintln(title.length());
        //     debug_valprintln(scrollIndex);
        //     debug_valprintln(title.substring(scrollIndex, scrollIndex + 7));
        //     lcd.setCursor(9, 1);
        //     lcd.print(title.substring(scrollIndex, scrollIndex + 7));
        //   }
        // } else {
        //   lcd.setCursor(9, 1);
        //   lcd.print(title);
        // * }
        
        state = READ;  // After displaying the outputs, go back to READ
        break;
      }
  }
}

// button select hold more than 1 second
// just documentation and scroll then done