/** 
 *  @author : fajarlabs
 *  @youtube : https://tinyurl.com/lpel43o0
 *  @facebook : https://www.facebook.com/SUPERBIN/
 *  
 */



#include "protothreads.h"
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// init servo
Servo servo;

// init LCD_12C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Led Display
const int LED = 13;

// Buzzer
const int pinBuzzer = 12;

// Relay Module
const int pinRelay = 11;

// servoSG90
const int pinServo = 10;
int angle = 0;

// ultrasonic
const int TRIG_PIN = 8;
const int ECHO_PIN = 9;

// ir proximity
const int pinIRProx = 7;

// store data
String inputString;
String dataStoreAlarm;
String dataStoreLcd;

// activate alarm
int alarmActive = false;

// label
const String LB_FACE = "SCAN WAJAH ANDA";
const String LB_TIMEOUT = "WAKTU HABIS";
const String LB_ENTRY = "SILAHKAN MASUK";
const String LB_HANDWASH = "CUCI TANGAN ANDA";
const String LB_DO_NOT_ENTRY = "DILARANG MASUK";
const String LB_TITLE = "PROTOKOL COVID19";
const String LB_STARTUP = "DEVICE READY";

/**
 * Step 1 read serial 1 or 0 from Fask Mask Recognizer
 * Step 2 read IR proximity to activating water pump
 * Step 3 open gate
 * Step 4 read ultrasonic to closing door
 */
// step
int step = 0;

const int TIMEOUT_STEP = 2; // 2 minutes

pt ptAlarm;
pt ptLcd;
pt ptUltraSonic;
pt ptTimeout;
pt ptInfraRed;

/**
 * Fungsi ini akan selalu mengecek apakah step berhenti
 * atau tidak ada kegiatan maka dalam 2 menit
 * akan di reset ulang sistemnya
 */
void timeoutThread(struct pt* pt) {
  PT_BEGIN(pt);
  if(step > 0) {
    PT_SLEEP(pt, (TIMEOUT_STEP*60000) );
    
    clearLCDLine(1);
    lcd.print(LB_TIMEOUT);
    PT_SLEEP(pt, 2000);
    
    clearLCDLine(1);
    lcd.print(LB_FACE);
    
    step = 0; // reset step if timeout in 60 seconds
    
    closeGate();
  }
  PT_END(pt);
}

void infraRedThread(struct pt* pt) {
  PT_BEGIN(pt);  
  
  if((step > 1) && (step < 4)) {
    // Jarak tangan ke cuci < 25 cm
    int irProxData = digitalRead(pinIRProx);
    if(irProxData < 1) {
      // pump On
      digitalWrite(pinRelay, HIGH);
      PT_SLEEP(pt, 2000);
      // show LCD 
      clearLCDLine(1);
      lcd.print(LB_ENTRY);
      digitalWrite(pinRelay, LOW);
      if(step == 2) {
        // open gate
        openGate();
        step++;
        // Serial.println("Step 3 :"+String(step));
      }
    }
  }
    
  PT_END(pt);
}

void ultraSonicThread(struct pt* pt) {
  PT_BEGIN(pt);  
  long duration, distanceCm, distanceIn;
  if(step == 3){
    // Serial.println("Step 4 :"+String(step));
    digitalWrite(TRIG_PIN, LOW);
    PT_SLEEP(pt, 2);
    digitalWrite(TRIG_PIN, HIGH);
    PT_SLEEP(pt, 2);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN,HIGH);
    
    // convert the time into a distance
    distanceCm = duration / 29.1 / 2 ;
    distanceIn = duration / 74 / 2;

    // Jarak tangan ke cuci < 10 cm
    if(distanceCm < 10) {
      // reset LCD
      clearLCDLine(1);
      lcd.print(LB_FACE);

      step = 0; //reset step  
      // Serial.println("Step END : "+String(step));
      
      // activating buzzer
      alarmActive = true;
      
      // close gate
      closeGate();
    }
  }
    
  PT_END(pt);
}

void alarmThread(struct pt* pt) {
  PT_BEGIN(pt);
  if((dataStoreAlarm.equals("1") && (step < 2)) or alarmActive ) {
      digitalWrite(pinBuzzer, LOW);   // turn the LED on (HIGH is the voltage level)
      PT_SLEEP(pt, 1000);
      digitalWrite(pinBuzzer, HIGH);    // turn the LED off by making the voltage LOW
  }
  alarmActive = false;
  dataStoreAlarm = ""; // empty data alarm
  PT_END(pt);
}

void lcdThread(struct pt* pt) {
  PT_BEGIN(pt);
  if(dataStoreLcd.equals("1") && (step == 1)) {
    // step 2
    step++;
    // Serial.println("Step 2 :"+String(step));
    
    clearLCDLine(1);
    lcd.print(LB_HANDWASH);
    digitalWrite(LED, LOW); // sets the LED on
  }

  if (dataStoreLcd.equals("0") && (step < 1)){
    clearLCDLine(1);
    lcd.print(LB_DO_NOT_ENTRY);
    digitalWrite(LED, HIGH);  // sets the LED off
    PT_SLEEP(pt, 300);
    clearLCDLine(1);
    lcd.print(LB_FACE);
  }

  dataStoreLcd=""; // empty data LCD
  PT_END(pt);
}

void openGate() {
  // scan from 0 to 99 degrees
  for(angle = 0; angle < 99; angle++)  
  {                                  
    servo.write(angle);               
    delay(20);                   
  } 
}

void closeGate() {
  // now scan back from 99 to 0 degrees
  for(angle = 99; angle > 0; angle--)    
  {                                
    servo.write(angle);           
    delay(20);       
  } 
}

void setup() { 
  PT_INIT(&ptAlarm);
  PT_INIT(&ptLcd);
  PT_INIT(&ptUltraSonic);
  PT_INIT(&ptTimeout);
  PT_INIT(&ptInfraRed);

  // servo
  servo.attach(10);
  servo.write(pinServo);

  // default gate is closed
  closeGate();

  // LCD
  lcd.begin();
  clearLCDLine(0);
  lcd.print(LB_TITLE);
  clearLCDLine(1);
  lcd.print(LB_FACE);
  Serial.begin(9600);

  // Led
  pinMode(LED, OUTPUT);
  digitalWrite (LED, HIGH);
  Serial.println(LB_STARTUP);

  // buzzer
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite (pinBuzzer, HIGH);

  // relay module 
  pinMode(pinRelay, OUTPUT);
  digitalWrite (pinRelay, LOW);

  // ultrasonic
  pinMode(TRIG_PIN,OUTPUT);
  pinMode(ECHO_PIN,INPUT);

  // ir proximity
  pinMode(pinIRProx, INPUT);
}

void loop() {
  alarmThread(&ptAlarm);
  lcdThread(&ptLcd);
  ultraSonicThread(&ptUltraSonic);
  timeoutThread(&ptTimeout);
  infraRedThread(&ptInfraRed);
}

void clearLCDLine(int line){
 for(int n = 0; n < 20; n++) { // 20 indicates symbols in line. For 2x16 LCD write - 16
   lcd.setCursor(n,line);
   lcd.print(" ");
 }
 lcd.setCursor(0,line); // set cursor in the beginning of deleted line
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      // do not receive data if step greather than 0
      if(step < 1) {
        inputString.trim();
        if(inputString.equals("1")) {
          dataStoreAlarm = inputString;
          dataStoreLcd = inputString;
          step++; // step 1
          // Serial.println("Step 1 :"+String(step));
        }
        inputString = "";
        // reset input string
        
      }
    }
  }
}
