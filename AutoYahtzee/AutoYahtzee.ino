#include "ServoAngles.cpp"
#include "StateManager.h"
#include "States.h"
#include <AFMotor.h>
#include <Servo.h>

Servo servo;

//FORWARD - close
//BACKWARD - open
AF_DCMotor horMotor(1);

//FORWARD - up
//BACKWARD - down
AF_DCMotor verMotor(2);

const int TOWER_DOWN_PIN = A0;
const int TOWER_UP_PIN = A1;
const int BOX_OPEN_PIN = A2;
const int BOX_CLOSE_PIN = A3;

bool towerDown = false;
bool towerUp = false;
bool boxClose = false;
bool boxOpen = false;

bool diagnosticsResult[] = { false, false, false, false };

bool diagnosticsCopmlete = false;
bool startUpCheckComplete = false;
bool communicationCheckComplete = true;
bool horSetupComplete = false;
bool verSetupComplete = false;
bool setupComplete = false;

const int servoSpeed = 3;

StateManager stateManager;

void setup() {
  Serial.begin(9600);
  pinMode(TOWER_DOWN_PIN, INPUT_PULLUP);
  pinMode(TOWER_UP_PIN, INPUT_PULLUP);
  pinMode(BOX_CLOSE_PIN, INPUT_PULLUP);
  pinMode(BOX_OPEN_PIN, INPUT_PULLUP);

  horMotor.setSpeed(255);
  horMotor.run(RELEASE);

  verMotor.setSpeed(255);
  verMotor.run(RELEASE);
}

void loop() {
  if(!communicationCheckComplete) {
    doCommunicationCheck();
  } else if(!diagnosticsCopmlete) {
    doDiagnostics();
  } else if(!setupComplete) {
    doSteup();
  } else {
    doWork();
  }
}

void updateSwitchStates() {
  towerDown = digitalRead(TOWER_DOWN_PIN) == LOW;
  towerUp = digitalRead(TOWER_UP_PIN) == LOW;
  boxClose = digitalRead(BOX_CLOSE_PIN) == LOW;
  boxOpen = digitalRead(BOX_OPEN_PIN) == LOW;
}

void doDiagnostics() {

  bool allButtonsPressed = false;
  
  Serial.println("Starting diagnostics..");
  
  while(!diagnosticsCopmlete) {
  
    updateSwitchStates();
   
    if(towerDown && !towerUp && !boxClose && !boxOpen) {
      diagnosticsResult[0] = true;
      verMotor.run(FORWARD);
    } else if(towerUp && !towerDown && !boxClose && !boxOpen) {
      diagnosticsResult[1] = true;
      verMotor.run(BACKWARD);
    } else if(boxClose && !boxOpen && !towerDown && !towerUp) {
      diagnosticsResult[2] = true;
      horMotor.run(BACKWARD);
    } else if(boxOpen && !boxClose && !towerDown && !towerUp) {
      diagnosticsResult[3] = true;
      horMotor.run(FORWARD);
    } else if(boxClose && towerDown && !boxOpen && !towerUp) {
      verMotor.run(FORWARD);
      horMotor.run(BACKWARD);
    } else if(boxClose && towerUp && !boxOpen && !towerDown) {
      verMotor.run(BACKWARD);
      horMotor.run(BACKWARD);
    } else if(boxOpen && towerDown && !boxClose && !towerUp) {
      verMotor.run(FORWARD);
      horMotor.run(FORWARD);
    } else if(boxOpen && towerUp && !boxClose && !towerDown) {
      verMotor.run(BACKWARD);
      horMotor.run(FORWARD);
    } else if(allButtonsPressed && towerUp && towerDown && !boxOpen && !boxClose) {
      Serial.println("Diagnostics complete.");
      diagnosticsCopmlete = true;
      
      verMotor.run(RELEASE);
      horMotor.run(RELEASE);
    } else {
      verMotor.run(RELEASE);
      horMotor.run(RELEASE);
    }
  
    allButtonsPressed = diagnosticsResult[0] && diagnosticsResult[1] && diagnosticsResult[2] && diagnosticsResult[3];
    
    delay(20);
  }
  
  verMotor.run(RELEASE);
  horMotor.run(RELEASE);
}


void receiveMessage(String message) {
  Serial.println("Waiting for a message: " + message);
  bool messageReceived = false;
  while(!messageReceived) {
   if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      if(data == message) {
        messageReceived = true;
        Serial.println("Message Received: " + message);
      }
    } 
  }
}

void sendMessage(String message) {
  Serial.println(message);
}

void doCommunicationCheck() {
  while(!communicationCheckComplete) {
    receiveMessage("PI:COMMUNICATION HANDSHAKE");
    sendMessage("ARDUINO: COMMUNICATION HANDSHAKE");
    Serial.println("Communication check complete");
    delay(100);
    communicationCheckComplete = true;
  }
}

