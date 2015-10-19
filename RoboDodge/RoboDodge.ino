/*----------------------------------
 *             ROBO DODGE
 *          
 *    Control a little robot as he
 *   dodges incoming projectiles.
 *    Grab power ups to increase
 *   your speed and health!
 *          
 *             Created by:
 *             
 *        Gavin Atkin (Gaveno)
 *        
 *        
 *         Graphics Converted
 *               Using:
 *               
 *           LCD Assistant
 *           
 *                &
 *                
 *      Arduboy Image Converter
 *         By: Andrew Lowndes
 *         
 *     
 *        Game Copyright (C) 2015
 *         
 *           Monkey Studios
 *
 ----------------------------------*/

#include <SPI.h>
#include <EEPROM.h>
#include "Arduboy.h"
#include "Bitmaps.h"

Arduboy display;

unsigned long lTime;
const byte FPS = 1000/30;

uint8_t gray = 0;
float frame = 0;
float animSpeed = 0.2;
uint8_t totalFrames = 4;

int pX = 20;
int pY = 0;
uint8_t pSpeed = 0; // speed moved
uint8_t pASpeed = 1; // players actual speed
int prevPX = 20;
int prevPY = 0;
int pDir = 270;

uint8_t boundY = 10;
uint8_t boundH = 10;

const uint8_t MAXOB = 5;
const uint8_t MAXSPD = 4;

//uint8_t obActive = 1;
float obSpeed = 1;
uint8_t difficulty = 1;
const uint8_t MAXLIVES = 12;
uint8_t lives = 3;
uint8_t level = 1;
int obLeft = 20;
uint8_t spCount = 0;


/*----------------------
 *  Power Ups:
 *  Extra Hearts
 *  Speed Boost
--------------------- */
struct PowerUps
{
  float x = 128;
  float y = 32;
  uint8_t type = 0; //0: heart, 1: speed
  bool active = false;
  float xspd = 0;
  float yspd = 0;
} powerup;

/*    Create Power Up
 *    Spawn a power up
 */
void createPowerUp(uint8_t type)
{
  powerup.xspd = 1;
  powerup.yspd = 1.5;
  powerup.x = 128;
  powerup.y = 10 + random(44);
  powerup.type = type;
  powerup.active = true;
}
/*
 *  Move Power Up
 */
 void movePowerUp()
 {
    powerup.x -= powerup.xspd;
    powerup.y -= powerup.yspd;
    if (powerup.x < -8)
      powerup.active = false;

    if (powerup.y <= 10)
      powerup.yspd = -1.5;
    if (powerup.y >= 46)
      powerup.yspd = 1.5;

    collisionPowerUp();
 }

 /*-----------------------------
  *   Check Power Up Collision
  ----------------------------*/
void collisionPowerUp()
{
  if (powerup.active)
  {
    if (powerup.x < pX + 8 &&
    powerup.x + 8 > pX &&
    powerup.y < pY + 8 &&
    8 + powerup.y > pY)
    {
      if (powerup.type == 0) // heart
      {
        lives++;
        powerup.active = false;
        powerup.x = 128;
      }
      if (powerup.type == 1) // speed
      {
        pASpeed++;
        powerup.x = 128;
        powerup.active = false;
      }
    }
  }
}
/*------------------
 *  Draw Power Up
----------------- */
void drawPowerUp()
{
  if (powerup.active)
  {
    if (powerup.type == 0)
      display.drawBitmap(powerup.x,powerup.y,bHeart, 8, 8, 1);
    if (powerup.type == 1)
      display.drawBitmap(powerup.x,powerup.y,bSpeed, 8, 8, 1);
  }
}

/*-----------------------
 *  Projectiles
 ----------------------*/
struct Objects
{
  float x = 128;
  float y = 32;
  float spd = 1;
  int p = 10;
  bool active = false;
  float swerve = false;
};

Objects objs[MAXOB];

/*------------------------------------------------------------
 *  Point in Rectangle
----------------------------------------------------------- */
bool pointInRect(int x, int y, int x1, int y1, int x2, int y2)
{
  return (x >= x1 && x <= x2 && y >= y1 && y <= y2);
}

/*---------------------------
 *  Collison with player
 --------------------------*/
 bool checkCollision()
 {
    for (int i = 0; i < MAXOB; i++)
    {
        return (pointInRect(objs[i].x, objs[i].y, pX, pY, pX + 8, pY + 8));
    }
 }

/*--------------------
 *  Start given level
 -------------------*/
void levelStart(int lvl)
{
  obSpeed = min(1 + (lvl / 5), MAXSPD);
  difficulty = 1;
  pASpeed = 1;
  
  if (lvl == 1)
  {
      lives = 3; // game over or restart
      display.clearDisplay();
      display.drawBitmap(17,10,bStart, 96, 48, 1);
      display.display();
      delay( 2000 );
  }
  else
  {
      display.clearDisplay();
      display.drawBitmap(17,10,bClear, 96, 48, 1);
      display.display();
      delay( 2000 );
  }
  level = lvl;
  obLeft = lvl*10;
  pY = 24;

  for (int i = 0; i < MAXOB; i++)
  {
    objs[i].x = 128;
    objs[i].y = boundY + random(62 - boundH - boundY);
    objs[i].spd = obSpeed;
    objs[i].p = 30;
    objs[i].active = false;
  }
  objs[0].active = true;
}

