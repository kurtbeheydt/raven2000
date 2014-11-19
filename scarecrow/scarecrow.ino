#include <Stepper.h>
#include <YunClient.h>
#include <YunServer.h>

#define pinMovementLeft A1
#define pinMovementRight A2
#define pinMovementUp A3
#define pinMovementDown A4
#define pinFire A5

#define pinVerticalStepA 4
#define pinVerticalDirectionA 5
#define pinVerticalStepB 6
#define pinVerticalDirectionB 7

#define pinVerticalStopSensor 8

#define pinHorizontalStepB 9
#define pinHorizontalDirectionB 10
#define pinHorizontalStepA 11
#define pinHorizontalDirectionA 12

#define pinHorizontalStopSensor 3
#define thresholdOpticalSensor 300 

#define pinMovementSpeedPotentio 0

#define pinValveOff 13
#define pinValveOn 2
#define durationValveOpen 500

#define STEPS 100


Stepper horizontalStepper(STEPS, pinHorizontalStepA, pinHorizontalDirectionA, pinHorizontalStepB, pinHorizontalDirectionB);
Stepper verticalStepper(STEPS, pinVerticalStepA, pinVerticalDirectionA, pinVerticalStepB, pinVerticalDirectionB);

int currentHorizontalPosition, newHorizontalPosition, currentVerticalPosition, newVerticalPosition, movementSpeed;

bool fire = false;
int maxHorizontalPosition = 600;
int maxVerticalPosition = 200;

byte action;
#define ACTION_STARTUP 0
#define ACTION_RESET 1
#define ACTION_READY 2

YunServer server;

void setup() {
  pinMode(pinMovementLeft, INPUT_PULLUP);
  pinMode(pinMovementRight, INPUT_PULLUP);
  pinMode(pinMovementUp, INPUT_PULLUP);
  pinMode(pinMovementDown, INPUT_PULLUP);
  pinMode(pinFire, INPUT_PULLUP);
  
  pinMode(pinVerticalStopSensor, INPUT_PULLUP);
  
  pinMode(pinValveOn, OUTPUT);
  pinMode(pinValveOff, OUTPUT);
  
  Bridge.begin();
  server.listenOnLocalhost();
  server.begin();

  Serial.begin(19200);
  delay(500);
  Serial.println("");
  
  action = ACTION_STARTUP;
}

void loop() {
  
  switch (action) {
    case ACTION_STARTUP:
      Serial.println("Startup");
      initHorizontal();
      initVertical();
      action = ACTION_RESET;
      break;
        
    case ACTION_RESET:
      if ((currentHorizontalPosition == 0) && (currentVerticalPosition == 0)) {
        action = ACTION_READY;
        Serial.println("Reset Complete");
      }
      delay(10);
      break;
        
    case ACTION_READY:
      readInput();
      readRemoteInput();
      break;
      
  }
  
  if (fire == true) {
      shoot();
      fire = false;
  }
  
  moveHorizontal();
  moveVertical();
  delay(10);
}

void readInput() {
  if (!digitalRead(pinMovementLeft)) {
    if (digitalRead(pinMovementRight)) {
      newHorizontalPosition--;
    }
  } else if (!digitalRead(pinMovementRight)) {
    newHorizontalPosition++;
  } else if (!digitalRead(pinMovementDown)) {
    if (digitalRead(pinMovementUp)) {
      newVerticalPosition--;
    }
  } else if (!digitalRead(pinMovementUp)) {
    newVerticalPosition++;
  } else if (!digitalRead(pinFire)) {
    if (!fire) {
      fire = true;
    }
  }
}

