/******************************
 *  Game made by Grigore Dan  *
 * Inspired from my childhood *
 ******************************/

#include <LiquidCrystal.h>
#include "LedControl.h"

/*
 * RED led connected to pin 5
 * GREEN led connected to pin 6
 
 * Joystick 
  pin A0 is connected to vertical movement (oX)
  pin A1 is connected to horizontal movement (oY)
 
 * LCD display
  pin 3 is connected to V0
  pin 4 is connected to RS
  pin 7 is connected to EN
  pin 8 is connected to D4
  pin 9 is connected to D5
  pin 10 is connected to D6
  pin 11 is connected to D7
 
 * DRIVER MAX7219
  pin 12 is connected to the DataIn
  pin A3 is connected to the CLK
  pin A4 is connected to LOAD
  
*/

typedef struct {
  int leftCol, topRow, rightCol, bottomRow;
  bool crashed;
} player;

typedef struct {
  int line, type, gate;
  unsigned long previousPipeMillis;
} pipeStructure;

const int RS = 4, EN = 7, D4 = 8, D5 = 9, D6 = 10, D7 = 11, V0 = 3;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

LedControl lc = LedControl(12, A3, A4, 1);
const int joyStickX = A0;
const int joyStickY = A1;
const int joystickButtonPin = 2;

// numbers that represent the gap in the pipe
const int possiblePipeGeneration[] = {63, 159, 207, 231, 243, 249, 252}; 

bool gameState = false;
bool joystickButtonState;
bool buttonPressed = false;
int redLed = 5;
int greenLed = 6;

int oX = 0, oY = 0;
player airplane;
pipeStructure firstPipe, secondPipe;

int previousRandomNumber = 3;
int pipeLine = 0, pipeType = 231;
int pipeSpeed = 600;
int airplaneSpeed = pipeSpeed / 2;
int generationInterval = 3000;
int pipeNumbers = 0;
int targetTime = pipeSpeed * 6; // Generate first appearance of 2nd pipe.
int blinkDuration = 600;
int numberOfLives = 3;
int firstGeneration = 1;
int level = 1;

unsigned long currentMillis = 0;             // stores the value of millis() in each iteration of loop()
unsigned long previousAirplaneMillis = 0;    // will store last time airplane was updated
unsigned long previousBlinkMillis = 0;
unsigned long previousPipeMillis = 0;
unsigned long previousGeneratingTime = 0;
unsigned long auxMillis = 0; // stores the time that passed before the button was pressed
unsigned long previousColissionMillis = 0;
bool colissionState = false;

void setup() 
{
  // LCD setup
  analogWrite(V0, 25);
  lcd.begin(16, 2);

  lcd.setCursor(3, 0);
  lcd.print("AirMaster");
  lcd.setCursor(0, 1);
  lcd.print("Press the button");

  // LED setup
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  analogWrite(greenLed, 1);

  // Driver setup
  lc.shutdown(0, false);
  lc.setIntensity(0, 3);
  lc.clearDisplay(0);

  // Joystick setup
  pinMode(joystickButtonPin, INPUT);
  randomSeed(analogRead(6)); // generate random sequence at each start.

  // Airplane setup (starting position)
  airplane = {3, 6, 4, 7, false};
  //Turning on the LEDs for the plane.
  lc.setLed(0, airplane.leftCol, airplane.topRow, true);
  lc.setLed(0, airplane.rightCol, airplane.bottomRow, true);
  lc.setLed(0, airplane.leftCol, airplane.bottomRow, true);
  lc.setLed(0, airplane.rightCol, airplane.topRow, true);

  // Pipe setup
  // First pipe with the gap in the middle (like the airplane's position).
  firstPipe = {0, possiblePipeGeneration[3], 3, 0};
  lc.setColumn(0, firstPipe.line, firstPipe.type);

  // Console debugging
  Serial.begin(9600);
}

