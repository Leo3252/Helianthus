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
int lightSourceToFollow;
int counter = 0;

bool done = false; //flag

const int lightSourceOffset = 200;
const int ambientLightOffset = 400;

void setup() { //initiate everything
  xServo.attach(xPin);
  yServo.attach(yPin);
  Serial.begin(9600);
}

void loop() {

  if (!done) { //if no highest light source found keep searching
   searchForLight('x', 180, false);
   searchForLight('y', 95, true);
   
   if (!lightSourceFound) {
     searchForLight('x', 180, false); //If there's still no light source found keep going
     searchForLight('y', 95, true);
   } 
   else if (lightSourceFound) { //When found
      done = true; // escape the if statement
    }
  }
  
  followLight();

  //If no more light look for it once again if not maybe servo motor to close lid
  
  delay(100);
}

//METHODS..........................................
bool lightSourceFound(int state) {
 
 if(state == 1) {
    return true;
  } else if (state == 0) {
    return false;
  }

}

void searchForLight(char axis,int maxAngleRange, bool needLightSource ) { //Optimized so that it can process x and y directions..........Avoid last operation on second run
  
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

  if(axis == 'x') {xServo.write(anglePositionAtHighestValueX); currentAngleX = anglePositionAtHighestValueX;} //Stores the angle positions so that it can later go back to it
  else if (axis == 'y') {yServo.write(anglePositionAtHighestValueY); currentAngleY = anglePositionAtHighestValueY;}
   
  if (currentHighestLightValue > ambientLightOffset && needLightSource) { //If the highest light source is above a threshold do (So that it ignores ambient light)
    lightSourceFound(1); //Manually set lightsourcefound to true
    lightSourceToFollow = currentHighestLightValue; //This is a value that will be used later on by follow light to make sure it is following that value..........
  } else {lightSourceFound(0);} //Manually set lightsourcefound to false
}

void followLight() { //After finding out highest light source follow it

  checkLogic();

  rightLdrValue = analogRead(rightLdrPin); //Start reading sensors
  leftLdrValue = analogRead(leftLdrPin) ;

  int ldrMeanValue = (rightLdrValue + leftLdrValue) / 2; 

  int difference = abs(rightLdrValue - leftLdrValue); //Find the difference in value of both ldrs
  bool move; 
  bool move2;

  if (difference >= 0 && difference <= 5) { //If the difference is within a threshold then don't move //need to implement light value............. Don't move if meanldr value below +- 100 lightsourcetofollow
   move = false;
  } else {move = true;}
  if(ldrMeanValue <= lightSourceToFollow + lightSourceOffset && ldrMeanValue >= lightSourceToFollow - lightSourceOffset) {
    move2 = true;
  } else {move2 = false;}

  if (rightLdrValue < leftLdrValue && move && move2) { //If rightLdrValue is less than the second and it is beyond the threshold do:
    counter++;
    currentAngleX--; //Increments its angle by one
    xServo.write(currentAngleX); //write it to motor  
  }
  else if (rightLdrValue > leftLdrValue && move && move2) { //Same here
    counter++;
    currentAngleX++;
    xServo.write(currentAngleX);
    
  }
  
  //After some repetition ideally
  if (counter == 30) {
    if(currentAngleX < 60 || currentAngleX > 120) {
      searchForLight('y', 95, false);
    } else if (currentAngleX >= 60 && currentAngleX <= 120) {
      searchForLight('y', 60, false); 
    }
    counter = 0;
    
  }

}


void checkLogic() {
  if (currentAngleX > 180) {
    currentAngleX = 180;
  } else if (currentAngleX < 0) {
    currentAngleX = 0;
  }
}

