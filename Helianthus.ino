#include <Servo.h>

Servo xServo; 
Servo yServo;

//Fixed values
const int rightLdrPin = A0; 
const int leftLdrPin = A1; 
const int xPin = 9;
const int yPin = 10;

//Declaring variables
int rightLdrValue = 0;
int leftLdrValue = 0; 
int currentAngleX; //Used to alter the current angle positions
int currentAngleY;
int lightSourceToFollow;
int terminateAtSecondSearch; //Stops looking for sunlight after 2 searches
int counter = 0; //To calc the number of rotations made by servo
int timer = 0; //timer
bool done = false; //flag
bool lightSourceFound;
bool isOffsetOn = false;

//Alter-able value
const int lightSourceOffset = 200; //To keep following the initial light intensity
const int ambientLightOffset = 400; //To make sure you're pointing at a light source
const int yOffset = 20; //To offset the distance between panel and sensors
const int sensorDif = 250;
const int idleTime = 200; //Time taken for offset to kick in (200 = 20sec)

//OBTAINING WATTAGE:
const int voltagePin = A2;
const int currentPin = A3;
const int numberToMultiply = 5; //Convertion

//Bluetooth
char receivedData;
String operation = "FOLLOW_LIGHT"; //Initial op. of the method
bool infoPresent; //If there's info flag

void setup() { //initiate everything
  xServo.attach(xPin);
  yServo.attach(yPin);
  Serial.begin(9600);
}

void loop() {
  if(terminateAtSecondSearch >= 2) {return;} //Maybe turn on led indicator
  else{
    timer++;

    if (!done) { //if no highest light source found keep searching
    searchForLight('x', 150, false); //Search at x axis, through 150º, don't ask for highest light source
    searchForLight('y', 95, true); //Search at y axis, through 95º, ask for highest light source value
    terminateAtSecondSearch++;
    
    if (!lightSourceFound) { //If there's still no light source found keep going
      currentAngleX = 95;
      xServo.write(currentAngleX);
      searchForLight('x', 150, false); //Same as above
      searchForLight('y', 95, true);
      if(!lightSourceFound){terminateAtSecondSearch++;}
      else if (lightSourceFound) {done = true;}
    } 
    else if (lightSourceFound) { //When found
        done = true; // escape the if statement
      }
    }
    
    determineOperation(); //We need to find out what task should we carry out from the options below
    
    //Tasks
    //We initialized operation to follow light so that it'll follow the light until otherwise
    if(operation == "READ_WATTAGE") { //When operation is set to the wattage reader do:
      outputWattageToApp(); //Sent wattage info to app
      delay(900); //This plus the 100ms delay at the end sum to 1sec, which is the required interval
    } 
    else if(operation == "MOVE_TRACKER") { //When operation is set to manually move tracker do:
      /*The code below is important: We set operation to move tracker but if no bool flag present
      operation will remain as move tracker and it will continue to move the robot in the first direction 
      you give. To fix this we need to know when instructions are coming in from the app to move the tracker
      w/o compromising the current method (using variable "operation" to define task). 
      So, a run down on how it works:
      First call of void loop, operation is MOVE TRACKER, because no buttons are pressed on the app we "return"
      to escape the else if statement.
      Second call of void loop, we get button pressed, infoPresent becomes true, so we them interpret the data 
      coming from the app and act accordingly.
      */
      if(!infoPresent) {return;} 
      interpretData(receivedData); //Parameter is the data we receive
    } 
    else if(operation == "FOLLOW_LIGHT") { //When no operaion is running we:
      followLight(); //Follow light
    }
    
    checkLogic();

    positionSolarPanelAtBetterAngle();

    
    delay(100);
  }
}

//METHODS in order of appearance........................................................

//Optimized so that it can process x and y directions without the need of two different methods
void searchForLight(char axis, int maxAngleRange, bool needLightSource ) { //Last parameter is to prevent assigning highest light value when not required
  
  int currentHighestLightValue = 0; //Used to compare between the highest and current light values
  
  if(axis == 'x') {currentAngleX = 0; xServo.write(currentAngleX);} //This is a tradeoff if I were to optimize the code
  else if (axis == 'y') {currentAngleY = 0; yServo.write(currentAngleY);} //Initializes servomotor to 0 degrees so that it can scan around
  
  for(int angle = 0; angle <= maxAngleRange; angle++) { //Using the value given by the parameter, the scan will go up until it satisifies the statement
    
    int ldrMeanValue; //New mean value every iteration
    rightLdrValue = analogRead(rightLdrPin); //Update ldr values for each iteration
    leftLdrValue = analogRead(leftLdrPin); 
    
    ldrMeanValue = (rightLdrValue + leftLdrValue) / 2;  //Calc mean
    
    if(ldrMeanValue > currentHighestLightValue) { //If value surpasses previous highest value do: 
      currentHighestLightValue = ldrMeanValue; //Set new highest value
      
      if(axis == 'x') {currentAngleX = angle;} //Stores the angle positions so that it can later, retrace its steps
      else if (axis == 'y') {currentAngleY = angle;}
      Serial.println(currentAngleY);
    }
    
    if(axis == 'x') {xServo.write(angle);} //Move the servo by 1 degree every iteration
    else if (axis == 'y') {yServo.write(angle);}
    
    delay(50);
  }

  //Set servo to the highest light intensity location 
  if(axis == 'x') {xServo.write(currentAngleX);}
  else if (axis == 'y') {yServo.write(currentAngleY);}
   
  if (currentHighestLightValue > ambientLightOffset && needLightSource) { //If the highest light source is above a threshold do (So that it ignores ambient light)
    lightSourceFound = true; 
    lightSourceToFollow = currentHighestLightValue; //This is a value that will be used later on by follow light to make sure it is following that value..........
  } else {lightSourceFound = false;}

  timer = 0;
}

