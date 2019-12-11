#include <HCSR04.h>  //https://github.com/gamegine/HCSR04-ultrasonic-sensor-lib
#include <RTClib.h>
//https://circuitdigest.com/microcontroller-projects/arduino-ssd1306-oled-display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

//  DEFAULT VARIABLES
const unsigned long minutes = 60000;

/*******************************************
          VARIABLES TO DEFINE
 *******************************************/
const DateTime currentTime(2019, 12, 10, 20, 48, 20);

//  Tank height
const int TANK_SIZE = 88; // cm

//  water periods
const long waterPeriod = 4 * minutes;  // minutes
const long waitWaterPeriod = 10 * minutes;  // minutes

//  water schedules HH,MM
const int waterSchedules[2] = {7, 18};


/*******************************************
          PINS USED
 *******************************************/
//  Moister sensors
#define MOISTER_1 A0
#define MOISTER_2 A1
//#define NO_DEFINED A2
//#define NO_DEFINED A3
//#define SDA A4
//#define SDC A5


//  Tank Sensors
//#define TANK_ULTRASONIC_ECHO 0
//#define TANK_ULTRASONIC_TRIG 1

//#define NO_DEFINED_0 2
#define TANK_ULTRASONIC_ECHO 3
#define TANK_ULTRASONIC_TRIG 2

//  Pump
#define PUMP_ON                5
#define PUMP_MENU_BUTTON       6

//  Oled Screen
#define OLED_MENU_BUTTON       7
#define OLED_SCL               8
#define OLED_SDA               9
#define OLED_DC               10
#define OLED_CS               11
#define OLED_RESET             4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


/*******************************************
        INITIALIZE COMPONENTS
 *******************************************/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_SCL, OLED_SDA, OLED_DC, OLED_RESET, OLED_CS);

//  RTC
RTC_DS3231 rtc;

//  ULTRASONIC SENSOR
HCSR04 hc(TANK_ULTRASONIC_TRIG, TANK_ULTRASONIC_ECHO);
//UltraSonicDistanceSensor hc(TANK_ULTRASONIC_TRIG, TANK_ULTRASONIC_ECHO);


/*******************************************
        MENU OLED
 *******************************************/
//  1 - OLED ON AND READ SENSORS
//  2 - DATE TIME
//  3 - SHOW LATEST WATER TIMES
//  default - OLED OFF
int screenMenuSelector = 1;
const int screenNumberOptions = 3;

/*******************************************
        WATERING TRAIL
 *******************************************/
const int numberWateringRegisters = 8;
int wateringRegisters[numberWateringRegisters][6];
int wateringRegistersPointer = 0;

/*******************************************
        MENU PUMP
 *******************************************/
//  1 - PUMP ON
//  default - PUMP OFF
int pumpMenuSelector = 0;
const int pumpMenuOptions = 1;

/*******************************************
        OTHERS VARIABLES
 *******************************************/
unsigned long referenceTime;

//  Moister
int moisterSensor1_value;
int moisterSensor2_value;
const int moisterMin = 1500;
const int moisterMax = 3500;
const int moisterRank = moisterMax - moisterMin;
const int moistureSoilMin = 45; // %
const int moistureSoilMax = 55; // %

void setup()
{
  Serial.println("setuop ini");
  //   Initialize devices
  Serial.begin(19200);
  initializeDisplay();
  //  RTC
  if (!rtc.begin()) {
    Serial.println("Module RTC no encontrado");
    while (1);
  }
  rtc.adjust(currentTime);

  //  Initialize I/O pins
  //  pinMode(OLED_RELAY_SWITCH, OUTPUT);
  pinMode(MOISTER_1, INPUT);
  pinMode(MOISTER_2, INPUT);
  pinMode(OLED_MENU_BUTTON, INPUT);
  pinMode(PUMP_MENU_BUTTON, INPUT);
  pinMode(PUMP_ON, OUTPUT);

  pinMode(TANK_ULTRASONIC_TRIG, OUTPUT);
  pinMode(TANK_ULTRASONIC_ECHO, INPUT);

  //  Initialize devices status
  digitalWrite(PUMP_ON, LOW);
  Serial.println("se tetupppppppp fin");

  
}

void loop() {
  //Serial.println(F("looooooooooooooooooooooop"));
  // Pump Menu Control
  if (digitalRead(PUMP_MENU_BUTTON) == HIGH) {
    Serial.println("PUMP_MENU_BUTTON HIGH");
    while (digitalRead(PUMP_MENU_BUTTON) == HIGH);//  Only 1 click
    pumpMenuSelector++;
    
  }
  pumpMenuController();

  // Screen Menu Control
  if (digitalRead(OLED_MENU_BUTTON) == HIGH) {
    Serial.println("OLED_MENU_BUTTON HIGH");
    while (digitalRead(OLED_MENU_BUTTON) == HIGH);//  Only 1 click
    screenMenuSelector++;
  }
  screenController();

  //  Automatic irrigation
//  automaticWater();

}

void readSensors() {
  moisterSensor1_value = getPercentageMoisterSensor(analogRead(MOISTER_1));
  moisterSensor2_value = getPercentageMoisterSensor(analogRead(MOISTER_2));
}