/*--------------------------
 *  Active Object
 *  
 *  Finds an available
 *  object to activate
 *  if max is not reached
 -------------------------*/
 void activateObject()
 {
    for (int i = 0; i < MAXOB; i++)
    {
        if (!objs[i].active)
        {
            objs[i].active = true;
            return;
        }
    }
 }

/*----------------------------
 *  Compute objects movement
 ---------------------------*/
void moveObjects()
{
  int numActive = 0;
  for (int i = 0; i < MAXOB; i++)
  {
    if (objs[i].active)
    {
     if (objs[i].p > 0) // paused
     {
        objs[i].p--;
     }
     else
     {    // not paused
     objs[i].x -= objs[i].spd;
     objs[i].y -= objs[i].swerve;
     if (objs[i].y < 10)
        objs[i].swerve = -1;
     if (objs[i].y > 53)
        objs[i].swerve = 1;
     if (objs[i].x < -6)
     {
        if (obLeft > 0)
        {
          obLeft--;
          objs[i].p = random(20); //pause
          objs[i].x = 128;
          objs[i].y = boundY + random(62 - boundH - boundY);
          objs[i].spd = min(objs[i].spd + ((float)level / 20), MAXSPD);

          if (level >= 5 && random(10) < 5)
          {
            objs[i].swerve = random(3) - 1;
          }
          else
            objs[i].swerve = 0;
        }
        else
        {
            objs[i].active = false;
        }
     }
     /*
      *  Check collison
      */
      if (pointInRect(objs[i].x, objs[i].y, pX, pY, pX + 8, pY +8))
      {
        obLeft--;
        objs[i].p = random(20); //pause
        objs[i].x = 128;
        objs[i].y = boundY + random(62 - boundH - boundY);
        objs[i].spd = min(objs[i].spd + ((float)level / 20), MAXSPD);
        //objs[i].spd += ((float)level / 20);
          
        lives--;
        if (lives <= 0)
        {
          displayTitle();
          levelStart(1);
        }
      }
     }
    }

     if (objs[i].active)
        numActive++;
  }

  if (numActive == 0)
      levelStart(level + 1);

  spCount++;
  if (spCount == 200)
  {
      spCount = 0;
      if (random(10) < 3)
      {
          createPowerUp(random(2));
      }
      if (numActive < MAXOB)
        activateObject();
  }
}

/*-------------------------------
 *  Draw objects
 ------------------------------*/
 void drawObjects()
 {
    for (int i = 0; i < MAXOB; i++)
    {
      if (objs[i].p <= 0 && objs[i].active)
      {
      display.drawPixel(objs[i].x, objs[i].y, 1);
      display.drawPixel(objs[i].x+1, objs[i].y, 1);
      display.drawPixel(objs[i].x, objs[i].y+1, 1);
      display.drawPixel(objs[i].x+1, objs[i].y+1, 1);
      display.drawPixel(objs[i].x+2, objs[i].y + gray, 1);
      display.drawPixel(objs[i].x+3, objs[i].y + 1 - gray, 1);
      display.drawPixel(objs[i].x+4, objs[i].y - 1 + random(3), 1);
      display.drawPixel(objs[i].x+5, objs[i].y + 1 - random(3), 1);
      display.drawPixel(objs[i].x+6, objs[i].y + random(3), 1);
      display.drawPixel(objs[i].x+7, objs[i].y + 1 - random(3), 1);
      }
    }
 }

 /*--------------------------
  *   Draw Header
  -------------------------*/
  void drawHeader()
  {
      display.fillRect(0, 0, 127, 10, 1);
      for (int i = 0; i < lives; i++)
      {
         display.drawBitmap(3 + (i * 10) ,1 , bHeart, 8, 8, 0);
      }

      display.fillRect(0, 54, 127, 63, 1);
      display.setCursor(3, 55);
      display.write('L');
      display.write('E');
      display.write('V');
      display.write('E');
      display.write('L');
      display.write(' ');
      display.print(level);
      display.drawLine(2,55,2,62,0);

      display.setCursor(59, 55);
      display.print("SPAWNS: ");
      display.print(obLeft);
      display.drawLine(58,55,58,62,0);
  }