void doSteup() {

  closeDoor();
  moveUp();
  
  servo.attach(9);
  servo.write(ServoAngles::MOVE);
  
  Serial.println("Setup complete. Ready for operatrion.");
  delay(2000);
  setupComplete = true;
}

void doWork() {
  String state = stateManager.GetCurrentState();
  updateSwitchStates();
  
  if(state == States::UNLOAD) {
    
    if(doUnloadCheck()) {
      doUnload();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  } else if(state == States::PRE_LIFT) {
    if(doPreLiftCheck()) {
      doPreLift();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  } else if(state == States::LIFT) {
    if(doLiftCheck()) {
      doLift();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  } else if(state == States::THROW) {
    if(doThrowCheck()) {
      sendMessage("ARDUINO:READY TO THROW");
      receiveMessage("PI:THROW");
      doThrow();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
      delay(1000);
    }
  } else if(state == States::PRE_LOWER) {
    if(doPreLowerCheck()) {
      doPreLower();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  } else if(state == States::LOWER) {
    if(doLowerCheck()) {
      receiveMessage("PI:RELOAD");
      doLower();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  } else if(state == States::PRE_UNLOAD) {
    if(doPreUnloadCheck()) {
      doPreUnload();
      stateManager.StepForward();
      Serial.println("Current State: " + stateManager.GetCurrentState());
    }
  }
}

void doUnload() {
  openDoor();
  delay(200);
  closeDoor();
}

void doPreLift() {
  rotateServo(ServoAngles::MOVE);
}

void doLift() {
  moveUp();
}

void doThrow() {
  rotateServo(ServoAngles::THROW);
}

void doPreLower() {
  rotateServo(ServoAngles::MOVE);
}

void doLower() {
  moveDown();
}

void doPreUnload() {
  rotateServo(ServoAngles::LOAD);
}

void closeDoor() {
  while(!boxClose) {
    horMotor.run(FORWARD);
    updateSwitchStates();
    delay(20);
  }

  Serial.println("Door Closed");
  horMotor.run(RELEASE);
}

void openDoor() {
  while(!boxOpen) {
    horMotor.run(BACKWARD);
    updateSwitchStates();
    delay(20);
  }

  Serial.println("Door Opened");
  horMotor.run(RELEASE);
}

void moveUp() {
  while(!towerUp) {
    verMotor.run(FORWARD);
    updateSwitchStates();
    delay(20);
  }

  Serial.println("Lifted to the top");
  verMotor.run(RELEASE);
}

void moveDown() {
  while(!towerDown) {
    verMotor.run(BACKWARD);
    updateSwitchStates();
    delay(20);
  }

  Serial.println("Lowered to the bottom");
  verMotor.run(RELEASE);
}

void rotateServo(int targetAngle) {
  
  int curPosition = servo.read();
  int angleToRotate = targetAngle - curPosition;

  bool up = true;

  if(angleToRotate < 0) {
    angleToRotate = angleToRotate * (-1);
    up = false;
  }

  int steps = angleToRotate / servoSpeed;
  if(angleToRotate % steps != 0) {
    steps++;
  }

  int stepsArr[steps];

  for(int i = 1; i <= steps; i++) {
    stepsArr[i - 1] =  up ? curPosition + i * servoSpeed : curPosition - i * servoSpeed;
    if(i == steps) {
      stepsArr[i - 1] = targetAngle;
    }
  }

  for(int i = 0; i < steps; i++) {
    servo.write(stepsArr[i]);
    delay(20);
  }
  
}

bool doUnloadCheck() {
  bool res = towerDown && boxClose;
  Serial.println(res ? "doUnloadCheck PASS" : "doUnloadCheck FAIL");
  return res;
}

bool doPreLiftCheck() {
  bool res = towerDown && boxClose;
  Serial.println(res ? "doPreLiftCheck PASS" : "doPreLiftCheck FAIL");
  return res;
}

bool doLiftCheck() {
  bool res = towerDown && boxClose;
  Serial.println(res ? "doLiftCheck PASS" : "doLiftCheck FAIL");
  return res;
}

bool doThrowCheck() {
  bool res = towerUp && boxClose;
  Serial.println(res ? "doThrowCheck PASS" : "doThrowCheck FAIL");
  return res;
}

bool doPreLowerCheck() {
  bool res = towerUp && boxClose;
  Serial.println(res ? "doPreLowerCheck PASS" : "doPreLowerCheck FAIL");
  return res;
}

bool doLowerCheck() {
  bool res = towerUp && boxClose;
  Serial.println(res ? "doLowerCheck PASS" : "doLowerCheck FAIL");
  return res;
}

bool doPreUnloadCheck() {
  bool res = towerDown && boxClose;
  Serial.println(res ? "doPreUnloadCheck PASS" : "doPreUnloadCheck FAIL");
  return res;
}


void printSwitchStates() {
  Serial.println("OPEN: " + String(boxOpen) + " | CLOSE: " + String(boxClose) + " | UP: " + String(towerUp) + " | DOWN: " + String(towerDown));
}