/**
    Oled controller
*/
void screenController() {
  if (screenMenuSelector > screenNumberOptions) {
    screenMenuSelector = 0;
  }
  switch (screenMenuSelector) {
    case 0: //  Screen OFF
      display.clearDisplay();
      display.display();
      break;
    case 1: //  Screen ON
      printSersorData();
      break;
    case 2:
      printLatestWatering();
      break;
    case 3:
      printClock();
      break;
  }
}



int getPercentageMoisterSensor(int value) {
  int moister = 100 - ((value - moisterMin) * 100) / moisterRank;
  if (moister > 100) {
    return 100;
  } else if (moister < 0) {
    return 0;
  }
  return moister;
}

void initializeDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    while (1); // Don't proceed, loop forever
  }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
}

int distance;
long duration;
int getDistance(){
  digitalWrite(TANK_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TANK_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TANK_ULTRASONIC_TRIG, LOW);

  duration = pulseIn(TANK_ULTRASONIC_ECHO, HIGH);
  return duration*0.034/2;
}



//int tempYearHistoric[12][];
int tempMonthHistoric[] = {};
int currentHour = 0;
int hourPointer;
//void saveTemp() {
//  DateTime now = rtc.now();
//  // update year
//  
//
//    tempMonthHistoric[(24 * monthDays[currentMonth])];
//    hourPointer = 0;
//  }
//  tempMonthHistoric[hourPointer] = round(rtc.getTemperature());
//}


//void printHistoricTemp() {
//  display.clearDisplay();
//  display.drawLine(20, 0, 20, 40, SSD1306_WHITE);
//  display.drawLine(20, 40, 120, 40, SSD1306_WHITE);
//  display.setCursor(5, 40);
//  int prevPosX = 20;
//  int prevPosY = 40;
//  for (int i = 0; i < sizeof(tempMonthHistoric); i++) {
//    display.drawLine(prevPosX, prevPosY, prevPosX + (i * 3), tempMonthHistoric[i], SSD1306_WHITE);
//  }
//  display.display();
//}


/**
    Save the DateTime in numberwateringRegisters  2Darray
*/
void saveDateTime() {
  wateringRegistersPointer++;
  if (wateringRegistersPointer == numberWateringRegisters) {
    wateringRegistersPointer = 0;
  }

  DateTime now = rtc.now();
  wateringRegisters[wateringRegistersPointer][0] = now.day();
  wateringRegisters[wateringRegistersPointer][1] = now.month();
  wateringRegisters[wateringRegistersPointer][2] = now.year();
  wateringRegisters[wateringRegistersPointer][3] = now.hour();
  wateringRegisters[wateringRegistersPointer][4] = now.minute();
  wateringRegisters[wateringRegistersPointer][5] = now.second();
}

/**
    Print clock
*/
void printClock() {
  // display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setFont(&FreeSans9pt7b);
  //display.setFont();

  // Start at top-left corner
  display.clearDisplay();
  DateTime now = rtc.now();

  display.setCursor(18, 20);
  display.print(formatTime(now.day()));
  display.print("/");
  display.print(formatTime(now.month()));
  display.print("/");
  display.println(now.year());

  display.setCursor(30, 47);
  display.print(formatTime(now.hour()));
  display.print(":");
  display.print(formatTime(now.minute()));
  display.print(":");
  display.println(formatTime(now.second()));

  display.display();
  delay(500);
}

uint8_t formatTime(uint8_t value) {
  return value;
//  String str = "" + value;
//  if (str.length() > 1) {
//    return str;
//  }
//
//  return '0' + str;
}

/**
    Print latest watering registers
*/
void printLatestWatering() {
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setFont();
  display.setCursor(0, 0);    // Start at top-left corner

  //  Print lastest irrigation days sorted by day
  int dateShorted = wateringRegistersPointer;
  for (int i = 0; i < numberWateringRegisters; ++i) {
    //  DATE
    display.print(wateringRegisters[dateShorted][0]);
    display.print(wateringRegisters[dateShorted][1]);
    display.print("/");
    display.print(wateringRegisters[dateShorted][2]);
    display.print("  ");
    display.print(wateringRegisters[dateShorted][3]);
    display.print(":");
    display.println(wateringRegisters[dateShorted][4]);

    dateShorted--;
    if (dateShorted < 0) {
      dateShorted = numberWateringRegisters - 1;
    }
  }
  display.display();
  delay(500);
}


void printSersorData() {
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setFont();
  display.setCursor(0, 0);    // Start at top-left corner
  display.clearDisplay();
  //  Moister sensor 1
  display.print(F("Moi. 1: "));
  display.print(moisterSensor1_value);
  display.println(F("%"));

  //  Moister sensor 2
  display.print(F("Moi. 2: "));
  display.print(moisterSensor2_value);
  display.println(F("%"));
  Serial.print(F("Tank "));
  Serial.println(hc.dist());
  Serial.print(F("Tank22222222222222 "));
  Serial.println(getDistance());
  
  //  Tank sensor 2
  display.print(F("Tank: "));
  display.println(hc.dist());
  display.println(F("%"));

  display.drawLine(83, 0, 83, 64, SSD1306_WHITE);

  //  Temp
  display.drawCircle(108, 20, 19, SSD1306_WHITE);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(92, 28);
  display.print(round(rtc.getTemperature()));
  display.setFont();
  display.setCursor(113, 15);
  display.write((int16_t) 247);
  display.print(F("C"));

  //  Print data
  display.display();
  delay(500);
}