/*-------------------------
 *  Draw Player
------------------------ */
void drawPlayer()
{
  if (pDir == 270)
  {
    if (frame < 1)
      display.drawBitmap(pX ,pY, bGWD0, 8, 8, 1);
    else if (frame < 2)
      display.drawBitmap(pX ,pY, bGWD1, 8, 8, 1);
    else if (frame < 3)
      display.drawBitmap(pX ,pY, bGWD0, 8, 8, 1);
    else if (frame < 4)
      display.drawBitmap(pX ,pY, bGWD2, 8, 8, 1);
  }
  if (pDir == 90)
  {
    if (frame < 1)
      display.drawBitmap(pX ,pY, bGWU0, 8, 8, 1);
    else if (frame < 2)
      display.drawBitmap(pX ,pY, bGWU1, 8, 8, 1);
    else if (frame < 3)
      display.drawBitmap(pX ,pY, bGWU0, 8, 8, 1);
    else if (frame < 4)
      display.drawBitmap(pX ,pY, bGWU2, 8, 8, 1);
  }
  if (pDir == 0)
  {
    if (frame < 1)
      display.drawBitmap(pX ,pY, bGWR0, 8, 8, 1);
    else if (frame < 2)
      display.drawBitmap(pX ,pY, bGWR1, 8, 8, 1);
    else if (frame < 3)
      display.drawBitmap(pX ,pY, bGWR0, 8, 8, 1);
    else if (frame < 4)
      display.drawBitmap(pX ,pY, bGWR1, 8, 8, 1);
  }
  if (pDir == 180)
  {
    if (frame < 1)
      display.drawBitmap(pX ,pY, bGWL0, 8, 8, 1);
    else if (frame < 2)
      display.drawBitmap(pX ,pY, bGWL1, 8, 8, 1);
    else if (frame < 3)
      display.drawBitmap(pX ,pY, bGWL0, 8, 8, 1);
    else if (frame < 4)
      display.drawBitmap(pX ,pY, bGWL1, 8, 8, 1);
  }
}

/*-----------------
 *  Display Title
 ----------------*/
void displayTitle()
{
  int flash = 0;
  while (true)
  {
    delay( 30 );
    display.clearDisplay();
    display.drawBitmap(14, 1, bTitle, 104, 48, 1);
    flash++;
    flash %= 50;

    if (flash < 25)
    {
      display.setCursor(30, 54);
      display.print("Press A or B");
    }
    
    display.display();

    if (display.pressed(A_BUTTON) || display.pressed(B_BUTTON))
      break;
  }
  
}


/*-----------------------
 * 
 *        SETUP
 *  
 ----------------------*/
void setup() {
  // put your setup code here, to run once:
  Serial.print("Entering Setup");
  display.start();
  
  
  displayTitle();
  

  levelStart(1);
  lTime = millis();
  display.initRandomSeed();
}


/*-----------------------
 * 
 *        LOOP
 *  
 ----------------------*/
void loop() {
  // put your main code here, to run repeatedly:
  //Serial.print("Loop");
  if (millis() > lTime + FPS)
  {
    /*----------------------------
     *  Pre-Logic
     ---------------------------*/
     if (pSpeed == 0)
        frame = 0;
     
     prevPX = pX;
     prevPY = pY;
     pSpeed = 0;
    /*----------------------------
     *  Logic
     ---------------------------*/

     /*
      *   Find current direction
      */
     if (display.pressed(UP_BUTTON))
     {
        pDir = 90;
        pSpeed = pASpeed;
     }
     else if (display.pressed(DOWN_BUTTON))
     {
        pDir = 270;
        pSpeed = pASpeed;
     }
     else if (display.pressed(LEFT_BUTTON))
     {
        pDir = 180;
        pSpeed = pASpeed;
     }
     else if (display.pressed(RIGHT_BUTTON))
     {
        pDir = 0;
        pSpeed = pASpeed;
     }

     /*
      *   Move in current direction
      */
     if (pSpeed > 0)
     switch( pDir )
     {
        case 90:
            pY -= pSpeed;
            break;
        case 270:
            pY += pSpeed;
            break;
        case 180:
            pX -= pSpeed;
            break;
        case 0:
            pX += pSpeed;
            break;
     }
     
     if (pY > 56 - boundH)
        pY = 56 - boundH;
     if (pY < boundY)
        pY = boundY;
     if (pX > 120)
        pX = 120;
     if (pX < 0)
        pX = 0;
        
     /*
      *   Animate if moving
      */
     if ( prevPX != pX || prevPY != pY )
        frame += animSpeed * pASpeed;
     if (frame > 4)
        frame = 0;

      /*
       *  Toggle for gray
       */
      //gray = (++gray) % 2; // not used

      /*------------------
       *  Move Baddies
       *  and
       *  Power Ups
       -----------------*/
       moveObjects();
       movePowerUp();
     /*----------------------------
     *  Drawing - Clear
     ---------------------------*/
    display.clearDisplay();
    lTime = millis();
    /*----------------------------
     *  Drawing - Draw
     ---------------------------*/
     drawHeader();
     
     drawPlayer();
     drawObjects();
     drawPowerUp();

    /*----------------------------
     *  Drawing - Display
     ---------------------------*/
    display.display();
  }
}
