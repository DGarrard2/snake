/*
  Snake

  A version of the game "Snake" displayed using Adafruit product #661 (or any compatible 128x32 screen) and two pushbuttons.

  The circuit:
  - display pins connected to Arduino pins as follows:
    - CS to 13
    - RST to 12
    - D/C to 11
    - CLK to 10
    - DATA to 9
    - VIN is not used
    - 3.3Vo to 3.3V
    - GND to GND
  - a pushbutton attached to pin 3 from 3.3V (or 5V); ideally positioned left of the screen
  - a pushbutton attached to pin 2 from 3.3V (or 5V); ideally positioned right of the screen
  - a 10kΩ resistor attached to pin 3 from ground
  - a 10kΩ resistor attached to pin 2 from ground
  - pin 8 connected to RESET (this connection may have to be removed to upload a new sketch to the Arduino)
  - to ensure a random seed, pin A0 shouldn't be connected to anything

  Upon reset, the game starts automatically.
  Pressing the pin 3 button turns the snake counterclockwise.
  Pressing the pin 2 button turns the snake clockwise.
  Upon crashing into itself or any of the screen's borders, the game will end, and pressing either button will restart the game.

  created 21 February 2023
  by Dan Garrard
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  32

// pins
#define RIGHT_BUTTON 2 // turns right from the snake's perspective, i.e. clockwise
#define LEFT_BUTTON  3 // turns left from the snake's perspective, i.e. counterclockwise
#define BOARD_RESET  8
#define OLED_MOSI    9
#define OLED_CLK    10
#define OLED_DC     11
#define OLED_RESET  12
#define OLED_CS     13

Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define BLACK SSD1306_BLACK
#define WHITE SSD1306_WHITE

#define START_SNAKE_LENGTH   3
#define MAX_SNAKE_LENGTH   200 // chosen semi-arbitrarily; limited by dynamic memory

#define DELAY_MILLISECONDS 50

int leftButtonState, leftButtonPreviousState, rightButtonState, rightButtonPreviousState;
int snakeLength, deltaX, deltaY, appleX, appleY;
int x[MAX_SNAKE_LENGTH], y[MAX_SNAKE_LENGTH];

void setup() {
  digitalWrite(BOARD_RESET, HIGH);
  pinMode(BOARD_RESET, OUTPUT);

  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);

  Serial.begin(9600);
  if(!screen.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed."));
    for(;;); // loop forever
  }

  leftButtonState = leftButtonPreviousState = rightButtonState = rightButtonPreviousState = LOW;

  // snake moves right by default
  deltaX = 1;
  deltaY = 0;

  snakeLength = START_SNAKE_LENGTH;

  // snake starts in upper-left corner
  x[0] = 2; y[0] = 0;
  x[1] = 1; y[1] = 0;
  x[2] = 0; y[2] = 0;

  screen.clearDisplay();
  screen.fillScreen(WHITE);

  randomSeed(analogRead(0)); // if pin A0 is disconnected, random noise should cause a random seed
  spawnApple();
}

void loop() {
  leftButtonPreviousState = leftButtonState;
  leftButtonState = digitalRead(LEFT_BUTTON);
  rightButtonPreviousState = rightButtonState;
  rightButtonState = digitalRead(RIGHT_BUTTON);

  // if left button is pressed, turn counterclockwise
  if (leftButtonState == HIGH && leftButtonPreviousState == LOW) {
    if (deltaX == 0) {
      deltaX = deltaY;
      deltaY = 0;
    } else {
      deltaY = -deltaX;
      deltaX = 0;
    }
  }

  // if right button is pressed, turn clockwise
  if (rightButtonState == HIGH && rightButtonPreviousState == LOW) {
    if (deltaX == 0) {
      deltaX = -deltaY;
      deltaY = 0;
    } else {
      deltaY = deltaX;
      deltaX = 0;
    }
  }

  // calculate new position for head
  int tempX = x[0] + deltaX;
  int tempY = y[0] + deltaY;

  // check if snake has eaten apple
  bool appleEaten = (tempX == appleX && tempY == appleY);

  // check for edge-of-screen collisions
  if (tempX < 0 || tempX >= SCREEN_WIDTH || tempY < 0 || tempY >= SCREEN_HEIGHT) {
    gameOver();
  }

  // grow snake or erase last part of snake, depending on whether apple was eaten
  if (appleEaten) {
    snakeLength++;
  } else {
    screen.drawPixel(x[snakeLength - 1], y[snakeLength - 1], WHITE);
  }

  // move the snake forward, segment by segment, checking for collision at every segment
  for (int i = snakeLength - 1; i > 0; i--) {
    if (x[i] == tempX && y[i] == tempY) {
      gameOver();
    }
    x[i] = x[i - 1];
    y[i] = y[i - 1];
  }
  x[0] = tempX;
  y[0] = tempY;

  // draw new head
  screen.drawPixel(tempX, tempY, BLACK);

  // spawn new apple if necessary
  if (appleEaten) {
    spawnApple();
  }

  screen.display();

  delay(DELAY_MILLISECONDS);
}

// displays the "game over" screen
void gameOver() {
  screen.clearDisplay();
  screen.fillScreen(WHITE);
  screen.setTextColor(BLACK);
  screen.setTextSize(2);
  screen.setCursor(10, 0);
  screen.print(F("GAME OVER"));
  screen.setTextSize(1);
  screen.setCursor(0, 16);
  screen.print(F("Press either button\nto restart."));
  screen.display();
  while (true) { // loop forever
    leftButtonPreviousState = leftButtonState;
    leftButtonState = digitalRead(LEFT_BUTTON);
    rightButtonPreviousState = rightButtonState;
    rightButtonState = digitalRead(RIGHT_BUTTON);
    // if either button is pressed, reset the board
    if ((leftButtonState == HIGH && leftButtonPreviousState == LOW)
     || (rightButtonState == HIGH && rightButtonPreviousState == LOW)) {
      digitalWrite(BOARD_RESET, LOW);
    }
  }
}

// creates a new apple at a valid location
void spawnApple() {
  // try random locations until a free location is found
  bool applePlaced = false;
  while (!applePlaced) {
    appleX = random(SCREEN_WIDTH);
    appleY = random(SCREEN_HEIGHT);
    applePlaced = true; // apple is placed unless collision is found; for loop checks for collision
    for (int i = 0; i < snakeLength; i++) {
      if (x[i] == appleX && y[i] == appleY) {
        applePlaced = false;
        i = snakeLength; // break for-loop
      }
    }
  }
  screen.drawPixel(appleX, appleY, BLACK);
}
