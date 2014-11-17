#include <Stepper.h>


#define pinVerticalStop 2

#define pinHorizontalStepA 8
#define pinHorizontalDirectionA 9
#define pinHorizontalStepB 10
#define pinHorizontalDirectionB 11

#define pinMovementSpeedPotentio 0


#define STEPS 100


Stepper horizontalStepper(STEPS, pinHorizontalStepA, pinHorizontalDirectionA, pinHorizontalStepB, pinHorizontalDirectionB);

int currentHorizontalPosition, newHorizontalPosition, movementSpeed;
int maxHorizontalPosition = 600;


#define pinMovementLeft 4
#define pinMovementRight 3

#define pinOpticalSensor 5
#define thresholdOpticalSensor 150  

byte action;
#define ACTION_STARTUP 0
#define ACTION_READY 1



void setup() {
  pinMode(pinVerticalStop, INPUT);
  digitalWrite(pinVerticalStop, HIGH);
  pinMode(pinMovementLeft, INPUT);
  digitalWrite(pinMovementLeft, HIGH);
  pinMode(pinMovementRight, INPUT);
  digitalWrite(pinMovementRight, HIGH);
  
  Serial.begin(9600);
  delay(500);
  Serial.println("");
  
  action = ACTION_STARTUP;
  initHorizontal();
 
}

void loop() {
  readInput();
  //readOpticalSensor(pinOpticalSensor);
  
  moveHorizontal();
}

void readInput() {
  if (!digitalRead(pinMovementLeft)) {
    if (digitalRead(pinMovementRight)) {
      newHorizontalPosition--;
    }
  } else if (!digitalRead(pinMovementRight)) {
    newHorizontalPosition++;
  }
  Serial.print("current: ");
  Serial.print(currentHorizontalPosition);
  Serial.print("New: ");
  Serial.println(newHorizontalPosition);
}

void initHorizontal() {
  Serial.println("InitHorizontal");
  currentHorizontalPosition = 200;
  newHorizontalPosition = 0;
//  readMovementSpeed();
}

void moveHorizontal() {
  readMovementSpeed();
  
  if (action == ACTION_STARTUP) {
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
    if (action == ACTION_STARTUP) {
      if (readOpticalSensor(pinOpticalSensor)) {
        currentHorizontalPosition--;
        horizontalStepper.step(-1);
      } else {
        action = ACTION_READY;
        currentHorizontalPosition = 0;
      }
    } else {
      currentHorizontalPosition--;
      horizontalStepper.step(-1);
    }
  }
//  delay(10);
}

bool readVerticalSensor() {
  return digitalRead(pinVerticalStop);
}

bool readOpticalSensor(uint8_t pin) {
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