void readRemoteInput() {
  YunClient client = server.accept();
  if (client) {
    client.setTimeout(5);
    
    String command = client.readStringUntil('/');
    command.trim();
    
    if (command == "left") { // http://michelle.local/arduino/left
      newHorizontalPosition--;
    } else if (command == "right") {
      newHorizontalPosition++;
    } else if (command == "bigleft") {
      newHorizontalPosition = newHorizontalPosition - 10;
    } else if (command == "bigright") {
      newHorizontalPosition = newHorizontalPosition + 10;
    } else if (command == "up") { 
      newVerticalPosition++;
    } else if (command == "down") {
      newVerticalPosition--;
    } else if (command == "bigup") {
      newVerticalPosition = newVerticalPosition + 10;
    } else if (command == "bigdown") {
      newVerticalPosition = newVerticalPosition - 10;
    } else if (command == "fire") {
      fire = true;
    } 

    Serial.print("Command received: ");
    Serial.print(command);
    Serial.print("    New Horizontal position: ");
    Serial.print(newHorizontalPosition);
    Serial.print(" - New Vertical position: ");
    Serial.println(newVerticalPosition);

    client.println("Status: 200");
    client.println("Content-type: application/json");
    client.println();
    client.print("{\"status\":\"ok\"}");
    client.stop();
  }
  delay(10);
}

void initHorizontal() {
  Serial.println("InitHorizontal");
  currentHorizontalPosition = 600;
  newHorizontalPosition = 0;
}

void initVertical() {
  Serial.println("initVertical");
  currentVerticalPosition = 200;
  newVerticalPosition = 0;
}

void moveHorizontal() {
  readMovementSpeed();
  
  if (action == ACTION_RESET) {
    if (newHorizontalPosition >= maxHorizontalPosition) {
      newHorizontalPosition = maxHorizontalPosition;
    } else if (newHorizontalPosition <= 0) {
      newHorizontalPosition = 0;
    }
  }
  
  if (newHorizontalPosition > currentHorizontalPosition) {
    if (currentHorizontalPosition <= maxHorizontalPosition) {
      currentHorizontalPosition++;
      horizontalStepper.step(1);
    }
  } else if (newHorizontalPosition < currentHorizontalPosition) {
    if (action == ACTION_RESET) {
      if (readHorizontalSensor(pinHorizontalStopSensor)) {
        currentHorizontalPosition--;
        horizontalStepper.step(-1);
      } else {
        currentHorizontalPosition = 0;
      }
    } else {
      currentHorizontalPosition--;
      horizontalStepper.step(-1);
    }
  }
}

void moveVertical() {
  readMovementSpeed();
  
  if (action == ACTION_RESET) {
    if (newVerticalPosition >= maxVerticalPosition) {
      newVerticalPosition = maxVerticalPosition;
    } else if (newVerticalPosition <= 0) {
      newVerticalPosition = 0;
    }
  }
  
  if (newVerticalPosition > currentVerticalPosition) {
    if (currentVerticalPosition <= maxVerticalPosition) {
      currentVerticalPosition++;
      verticalStepper.step(1);
    }
  } else if (newVerticalPosition < currentVerticalPosition) {
    if (readVerticalSensor()) {
      currentVerticalPosition--;
      verticalStepper.step(-1);
    } else {
      currentVerticalPosition = 0;
    }
  }
}

void shoot() {
  digitalWrite(pinValveOn, 1);
  digitalWrite(pinValveOff, 0);
  delay(durationValveOpen);
  digitalWrite(pinValveOn, 0);
  digitalWrite(pinValveOff, 1);
  delay(100);
  digitalWrite(pinValveOn, 0);
  digitalWrite(pinValveOff, 0);
}

bool readVerticalSensor() {
  return digitalRead(pinVerticalStopSensor);
}

bool readHorizontalSensor(uint8_t pin) {
  pinMode(pin, OUTPUT);  
  digitalWrite(pin, HIGH);
  uint32_t t0 = micros();
  uint32_t dif;
  pinMode(pin, INPUT);
  
  while(digitalRead(pin)) {
    dif = micros() - t0;
    if (dif > thresholdOpticalSensor) break;
  }
  
  Serial.print(pin);
  Serial.print(": ");
  Serial.println(dif);
  return dif > thresholdOpticalSensor;
}

void readMovementSpeed() {
  uint32_t newSpeed = analogRead(pinMovementSpeedPotentio);
  newSpeed = 25 + map(newSpeed, 0, 1021, 0, 100);
  if (newSpeed != movementSpeed) {
    movementSpeed = newSpeed;
    horizontalStepper.setSpeed(movementSpeed);
    Serial.print("New movementSpeed: ");
    Serial.println(movementSpeed);
  }
}
