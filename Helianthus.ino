#include <Servo.h>

Servo xServo; 
Servo yServo;

int rightLdrPin = A0; 
int leftLdrPin = A1; 
int xPin = 9;
int yPin = 10;

int rightLdrValue = 0;
int leftLdrValue = 0; 
int anglePositionAtHighestValueX = 0; //variable to store highest light value
int anglePositionAtHighestValueY = 0;
int lightValueOfX; //Stores the highest light value found by "search for light"
int lightValueOfY;
int currentAngleX; //Used to alter the current x position
int currentAngleY;

bool done = false; //flag

void setup() { //initiate everything
  xServo.attach(xPin);
  yServo.attach(yPin);
  Serial.begin(9600);
}

void loop() {

  if (!done) { //if no highest light source found keep searching
   lightValueOfX = searchForLight('x', 180);
   lightValueOfY = searchForLight('y', 95);
   
   if (!lightSourceFound) {
     lightValueOfX = searchForLight('x', 180); //If there's still no light source found keep going
     lightValueOfY = searchForLight('y', 95);
   } 
   else if (lightSourceFound) { //When found
      xServo.write(anglePositionAtHighestValueX); //Return the x position to the highest light source
      currentAngleX = anglePositionAtHighestValueX; //Set the current x angle to the above value too
      yServo.write(anglePositionAtHighestValueY); 
      currentAngleY = anglePositionAtHighestValueY;
      
      done = true; // escape the if statement
    }
  }
  
  followLight();

  //If no more light look for it once again if not maybe servo motor to close lid
  
  delay(50);
}

//METHODS..........................................
bool lightSourceFound(int state) {
 
 if(state == 1) {
    return true;
  } else if (state == 0) {
    return false;
  }

}

int searchForLight(char axis,int maxAngleRange ) { //Optimized so that it can process x and y directions
  
  int currentHighestLightValue = 0; //Used to compare between the highest and current light values
  
  if(axis == 'x') {xServo.write(0);} //This is a tradeoff if I were to optimize the code
  else if (axis == 'y') {yServo.write(0);} //Initializes servomotor to 0 degrees so that it can scan around

  for(int angle = 0; angle <= maxAngleRange; angle++) { //Using the value given by the parameter, the scan will go up until it satisifies the statement
    int ldrMeanValue;

    rightLdrValue = analogRead(rightLdrPin); //Update ldr values for each iteration
    leftLdrValue = analogRead(leftLdrPin); 

    ldrMeanValue = (rightLdrValue + leftLdrValue) / 2; 

    if(ldrMeanValue > currentHighestLightValue) { 
      currentHighestLightValue = ldrMeanValue;
      if(axis == 'x') {anglePositionAtHighestValueX = angle;} //Stores the angle positions so that it can later go back to it
      else if (axis == 'y') {anglePositionAtHighestValueY = angle;}
    }
    
    if(axis == 'x') {xServo.write(angle);} //Move the servo by 1 degree every iteration
    else if (axis == 'y') {yServo.write(angle);}
    
    delay(50);
  }

  if (currentHighestLightValue > 400) { //If the highest light source is above a threshold do (So that it ignores ambient light)
    lightSourceFound(1); //Manually set lightsourcefound to true
    return currentHighestLightValue; //This is a value that will be used later on by follow light to make sure it is following that value..........
  } else {lightSourceFound(0); return 0;} //Manually set lightsourcefound to false
}

void followLight() { //After finding out highest light source follow it
  
  rightLdrValue = analogRead(rightLdrPin); //Start reading sensors
  leftLdrValue = analogRead(leftLdrPin) ;

  int difference = abs(rightLdrValue - leftLdrValue); //Find the difference in value of both ldrs
  bool move; 

  if (difference >= 0 && difference <= 100) { //If the difference is within a threshold then don't move //need to implement light value.............
    move = false;
  } else {move = true;}

  if (rightLdrValue < leftLdrValue && move) { //If rightLdrValue is less than the second and it is beyond the threshold do:
    currentAngleX++; //Increments its angle by one
    xServo.write(currentAngleX); //write it to motor
 
  }
  else if (rightLdrValue > leftLdrValue && move) { //Same here
    currentAngleX--;
    xServo.write(currentAngleX);
  }
}

//Algorithm to search for light
//Focus on 1 ldr, if value decreases move up a little bit, store information and move left a little bit store info
//Compare both data and decide where to go
//Mirror functionality to the other ldr
void lightDetectionAlgorithm() {
  rightLdrValue = analogRead(rightLdrPin); //Start reading sensors
  leftLdrValue = analogRead(leftLdrPin) ;
  int difference = abs(rightLdrValue - leftLdrValue); //Find the difference in value of both ldrs
  bool move; 

  if (rightLdrValue < leftLdrValue && move)
}