void loop()
{
  if (!gameState)
  {
    joystickButtonState = digitalRead(joystickButtonPin);
    auxMillis = millis();
    if (joystickButtonState == HIGH && !buttonPressed)
    {
      analogWrite(greenLed, 0);
      gameState = true;
      buttonPressed = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SC  STARTED   LI"); // Score label
      lcd.setCursor(0, 1);
      lcd.print(pipeNumbers);
      lcd.setCursor(6, 1);
      lcd.print("L");
      lcd.setCursor(7, 1);
      lcd.print(level);
      lcd.setCursor(14, 1);
      lcd.print(numberOfLives);
    }
    else
      gameState = false;
  }
  else
  {
    // currentMillis = millis() - auxMillis - abs(numberOfLives - 3) * 600;
    currentMillis = millis() - auxMillis;
    movePipe(firstPipe);
    if (secondPipe.type != 0)
      movePipe(secondPipe);

    // Generation of 2nd pipe (used a single time in the beginning of the game)
    if (currentMillis > firstGeneration * targetTime)
    {
      if (firstGeneration % 2 == 1)
      {
        secondPipe.previousPipeMillis = firstPipe.previousPipeMillis - pipeSpeed;
        generatePipe(secondPipe);
        movePipe(secondPipe);
        firstGeneration++;
      }
    }
    
     // Colission conditions
    if (colission(firstPipe.line, firstPipe.gate) == true && airplane.crashed == false || colission(secondPipe.line, secondPipe.gate) == true)
    {
      if (numberOfLives > 0)
      {
        if (currentMillis - previousColissionMillis >= 2 * pipeSpeed)
        {
          numberOfLives --;

          lcd.setCursor(14, 1);
          lcd.print(numberOfLives);

          previousColissionMillis = currentMillis;
        }
      }
      else
      {
        analogWrite(greenLed, 0);
        analogWrite(redLed, 1);

        gameState = false;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Game over!");
        lcd.setCursor(0, 1);
        lcd.print("Score:");
        lcd.setCursor(7, 1);
        lcd.print(pipeNumbers);
      }
      colissionState = true;
    }

    // When the airplane is in the pipe, the green LED is turned on
    if (firstPipe.line == 7 || secondPipe.line == 7)
      if(!colissionState)
        analogWrite(greenLed, 1);
      else 
        analogWrite(redLed, 1);
    // When its out of matrix, it generates another pipe.
    if (firstPipe.line == 9)
    {
      generatePipe(firstPipe);
      firstPipe.previousPipeMillis = secondPipe.previousPipeMillis;
      movePipe(firstPipe);

      pipeNumbers ++;

      lcd.setCursor(0, 1);
      lcd.print(pipeNumbers);
      analogWrite(greenLed, 0); // turning off the green led which I opened while the plane was in the pipe
      analogWrite(redLed, 0);

      colissionState = false;
    }

    if (secondPipe.line == 9) // -------------------------------------------------
    {
      secondPipe.previousPipeMillis = firstPipe.previousPipeMillis;
      generatePipe(secondPipe);
      movePipe(secondPipe);

      pipeNumbers++;

      lcd.setCursor(0, 1);
      lcd.print(pipeNumbers);
      analogWrite(greenLed, 0);
      analogWrite(redLed, 0);

      colissionState = false;
    }

    // At 10 pipes, the pipe generation and airplane speed are increased
    if (pipeNumbers >= level * 10 && level <= 4) {
      pipeSpeed -= 100;
      airplaneSpeed = pipeSpeed / 2;
      level++;
      lcd.setCursor(7, 1);
      lcd.print(level);
    }

    moveThePlane();
  }

}

bool colission(int line, int gate) 
{
  if (!(gate == airplane.leftCol && gate + 1 == airplane.rightCol) && (line == airplane.topRow + 1 || line == airplane.bottomRow + 1))
    return true;
  return false;
}

void movePipe(pipeStructure& x)
{
  if (currentMillis - x.previousPipeMillis >= pipeSpeed)
  {
    lc.setColumn(0, x.line - 1, 0);
    if (x.line <= 7)
      lc.setColumn(0, x.line, x.type);
    x.line++;
    reopen();
    x.previousPipeMillis += pipeSpeed;
  }
}

void generatePipe(pipeStructure& x)
{
  int randomNumber = random(100);
  randomNumber %= 7;

  // In order to not have an impossible move from pipe to pipe,
  // I let the distance between gates be maximum 4 units
  while (abs(previousRandomNumber - randomNumber) > 4)
    randomNumber = random(100) % 7;

  // while (previousRandomNumber == randomNumber)
  //   randomNumber = random(100) % 7;

  previousRandomNumber = randomNumber;
  x.type = possiblePipeGeneration[randomNumber];
  x.line = 0;
  x.gate = randomNumber;
  lc.setColumn(0, x.line, x.type); x.line++;
}

void moveThePlane()
{
  oX = map(analogRead(joyStickX), 0, 1023, -5, 6);
  oY = map(analogRead(joyStickY), 0, 1023, -5, 5);

  if (currentMillis - previousAirplaneMillis >= airplaneSpeed)
  {
    if (airplane.leftCol != 0 && oX < 0)
    {
      reset();
      airplane.leftCol--;
      airplane.rightCol --;
      reopen();
    }
    if (airplane.rightCol != 7 && oX > 0)
    {
      reset();
      airplane.leftCol++;
      airplane.rightCol++;
      reopen();
    }
    previousAirplaneMillis += airplaneSpeed;
  }
}

// Function to turn off the LEDs of the plane
void reset()
{
  lc.setLed(0, airplane.leftCol, airplane.topRow, false);
  lc.setLed(0, airplane.leftCol, airplane.bottomRow, false);
  lc.setLed(0, airplane.rightCol, airplane.bottomRow, false);
  lc.setLed(0, airplane.rightCol, airplane.topRow, false);
}

// Function to turn on the LEDs of the plane
void reopen()
{
  lc.setLed(0, airplane.leftCol, airplane.topRow, true);
  lc.setLed(0, airplane.leftCol, airplane.bottomRow, true);
  lc.setLed(0, airplane.rightCol, airplane.bottomRow, true);
  lc.setLed(0, airplane.rightCol, airplane.topRow, true);
}