void determineOperation() { //Find out what we're doing
  if(Serial.available() > 0) { //If there's info sent do:
    infoPresent = true; //A flag which is required for later operation
    receivedData = Serial.read(); //Obtain data
    
    if(receivedData == 'x') { //If data matches char 'x' do:
      operation = "MOVE_TRACKER"; //Initiate bluetooth operation
    }
    else if (receivedData == 'y'){ //Initiate wattage reader
      operation = "READ_WATTAGE";
    }
    else if (receivedData == 'z') { //When connection is terminated do:
      operation = "FOLLOW_LIGHT"; //Start following light
    }
    timer = 0;
  } else{infoPresent = false;} //If no info sent set flag to false
}

void outputWattageToApp() { //We output wattage values to the app
  float wattage = readWattage(); //Find wattage
  wattage = abs(wattage); //Make it positive
  byte outputWattage = (map(wattage*100, 0, 100, 0, 25500)/100); //Convert wattage value to 0,255 (Max 1mW)
  Serial.write(outputWattage);
}

float readWattage() {
  int voltageValue = analogRead(voltagePin);
  float voltage = (voltageValue/1023.0) * numberToMultiply;
  float current = analogRead(currentPin) * (numberToMultiply / 1023.0);
  current = (current - (numberToMultiply / 2.0)) / 0.185;
  return voltage * current; //Return wattage in (W)
}

void interpretData(char data) { //Find out what data is being sent by app
  if(data == '1' && (currentAngleY - 5) >= 5) { //UP
    currentAngleY = currentAngleY - 5;
    yServo.write(currentAngleY);
  } else if (data == '2' && (currentAngleY + 5) <= 90) { //DOWN
    currentAngleY = currentAngleY + 5;
    yServo.write(currentAngleY);
  } else if (data == '3' && (currentAngleX + 10) <= 150) { //RIGHT
    currentAngleX = currentAngleX + 10;
    xServo.write(currentAngleX);
  } else if (data == '4' && (currentAngleX - 10) >= 0) { //LEFT
    currentAngleX = currentAngleX - 10;
    xServo.write(currentAngleX);
  }
  timer = 0;
  isOffsetOn = false;
}

void followLight() { //After finding out highest light source follow it 
  checkLogic();
  
  rightLdrValue = analogRead(rightLdrPin); //Start reading sensors
  leftLdrValue = analogRead(leftLdrPin) ;
  /*Serial.print("rightLdr: ");
  Serial.println(rightLdrValue);
  Serial.print("leftLdr: ");
  Serial.println(leftLdrValue);*/
  
  int ldrMeanValue = (rightLdrValue + leftLdrValue) / 2; 
  int difference = abs(rightLdrValue - leftLdrValue); //Find the difference in value of both ldrs
  /*Serial.print("ldrMean: ");
  Serial.println(ldrMeanValue);*/
  Serial.print("Difference: ");
  Serial.println(difference);
  
  bool move; //We need to check to things
  bool move2;
  
  if (difference >= 0 && difference <= 15) { //If the difference is within a threshold then don't move
  } else {move = true;}
  
  //Make sure we're following the same light source, unaffected by other's
  if(ldrMeanValue <= lightSourceToFollow + lightSourceOffset && ldrMeanValue >= lightSourceToFollow - lightSourceOffset) { 
    move2 = true;
  } else {move2 = false;}
  
  if ((rightLdrValue - sensorDif) < leftLdrValue && move && move2) { //If rightLdrValue is less than the second and it is beyond the threshold do:
    counter++; //Count so that we know when to update y-angle position
    currentAngleX--; //Increments its angle by one
    xServo.write(currentAngleX); //write it to motor  
    timer = 0;
    isOffsetOn = false;
  }
  else if ((rightLdrValue - sensorDif) > leftLdrValue && move && move2) { //Same here
    counter++;
    currentAngleX++;
    xServo.write(currentAngleX);
    timer = 0;
    isOffsetOn = false;
    
  }
  
  //After 30 +- rotations check for y anlge
  if (counter == 30) {
    searchForLight('y', 95, false);
    counter = 0; //Reset the counter
    
  }
}

void checkLogic() { //If current angle is above or below allowed threshold then set them back to original value
  if (currentAngleX > 150) { //Checking X-axis 
    currentAngleX = 150;
  } else if (currentAngleX < 0) {
    currentAngleX = 0;
  }
  if (currentAngleY > 95) { //Checking Y-axis
    currentAngleY = 95;
  } else if (currentAngleY < 0) {
    currentAngleY = 0;
  }
}

void positionSolarPanelAtBetterAngle() {
  if (timer > idleTime) { //+- 20 seconds 
      timer = 0; //Reset clock
      if (currentAngleY >= yOffset && !isOffsetOn) {
        currentAngleY = currentAngleY - yOffset; //Make sure pointing directly at sun
        yServo.write(currentAngleY);
        isOffsetOn = true;
      } else {return;}
    } 
}