/**
    Condition to water
    - Water in the tank
    - Morning/Evening water time
    - soil dry
*/
void automaticWater() {
  if (89 < TANK_SIZE) { // Check if the tank has water. tank with water  hc.dist()

    DateTime now = rtc.now();
    for (int i = 0; i < sizeof(waterSchedules); i++) { //  Times scheduled
      if (waterSchedules[i] == now.hour()) {          //  if is time to watering "waterSchedules"

        readSensors();
        if (moisterSensor1_value < moistureSoilMax || moisterSensor2_value < moistureSoilMax) { // if moister is < 55
          if ((PUMP_ON == LOW)                                          //  pump not active
              && (moisterSensor1_value < moistureSoilMin || moisterSensor2_value < moistureSoilMin) //  if moister is < 45
              && ((millis() - referenceTime) > waitWaterPeriod)) {        //  latest water > 'waitWaterPeriod'

            referenceTime = millis(); //  save current time
            digitalWrite(PUMP_ON, HIGH); // start water
            saveDateTime();
          }
        }
      }
    }
  }

  //  Stop watering 'waterPeriod' watering OR not water in the tank
  if ((PUMP_ON == HIGH) && (((millis() - referenceTime) > waterPeriod) || hc.dist() > TANK_SIZE)) {
    digitalWrite(PUMP_ON, LOW);
    referenceTime = millis();
  }
}

/**
    Manual pump control
*/
void pumpMenuController() {
  if (pumpMenuSelector > pumpMenuOptions) {
    pumpMenuSelector = 0;
  }

  switch (pumpMenuSelector) {
    case 1: //  PUMP ON
      digitalWrite(PUMP_ON, HIGH);
      saveDateTime();
      break;
    default: //  PUMP OFF
      digitalWrite(PUMP_ON, LOW);
      break;
  }

}
/*TESTS
  -- TANK SENSOR
  - LESS THAN 2 CM
  - 5O CM
  - MORE THAN 90 CM

  --  MOISTURE
  - DRY
  - WET

  --  CLOCK
  - TEMPERATURE

  --  PUMP
  - TURN ON
  - TURN OFF
  - VALIDATE TIMES
    . MOISTER WET
    . MOISTER DRY
  - OFF TIMES
    . MOISTER WET
    . MOISTER DRY

  --  OLED
  - DISPLAY CLOCK
  - DISPLAY LAST WATERING
  - DISPLAY SENSORS
*/

void printSersorData2() {
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setFont();
  display.clearDisplay();
  printTank(tankPercentage2());
  printMoister();
  printRtcTemp();
  display.drawLine(83, 0, 83, 64, SSD1306_WHITE);

  display.display();
  delay(100);
}

int testSize = 0;
int tankPercentage2() {
  testSize++;
  if (testSize > 100) {
    testSize = 0;
  }
  return (hc.dist() * 100) / TANK_SIZE;
  return (testSize * 100) / TANK_SIZE;
}

/**
  Print a tank with the percentage fully
*/
const int tankHeight = 50;
const int tankWidth = 25;
const int tankXPosition = 92;
void printTank(int tankPercentage) {
  String percentString = String(tankPercentage);
  if (percentString.length() == 2) {
    display.setCursor(tankXPosition + 4, 3);
  } else if (percentString.length() == 3) {
    display.setCursor(tankXPosition + 1, 3);
  } else if (percentString.length() == 1) {
    display.setCursor(tankXPosition + 7, 3);
  }
  display.print(tankPercentage);
  display.println("%");
  display.drawRect(tankXPosition, display.height() - tankHeight, tankWidth, tankHeight, SSD1306_WHITE);
  int tankGraphic = display.height() - (tankPercentage / 2 );
  display.fillRect(tankXPosition, tankGraphic, tankWidth, display.height(), SSD1306_WHITE);
}

void printMoister() {
  //  Moister sensor 1
  display.setCursor(0, 0);
  display.print(F("Moi. 1: "));
  display.print(100);
  display.println(F("%"));

  //  Moister sensor 2
  display.print(F("Moi. 2: "));
  display.print(moisterSensor2_value);
  display.println(F("%"));
}

const int tempXPos = 35;
const int tempYPos = 40;
void printRtcTemp() {
  //  Temp
  display.drawCircle(tempXPos, tempYPos, 19, SSD1306_WHITE);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(tempXPos - 16, tempYPos + 8);
  display.print(rtc.getTemperature()); 
  display.setFont();
  display.setCursor(tempXPos + 5, tempYPos - 5);
  display.write((int16_t) 247);
  display.print(F("C"));
}
