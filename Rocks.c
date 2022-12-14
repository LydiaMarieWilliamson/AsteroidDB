#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h> // All SDL Applications need this
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "Timer.h"
#include "Log.h"
#include "Levels.h"

// Debugging
// #define SHOWOVERLAP    // Comment this out for normal play, uncomment to show collisions
#define TIMEGAMELOOP

// key definitions
#define ADDASTEROIDKEY SDLK_a
#define COUNTERCLOCKWISEKEY SDLK_q
#define BOMBKEY SDLK_b
#define CLOCKWISEKEY SDLK_w
#define DEBUGKEY SDLK_TAB
#define FIREKEY SDLK_SPACE
#define JUMPKEY SDLK_j
#define PAUSEKEY SDLK_PAUSE
#define QUITKEY SDLK_ESCAPE
#define SHIELDKEY SDLK_s
#define THRUSTKEY SDLK_LCTRL

// sizes
#define ALIENSHIPHEIGHT 64
#define ALIENSHIPWIDTH 64
#define SCREENWIDTH 1024
#define SCREENHEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64
#define EXPLOSIONSIZE 128
#define CELLSIZE 64
#define CHARWIDTH 13

// Define x and y restricted area
#define ZONESIZE 200
#define ZONELEFT (SCREENWIDTH-ZONESIZE)/2
#define ZONERIGHT ZONELEFT+ZONESIZE
#define ZONETOP (SCREENHEIGHT-ZONESIZE)/2
#define ZONEBOTTOM ZONETOP+ZONESIZE

// numbers of things
#define ALIENFLASHFRAMES 15
#define MAXALIENS 3
#define MAXBULLETSPERALIEN 8
#define MAXASTEROIDS 32
#define MAXPLAYERBULLETS 16
#define MAXEXPLOSIONS 256
#define NUMSCORES 10
#define NUMSOUNDS 4
#define NUMTEXTURES 15
#define NUMEXPLOSIONSOUNDS 2
#define NUMPOINTERS 273
#define CELLY SCREENHEIGHT/CELLSIZE
#define CELLX SCREENWIDTH/CELLSIZE
#define MAXTEXTSPRITES 10
#define MAXBULLETS MAXPLAYERBULLETS + (MAXBULLETSPERALIEN*MAXALIENS)

// Texture indices
#define TEXTUREASTEROID1 0 // 0, 1, 2, 3
#define PLBACKDROP 4
#define TEXTTEXTURE 5
#define TEXTUREPLAYERSHIP 6
#define TEXTUREDEBUG 7
#define TEXTUREBULLET 8
#define TEXTURESMALLSHIP 9
#define TEXTUREALIENSHIP 10
#define TEXTUREEXPLOSION 11 // 11, 12, 13, 14

// sound indices
#define THRUSTSOUND 0
#define CANNONSOUND 1
#define EXPLOSIONSOUND 2

// functions
#define sgn(x) (x < 0) ? -1 : (x > 0)

// typedefs
typedef struct firstpart *pfirstpart;
typedef pfirstpart ptrarray[100]; // 100 pointers to a firstpart
typedef unsigned char byte, *pbyte;

// enums
enum ObjectType { tAsteroid, tBullet, tPlayer, tAlien };
enum BulletOwner { bNone = -1, bPlayer, bAlien1, bAlien2, bAlien3 };

// Use this as a type- matches asteroid, bullet and playership
struct firstpart {
   SDL_Rect r;
   int type;
   bool active;
   int rotdir;
};

struct pair {
   int x, y;
};

// structs
struct asteroid {
   SDL_Rect r;
   int type; // 0
   bool active;
   int rotdir;
   float x, y;
   float xvel, yvel;
   int xtime, ytime;
   int xcount, ycount;
   bool rotclockwise;
   int rottime;
   int rotcount;
   int sizeindex;
#if 0 //(@) Not used.
   bool toDie;
#endif
};

struct bullet {
   SDL_Rect r;
   int type; // 1
   bool active;
   float x, y;
   int timer;
   float vx, vy;
   int ttl; // time to live
   int countdown;
   int playerbullet; // -1 = none,0= player, 1.. MAXALIENS = alien saucer
};

struct explosion {
   int x, y;
   int countdown;
   int frame;
   int dimension;
};

struct player {
   SDL_Rect r;
   int type; // 2
   bool active;
   int dir; // 0-23
   float x, y;
   float vx, vy;
   int lives;
} Player;

struct alien {
   SDL_Rect r;
   int type; // 3
   bool active;
   int moveDir;
   int moveTimer;
   float x, y;
   float xvel, yvel;
   int xcount, ycount;
   int xtime, ytime;
   int ttl;
   int flTimer;
   bool flShow;
};

struct Cell {
   ptrarray ptrs;
   int numobjects;
};

struct TextSprite {
   int x, y;
   int life;
   char message[130];
   int textFactor;
};

struct HighScoreEntry {
   char initials[4];
   int score;
   int d, m, y;
   int level;
};

// SDL variables
SDL_Window *screen = NULL;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect source, destination, dst, sourceRect, destRect;
SDL_Texture *textures[NUMTEXTURES];

// int variables
int keypressed;
int rectCount = 0;
int frameCount, tickCount, lastTick; bool showFPS;
int score; bool paused; int gameLevel; bool CloseFlag, playerDead; int shieldRadius;
bool debugFlag, fireFlag, flashFlag, shieldFlag; int shieldStrength, shieldTimer;
int numsprites, playerBulletCount, alienBulletCount;
int rotTimer, speedTimer, fireTimer, jumpTimer, pauseTimer, flashTimer, controlTimer;
bool thrustFlag, jumpFlag, pauseFlag;
int numcells, coolDown, numHighScores;
int rotateFlag; // 0 do nowt, 1 counter clockwise, 2 clockwise
int numAsteroids = 0;
float speedFactor = 1;
char *maskErrorFile;

// array variables
char timebuff[50]; // used for timing
char buffer[100], buffer2[100];
float thrustx[24];
float thrusty[24];
byte bulletmask[3*3];
byte plmask[24*64*64];
byte a1mask[24*280*280];
byte a2mask[24*140*140];
byte a3mask[24*70*70];
byte a4mask[24*35*35];
byte alienmask[1*64*64];

// struct variables
struct Cell cells[CELLX][CELLY];
struct pair cellList[CELLX*CELLY];
struct TextSprite sprites[MAXTEXTSPRITES];
struct alien aliens[MAXALIENS];
struct asteroid asteroids[MAXASTEROIDS];
struct bullet bullets[MAXBULLETS];
struct explosion explosions[MAXEXPLOSIONS];
struct Mix_Chunk *sounds[NUMSOUNDS];
struct HighScoreEntry highscores[NUMSCORES];

TimeT Ticks;

// consts
const char *texturenames[NUMTEXTURES] = {
   "Images/Rock0.png", "Images/Rock1.png", "Images/Rock2.png", "Images/Rock3.png",
   "Images/Space.png", "Images/Text.png", "Images/Ship.png", "Images/Debug.png",
   "Images/Pea.png", "Images/Pod.png", "Images/Alien.png",
   "Images/Boom0.png", "Images/Boom1.png", "Images/Boom2.png", "Images/Boom3.png"
};
const char *highscoreFN = "HiScores.txt";
const char *soundnames[NUMSOUNDS] = { "Sounds/Push.wav", "Sounds/Fire.wav", "Sounds/Boom0.wav", "Sounds/Boom1.wav" };

const int explosionSizes[4] = { 128, 128, 128, 64 };
const int sizes[4] = { 280, 140, 70, 35 };
const int debugOffs[4] = { 0, 280, 420, 490 };
const int bulletx[24] = { 32, 38, 50, 64, 66, 67, 68, 67, 66, 64, 50, 38, 32, 26, 16, 0, -2, -3, -4, -3, -2, 0, 16, 24 };
const int bullety[24] = { -4, -3, -2, 0, 16, 26, 32, 38, 50, 64, 66, 67, 71, 70, 69, 64, 50, 38, 32, 26, 16, 0, -2, -3 };
const int xdir[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const int ydir[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

// Sets Window caption according to state - eg in debug mode or showing fps
void SetCaption(char *msg) {
   if (showFPS) {
      snprintf(buffer, sizeof(buffer), "Fps = %d #Asteroids =%d %s", frameCount, numAsteroids, msg);
      SDL_SetWindowTitle(screen, buffer);
   } else {
      SDL_SetWindowTitle(screen, msg);
   }
}

SDL_Texture *LoadTexture(const char *afile, SDL_Renderer *ren) {
   SDL_Texture *texture = IMG_LoadTexture(ren, afile);
   if (texture == NULL) {
      LogError2("Failed to load texture from ", afile);
   }
   return texture;
}

// Loads Textures by calling LoadTexture
void LoadTextures(void) {
   for (int i = 0; i < NUMTEXTURES; i++) {
      textures[i] = LoadTexture(texturenames[i], renderer);
   }
}

// Loads all sound files
void LoadSoundFiles(void) {
   for (int i = 0; i < NUMSOUNDS; i++) {
      sounds[i] = Mix_LoadWAV(soundnames[i]);
   }
}

// Init thrustx and thrusty[]
void InitThrustAngles(void) {
   const float pi = 3.14159265f;
   const float degreeToRad = pi/180.0f;
   float degree = 0.0f;
   for (int i = 0; i < 24; i++) {
      float radianValue = degree*degreeToRad;
      thrustx[i] = (float)sin(radianValue);
      thrusty[i] = (float)-cos(radianValue);
      degree += 15;
   }
}

void processTexture(int texturenum, int size, char *filename) {
   SDL_Texture *targettexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,/* SDL_TEXTUREACCESS_STREAMING, */SDL_TEXTUREACCESS_TARGET, size, size);
   SDL_Rect src; src.w = size, src.h = size, src.y = 0;
   for (int i = 0; i < 24; i++) {
      src.x = i*size;
      SDL_SetRenderTarget(renderer, targettexture);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, textures[texturenum], &src, NULL);
      int access, aw, ah; (void)SDL_QueryTexture(targettexture, NULL, &access, &aw, &ah);
      void *pixels; int pitch; (void)SDL_LockTexture(targettexture, NULL, &pixels, &pitch);
      Uint32 *upixels = (Uint32 *)pixels;
      for (int y = 0; y < size; y++) for (int x = 0; x < aw*ah; x++) {
         Uint32 p = *upixels;
      }
      SDL_SetRenderTarget(renderer, NULL);
      SDL_RenderCopy(renderer, targettexture, NULL, NULL);
      SDL_DestroyTexture(targettexture);
      SDL_RenderPresent(renderer);
      SDL_Delay(500);
   }
}

// Load a mask file containing number images of size*size
bool LoadMask(char *filename, int size, int number, char *mask) {
   maskErrorFile = filename;
   int sizeofmask = size*size*number;
   FILE *fmask = fopen(filename, "rb");
   if (fmask == NULL) return false;
   int numread = fread(mask, 1, sizeofmask, fmask);
   fclose(fmask);
   return numread == sizeofmask;
}

// Load all masks if any fail then o exit
bool LoadMasks(void) {
   maskErrorFile = NULL;
   if (!LoadMask("Masks/Pea.msk", 3, 1, bulletmask)) return false; // playership
   if (!LoadMask("Masks/Ship.msk", 64, 24, plmask)) return false; // playership
   if (!LoadMask("Masks/Rock0.msk", 280, 24, a1mask)) return false;
   if (!LoadMask("Masks/Rock1.msk", 140, 24, a2mask)) return false;
   if (!LoadMask("Masks/Rock2.msk", 70, 24, a3mask)) return false;
   if (!LoadMask("Masks/Alien.msk", 64, 1, alienmask)) return false;
   if (!LoadMask("Masks/Rock3.msk", 35, 24, a4mask)) return false;
   return true;
}

void ClearCellList(void) {
   memset(cellList, 0, sizeof(cellList));
   numcells = 0;
   memset(cells, 0, sizeof(cells));
   l("ClearCellList");
}

void AddPointerToCell(int x, int y, pfirstpart objectptr) {
   if (x < 0 || x >= CELLX || y < 0 || y > CELLY) {
      x = x;
   }
   int numobjects = cells[x][y].numobjects;
   if (numobjects == 0) { // 1st time cell added to so add to cellList
      cellList[numcells].x = x;
      cellList[numcells++].y = y;
   }
   cells[x][y].ptrs[numobjects] = objectptr;
   cells[x][y].numobjects++;
   ln("AddPointerToCell", "", numobjects);
}

// Adds an object to every cell it covers. For a large asteroid 280 x 280
// This can occupy between 25, 30 or 36 cells.
void AddObjectToCells(pfirstpart objectptr) {
   int x = objectptr->r.x;
   int y = objectptr->r.y;
   int h = objectptr->r.h;
   int w = objectptr->r.w;
   int cellx = x/CELLSIZE;
   int celly = y/CELLSIZE;
   int endcellx = (x + w - 1)/CELLSIZE;
   int endcelly = (y + h - 1)/CELLSIZE;
// Range check cellx, endcellx,celly and endcelly
   if (endcellx >= CELLX) endcellx = CELLX - 1;
   if (endcelly >= CELLY) endcelly = CELLY - 1;
   if (cellx < 0) cellx = 0;
   if (celly < 0) celly = 0;
   for (int ix = cellx; ix <= endcellx; ix++)
      for (int iy = celly; iy <= endcelly; iy++)
         AddPointerToCell(ix, iy, objectptr);
}

// returns index of frst free sprite, otherwise if non are free return -1
int findFreeSprite(void) {
   for (int i = 0; i < MAXTEXTSPRITES; i++) {
      if (sprites[i].life == 0)
         return i;
   }
   return -1;
}

// This creates a Text sprite at the x,y coordinates
void AddTextSpriteAt(char *value, int x, int y, int textfactor) {
   int spriteindex = findFreeSprite();
   if (spriteindex == -1 || y < 20) return;
   struct TextSprite *psprite = &sprites[spriteindex];
   psprite->x = x;
   psprite->y = y;
   psprite->life = 60;
   psprite->textFactor = textfactor;
   strncpy(psprite->message, value, sizeof(psprite->message));
   numsprites++;
}

// This centres the text horizontally
void AddTextSpriteString(char *value, int y, int textfactor) {
   int textlen = strlen(value)*CHARWIDTH*textfactor; // 13 = width of chars in set
   int x = (SCREENWIDTH - textlen)/2;
   AddTextSpriteAt(value, x, y, textfactor);
}

// This centres it but also converts int to str
void AddTextSpriteInt(int value, int y, int textfactor) {
   AddTextSpriteString(sltoa(value), y, textfactor);
}

// initialises data but possibly gets overwritten when HiScores.txt file is read in in ReadhighScores()
void InitHighScores(void) {
   memset(highscores, 0, sizeof(highscores));
   strncpy(highscores[0].initials, "DHB", 4);
   highscores[0].score = 500;
   highscores[0].d = 1;
   highscores[0].m = 1;
   highscores[0].y = 2018;
   highscores[0].level = 1;
   numHighScores = 1;
}

// copies len chars from line starting from start then return numeric value of copied chars
int StrConv(char *line, int start, int len) {
   char buffer[20];
   if (len > sizeof(buffer))
      len = sizeof(buffer) - 1; // error check length
   for (int i = 0; i < len; i++) {
      buffer[i] = line[start++];
   }
   buffer[start] = '\0';
   return atoi(buffer);
}

// copies len chars from line into starting from start
// There's no terminating '\0' so can't use strncpy
void StrCopyTo(char *dest, char *line, int start, int len) {
   for (int i = 0; i < len; i++) {
      dest[i] = line[start++];
   }
   dest[start] = '\0';
}

// Reads highscores from highScoreFile. Each row has this
// ddmmyyyyIIILLSSSSSS - 19 chars long
// where dd= day 01-31 mm = month 01-12 yy = year e.g. 2018 III = Initials e.g. DH, Level LLB and SSSSSS = 6 digit score with leading zeroes
// example 01012012DHB10000500 - 1st Jan 2018 DHB beat level 10 with 500 score
void ReadHighScores(void) {
   InitHighScores();
   FILE *fscores = fopen(highscoreFN, "rt");
   if (fscores == NULL) return; // error reading
   numHighScores = 0;
   char line[25]; // lonhg enough for 19 char string plus trailing '\0'
   while (fgets(line, sizeof(line), fscores) != NULL) {
      int len = strlen(line);
      if (len == 20) {
         highscores[numHighScores].d = StrConv(line, 0, 2);
         highscores[numHighScores].m = StrConv(line, 2, 2);
         highscores[numHighScores].y = StrConv(line, 4, 4);
         StrCopyTo((char *)&highscores[numHighScores].initials, line, 8, 3);
         highscores[numHighScores].level = StrConv(line, 11, 2);
         highscores[numHighScores++].score = StrConv(line, 13, 6);
         if (numHighScores == NUMSCORES) break;
      } else break;
   }
   fclose(fscores);
}

// Write high scores to a file but only those with a score
// Ecah row is like this  ddmmyyyyIIISSSSSS - 17 chars long
void WriteHighScores(void) {
   FILE *fscores = fopen(highscoreFN, "wt");
   if (fscores == NULL) return;
   numHighScores = 0;
   for (int i = 0; i < NUMSCORES; i++) {
      if (highscores[i].score > 0) {
         numHighScores++;
         char line[20];
         snprintf(line, sizeof(line), "%02d%02d%04d", highscores[i].d, highscores[i].m, highscores[i].y);
         StrCopyTo((char *)&line[8], highscores[i].initials, 0, 3);
         snprintf(&line[11], sizeof(line), "%02d", highscores[i].level);
         snprintf(&line[13], sizeof(line), "%06d", highscores[i].score);
         fputs(line, fscores);
         fputs("\n", fscores);
      }
   }
   fclose(fscores);
}

// Initialize all setup, set screen mode, load images etc
void InitSetup(void) {
   errorCount = 0;
   CloseFlag = false;
   flashFlag = false;
   flashTimer = 0;
   srand((int)time(NULL));
   SDL_Init(SDL_INIT_EVERYTHING);
   screen = SDL_CreateWindow("Asteroids", 100, 100, SCREENWIDTH, SCREENHEIGHT, 0);
   if (screen == NULL) {
      LogError("InitSetup failed to create window");
   }
   renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
   LoadTextures();
   int success = Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 8192);
   if (success == -1) {
      LogError("InitSetup failed to init audio");
   }
   LoadSoundFiles();
   if (!LoadMasks()) {
      LogError2("InitSetup failed to load mask number:", maskErrorFile);
   }
   ReadHighScores();
}

// Free memory allocated for .wav files
void DestroySounds(void) {
   for (int i = NUMSOUNDS - 1; i >= 0; i--) {
      Mix_FreeChunk(sounds[i]);
   }
}

// release memory for textures
void DestroyTextures(void) {
   for (int i = NUMTEXTURES - 1; i >= 0; i--) {
      SDL_DestroyTexture(textures[i]);
   }
}

// Cleans up after game over
void FinishOff(void) {
   DestroySounds();
   DestroyTextures();
   Mix_CloseAudio();
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(screen);
   SDL_Quit();
   exit(EXIT_SUCCESS);
}

// Plays a sound from sounds[]
void PlayASound(int soundindex) {
   int success = Mix_PlayChannel(-1, sounds[soundindex], 0);
   if (success == -1) {
      char buffer[10];
      LogError2("Mix_PlayChannel failed to play audio sound #", SDL_ltoa(soundindex, buffer, 10));
   }
}

void RenderTexture(SDL_Texture *tex, int x, int y) {
// Setup the destination rectangle to be at the position we want
   dst.x = x;
   dst.y = y;
// Query the texture to get its width and height to use
   SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
   SDL_RenderCopy(renderer, tex, NULL, &dst);
}

// print single char c at rect target
void printch(char c, SDL_Rect *target, float textFactor) {
   int start = (c - '!');
   if (c != ' ') {
      sourceRect.h = 23;
      sourceRect.w = 12;
      sourceRect.x = start*12;
      sourceRect.y = 0;
      SDL_RenderCopy(renderer, textures[TEXTTEXTURE], &sourceRect, target);
   }
   (*target).x += (int)(CHARWIDTH*textFactor); // in this bitmap font, chars are CHARWIDTH pixels width
}

// print string text at x,y pixel coords
void TextAt(int atX, int atY, char *msg, float textFactor) {
   destRect.h = (int)(23*textFactor);
   destRect.w = (int)(12*textFactor);
   destRect.x = atX;
   destRect.y = atY;
   for (int i = 0; i < (int)strlen(msg); i++) {
      printch(msg[i], &destRect, textFactor);
   }
}

void UpdateCaption(void) {
#ifdef TIMEGAMELOOP
   SetCaption(timebuff);
#else
   snprintf(buffer2, sizeof(buffer2), "%10.6f", EndTimer(Ticks));
   tickCount = SDL_GetTicks();
   if (tickCount - lastTick >= 1000) {
      lastTick = tickCount;
      if (showFPS) {
         SetCaption(buffer2);
         frameCount = 0;
      }
   } else if (!showFPS) {
      SetCaption(buffer2);
   }
#endif
}

// from https://stackoverflow.com/questions/38334081/howto-draw-circles-arcs-and-vector-graphics-in-sdl
void DrawCircle(SDL_Renderer *Renderer, int _x, int _y, int radius) {
   int x = radius - 1;
   int y = 0;
   int tx = 1;
   int ty = 1;
   int err = tx - (radius << 1); // shifting bits left by 1 effectively doubles the value. == tx - diameter
   while (x >= y) {
   // Each of the following renders an octant of the circle
      SDL_RenderDrawPoint(Renderer, _x + x, _y - y);
      SDL_RenderDrawPoint(Renderer, _x + x, _y + y);
      SDL_RenderDrawPoint(Renderer, _x - x, _y - y);
      SDL_RenderDrawPoint(Renderer, _x - x, _y + y);
      SDL_RenderDrawPoint(Renderer, _x + y, _y - x);
      SDL_RenderDrawPoint(Renderer, _x + y, _y + x);
      SDL_RenderDrawPoint(Renderer, _x - y, _y - x);
      SDL_RenderDrawPoint(Renderer, _x - y, _y + x);
      if (err <= 0) {
         y++;
         err += ty;
         ty += 2;
      } else {
         x--;
         tx += 2;
         err += tx - (radius << 1);
      }
   }
}

// Draws throbbing circle round ship and shield strength underneath.
void DisplayShield(SDL_Rect *target) {
   if (shieldFlag && shieldStrength > 10) {
      SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
      DrawCircle(renderer, target->x + (SHIPWIDTH/2), target->y + (SHIPHEIGHT/2), shieldRadius);
      shieldRadius += 2;
      if (shieldRadius == 46) {
         shieldRadius = 38;
      }
   }
   if (shieldStrength < 100) {
      TextAt(target->x + 10, target->y + 58, sltoa(shieldStrength), 0.67f);
   }
}

// Draw player ship
void DrawPlayerShip(void) {
   if (!Player.active) return;
   SDL_Rect target;
   target.h = SHIPHEIGHT;
   target.w = SHIPWIDTH;
   target.x = (int)Player.x;
   target.y = (int)Player.y;
   SDL_Rect spriterect;
   spriterect.h = SHIPHEIGHT;
   spriterect.w = SHIPWIDTH;
   spriterect.x = Player.dir*SHIPWIDTH;
   spriterect.y = 0;
   SDL_RenderCopy(renderer, textures[TEXTUREPLAYERSHIP], &spriterect, &target);
   DisplayShield(&target);
   if (SDL_GetTicks() - jumpTimer < 3000) {
      if (flashFlag)
         TextAt((int)Player.x - 50, (int)Player.y + 66, "No jump!", 1);
   }
   if (!debugFlag) return;
// code after here is only run if debugFlag
   target.w = 64;
   target.h = 64;
   spriterect.x = 280;
   spriterect.y = 140;
   spriterect.w = 66;
   spriterect.h = 66;
   SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &spriterect, &target);
   char buff[30];
   snprintf(buff, sizeof(buff), "(%6.4f,%6.4f) Dir= %i", Player.x, Player.y, Player.dir);
   TextAt((int)Player.x - 50, (int)Player.y + 66, buff, 1);
   snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", Player.vx, Player.vy);
   TextAt((int)Player.x - 50, (int)Player.y + 90, buff, 1);
}

// Draw player ship
void DrawAlienShips(void) {
   SDL_Rect target;
   target.h = ALIENSHIPHEIGHT;
   target.w = ALIENSHIPWIDTH;
   SDL_Rect spriterect;
   spriterect.h = ALIENSHIPHEIGHT;
   spriterect.w = ALIENSHIPWIDTH;
   spriterect.x = 0;
   spriterect.y = 0;
   for (int i = 0; i < MAXALIENS; i++) {
      if (!aliens[i].active) continue;
      struct alien *palien = &aliens[i];
      target.x = (int)palien->x;
      target.y = (int)palien->y;
      if (palien->flShow) {
         SDL_RenderCopy(renderer, textures[TEXTUREALIENSHIP], &spriterect, &target);
      }
      if (!debugFlag) continue;
   // code after here is only run if debugFlag
      target.w = ALIENSHIPWIDTH;
      target.h = ALIENSHIPHEIGHT;
      spriterect.x = 280;
      spriterect.y = 140;
      spriterect.w = ALIENSHIPWIDTH;
      spriterect.h = ALIENSHIPHEIGHT;
      SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &spriterect, &target);
      char buff[30];
      snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", palien->x, palien->y);
      TextAt((int)palien->x - 50, (int)palien->y + 66, buff, 1);
      snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", palien->xvel, palien->yvel);
      TextAt((int)palien->x - 50, (int)palien->y + 90, buff, 1);
   }
}

// Draw All asteroids
void DrawAsteroids(void) {
   numAsteroids = 0;
   SDL_Rect asteroidRect;
   asteroidRect.y = 0;
   for (int i = 0; i < MAXASTEROIDS; i++) {
      if (asteroids[i].active) {
         numAsteroids++; // keep track of how many onscreen
         int sizeIndex = asteroids[i].sizeindex; // 0-3
         int asize = sizes[sizeIndex]; // asteroid size 280,140, 70,35
         SDL_Rect target;
         target.h = asize;
         target.w = asize;
         asteroidRect.h = asize;
         asteroidRect.w = asize;
         asteroidRect.x = asize*asteroids[i].rotdir;
         target.x = (int)asteroids[i].x;
         target.y = (int)asteroids[i].y;
         SDL_RenderCopy(renderer, textures[TEXTUREASTEROID1 + sizeIndex], &asteroidRect, &target);
         if (!debugFlag)
            continue;
         asteroidRect.x = debugOffs[sizeIndex];
         SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &asteroidRect, &target);
         char buff[30];
         TextAt(target.x + 25, target.y + 25, SDL_ltoa(i, buff, 10), 1);
         TextAt(target.x + 25, target.y + 55, SDL_ltoa(sizeIndex, buff, 10), 1);
      }
   }
}

// Draw all bullets with countdown > 0
void DrawBullets(void) {
   SDL_Rect target;
   target.h = 3;
   target.w = 3;
   SDL_Rect spriterect;
   spriterect.h = 3;
   spriterect.w = 3;
   spriterect.x = 0;
   spriterect.y = 0;
   for (int i = 0; i < MAXBULLETS; i++) {
      if (bullets[i].active && bullets[i].ttl > 0) {
         target.x = (int)bullets[i].x;
         target.y = (int)bullets[i].y;
         SDL_RenderCopy(renderer, textures[TEXTUREBULLET], &spriterect, &target);
         if (debugFlag) {
            char buff[10];
            snprintf(buff, 10, "%i", bullets[i].ttl);
            TextAt((int)bullets[i].x + 5, (int)bullets[i].y, buff, 1);
         }
      }
   }
}

// Draw all explosions with countdown > 0
void DrawExplosions(void) {
   SDL_Rect spriterect;
   spriterect.y = 0;
   for (int i = 0; i < MAXEXPLOSIONS; i++) {
      if (explosions[i].frame != -1) {
         int dimension = explosions[i].dimension; // 0..3
         SDL_Rect target;
         target.x = explosions[i].x - EXPLOSIONSIZE/2; // adjustment so explosion matches asteroid location
         target.y = explosions[i].y - EXPLOSIONSIZE/2;
         target.h = EXPLOSIONSIZE;
         target.w = EXPLOSIONSIZE;
         spriterect.h = EXPLOSIONSIZE;
         spriterect.w = EXPLOSIONSIZE;
         int frame = explosions[i].frame;
         spriterect.y = (frame/8)*EXPLOSIONSIZE;
         spriterect.x = (frame%8)*EXPLOSIONSIZE;
         SDL_RenderCopy(renderer, textures[TEXTUREEXPLOSION + dimension], &spriterect, &target);
         if (debugFlag) {
            char buff[15];
            snprintf(buff, sizeof(buff), "X %i", explosions[i].frame);
            TextAt(target.x + 10, target.y + EXPLOSIONSIZE, buff, 1);
         }
      }
   }
}

// Show score and lives on screen
void DrawScoreAndLives(void) {
   char scorebuffer[10];
   snprintf(scorebuffer, sizeof(scorebuffer), "%i", score);
   TextAt(25, SCREENHEIGHT - 30, scorebuffer, 1);
   SDL_Rect destr;
   destr.h = 33;
   destr.w = 24;
   destr.y = SCREENHEIGHT - 70;
   for (int i = 0; i < Player.lives; i++) {
      destr.x = (i*30) + 50;
      SDL_RenderCopy(renderer, textures[TEXTURESMALLSHIP], NULL, &destr);
   }
}

// Draw Paused if paused flashing at twice per second
void DrawPauseMessage(char *message) {
   if (!paused) return;
   if (frameCount < 16 || frameCount > 45) {
      TextAt((SCREENWIDTH/2) - 20, (SCREENHEIGHT/2) - 40, message, 2);
   }
}

void DrawTextSprites(void) {
   for (int i = 0; i < MAXTEXTSPRITES; i++) {
      struct TextSprite *psprite = &sprites[i];
      if (psprite->life > 0) {
         TextAt(psprite->x, psprite->y--, psprite->message, (float)psprite->textFactor);
         psprite->life--;
         if (psprite->y == 0) {
            psprite->life = 0;
         }
      }
   }
}

// Copies Asteroids, playership, bullets etc to offscreen buffer
void DrawEverything(void) {
   RenderTexture(textures[PLBACKDROP], 0, 0);
   DrawAsteroids();
   DrawPlayerShip();
   DrawAlienShips();
   DrawBullets();
   DrawExplosions();
   DrawScoreAndLives();
   DrawTextSprites();
   DrawPauseMessage("Paused");
   UpdateCaption();
}

// Copies Asteroids, playership, bullets etc to offscreen buffer
void RenderEverything(void) {
   SDL_RenderPresent(renderer); // make offscreen buffer vsisble
   frameCount++;
   if (frameCount == 60) frameCount = 0;
}

void DrawLevelStart(void) {
   char buffer[30];
   snprintf(buffer, sizeof(buffer), "Level %d", gameLevel + 1);
   AddTextSpriteString(buffer, 380, 3);
   switch (Player.lives) {
      case 1:
         snprintf(buffer, sizeof(buffer), "Last Life!");
      break;
      case 2:
      case 3:
         snprintf(buffer, sizeof(buffer), "Lives left: %d", Player.lives);
      break;
   }
   AddTextSpriteString(buffer, 440, 3);
}

// Initialize Player struct
void InitPlayer(int numlives) {
   Player.type = tPlayer;
   Player.dir = 0;
   Player.vy = 0;
   Player.vy = 0;
   Player.lives = numlives;
   Player.x = 500;
   Player.y = 360;
   Player.r.h = 64;
   Player.r.w = 64;
   Player.vx = 0.0f;
   Player.vy = 0.0f;
   Player.active = true;
}

// Initialize all asteroid variables
void InitAsteroids(void) {
   for (int i = 0; i < MAXASTEROIDS; i++) {
      asteroids[i].active = false;
   }
}

// Init all MAXBULLETS
void InitBullets(void) {
   for (int i = 0; i < MAXBULLETS; i++) {
      struct bullet *pbullet = &bullets[i];
      pbullet->active = false;
      pbullet->x = 0;
      pbullet->y = 0;
      pbullet->timer = 0;
      pbullet->ttl = 0;
      pbullet->countdown = 0;
      pbullet->r.h = 3;
      pbullet->r.w = 3;
      pbullet->type = tBullet;
      pbullet->playerbullet = -1; // set when fired -1 = player=0, 1..3 is alien ship
   }
}

// Init all Explosions
void InitExplosions(void) {
   for (int i = 0; i < MAXEXPLOSIONS; i++) {
      explosions[i].x = 0;
      explosions[i].y = 0;
      explosions[i].countdown = 0;
      explosions[i].frame = -1;
   }
}

void InitTextSprites(void) {
   memset(sprites, 0, sizeof(sprites));
   numsprites = 0;
}

// Called to initialise each game
void InitGame(void) {
   score = 0;
   paused = false;
   gameLevel = 0;
   InitPlayer(3); // pass in number lives
// All other Initialisation done in ReInitGame called from InitLevel
}

// Alter Player.dir according to rotation key pressed
void RotatePlayerShip(void) {
   if (rotateFlag && (SDL_GetTicks() - rotTimer > 40)) {
      rotTimer = SDL_GetTicks();
      if (rotateFlag == 1) { // CounterClockwise
         Player.dir += 23;
         Player.dir %= 24;
      } else if (rotateFlag == 2) { // Clockwise
         Player.dir++;
         Player.dir %= 24;
      }
   }
}

// move the player ship
void MovePlayerShip(void) {
   if (!Player.active) return;
   Player.x += Player.vx;
   Player.y += Player.vy;
   if (Player.x <= -5)
      Player.x += SCREENWIDTH;
   else if (Player.x > SCREENWIDTH - 4)
      Player.x = 0;
   if (Player.y <= -5)
      Player.y += SCREENHEIGHT;
   else if (Player.y > SCREENHEIGHT - 4)
      Player.y = 0;
   Player.r.y = (int)Player.y;
   Player.r.x = (int)Player.x;
   l("Adding pointer for player");
   AddObjectToCells((pfirstpart)&Player);
}

// applies thrust
void ApplyThrust(void) {
   if (thrustFlag && (SDL_GetTicks() - speedTimer > 40)) {
      speedTimer = SDL_GetTicks();
      Player.vx += (thrustx[Player.dir]/3.0f);
      Player.vy += (thrusty[Player.dir]/3.0f);
      PlayASound(THRUSTSOUND);
   }
}

// Move an Asteroid
void MoveAsteroid(int index) {
   if (!asteroids[index].active)
      return;
   struct asteroid *past = &asteroids[index];
   int size = sizes[past->sizeindex]; // used later so fetch once!
// do the rotation
   if (past->rottime > 0) {
      past->rottime--;
      if (past->rottime == 0) {
         past->rottime = past->rotcount;
         if (past->rotclockwise) {
            past->rotdir = (past->rotdir + 1)%24;
         } else {
            past->rotdir = ((past->rotdir - 1) + 24)%24;
         }
      }
   }
// movement along x axis
   if (past->xtime > 0) {
      past->xtime--;
      if (past->xtime <= 0) {
         past->x += past->xvel;
         past->r.x = (int)past->x;
         past->xtime = past->xcount;
         if (past->x < -size) { // off left edge, on right
            past->x += SCREENWIDTH;
         } else if (past->x > SCREENWIDTH) { // off right edge, on left
            past->x -= SCREENWIDTH;
         }
      }
   }
// movement along y axis
   if (past->ytime > 0) {
      past->ytime--;
      if (past->ytime == 0) {
         past->y += past->yvel;
         past->r.y = (int)past->y;
         past->ytime = past->ycount;
         if (past->y < -size) {
            past->y += SCREENHEIGHT;
         } else if (past->y > SCREENHEIGHT) {
            past->y -= SCREENHEIGHT;
         }
      }
   }
// a check in case the asteroid is now inactive
   if (past->active) {
      l2("Adding pointer for asteroids", sltoa(index));
      AddObjectToCells((pfirstpart) past);
   }
}

// Move All Asteroids
void MoveAsteroids(void) {
   for (int i = 0; i < MAXASTEROIDS; i++) {
      if (asteroids[i].active) {
         MoveAsteroid(i);
      }
   }
}

// move all alien ships
void MoveAlienShips(void) {
   for (int i = 0; i < MAXALIENS; i++) {
      struct alien *palien = &aliens[i];
      if (!palien->active) continue;
   // movement along x axis
      if (palien->xtime > 0) {
         palien->xtime--;
         if (palien->xtime <= 0) {
            palien->x += palien->xvel;
            palien->r.x = (int)palien->x;
            palien->xtime = palien->xcount;
            if (palien->x < -ALIENSHIPWIDTH) { // off left edge, on right
               palien->x += SCREENWIDTH;
            } else if (palien->x > SCREENWIDTH) { // off right edge, on left
               palien->x -= SCREENWIDTH;
            }
         }
      // movement along y axis
         if (palien->ytime > 0) {
            palien->ytime--;
            if (palien->ytime == 0) {
               palien->y += palien->yvel;
               palien->r.y = (int)palien->y;
               palien->ytime = palien->ycount;
               if (palien->y < -ALIENSHIPHEIGHT) {
                  palien->y += SCREENHEIGHT;
               } else if (palien->y > SCREENHEIGHT) {
                  palien->y -= SCREENHEIGHT;
               }
            }
         }
      // a check in case the asteroid is now inactive
         if (palien->active) {
#if 0
            l2("Adding pointer for Aliensd", sltoa(i)); // w
#endif
            AddObjectToCells((pfirstpart) palien);
         }
      }
   }
}

// return index or -1 if not found
int FindFreeAsteroidSlot(void) {
   for (int i = 0; i < MAXASTEROIDS; i++) {
      if (!asteroids[i].active) {
         return i;
      }
   }
   return -1;
}

// Checks target x,y coords have nothing in cells below
bool IsEmptySpace(int x, int y) {
   if (x < 0 || y < 0 || x > SCREENWIDTH || y > SCREENHEIGHT) {
      return false;
   }
   int celly = y/CELLSIZE;
   int cellx = x/CELLSIZE;
   if (cells[cellx][celly].numobjects == 0 && cells[cellx + 1][celly].numobjects == 0 && cells[cellx][celly + 1].numobjects == 0 && cells[cellx + 1][celly + 1].numobjects == 0) return true;
   return false;
}

// returns true if x,y is in central restricted zone or if a non-zero radius is
bool InCentre(int x, int y, int radius) {
   if (radius == 0) {
      if ((x < ZONELEFT || x > ZONERIGHT) && (y < ZONETOP || y > ZONEBOTTOM)) return false;
   } else {
      x -= (SCREENWIDTH/2); // calculate distance from centre
      y -= (SCREENHEIGHT/2);
      int distance = x*x + y*y;
      if (radius*radius < distance) return false; // it's outside the distance
   }
   return true;
}

// adds alien ship to screen
void AddAlienShip(void) {
   int alienindex = -1;
   for (int index = 0; index < MAXALIENS; index++) {
      if (!aliens[index].active) {
         alienindex = index;
         break;
      }
   }
   if (alienindex == -1) return;
   struct alien *palien = &aliens[alienindex];
   palien->active = true;
   do {
      palien->x = (float)Random(SCREENWIDTH - ALIENSHIPWIDTH) + 2*ALIENSHIPWIDTH;
      palien->y = (float)Random(SCREENHEIGHT - ALIENSHIPHEIGHT) + 2*ALIENSHIPHEIGHT;
   // Prevent asteroids appearing in a 200 x 200 square in the centre. Otherwise player ship can get blown up immediately!
      if (InCentre((int)palien->x, (int)palien->y, 200)) continue;
      if (IsEmptySpace((int)palien->x, (int)palien->y)) break;
   } while (true);
   palien->type = tAlien;
   palien->r.y = (int)palien->y;
   palien->r.x = (int)palien->x;
   palien->moveDir = 0;
   palien->xvel = (float)3 - Random(6);
   palien->yvel = (float)3 - Random(6);
   palien->r.w = ALIENSHIPWIDTH;
   palien->r.h = ALIENSHIPHEIGHT;
   palien->xtime = 2 + Random(3);
   palien->xcount = palien->xtime;
   palien->ytime = 2 + Random(3);
   palien->ycount = palien->ytime;
   palien->moveTimer = 10 + Random(5);
   palien->flShow = true;
   palien->flTimer = Random(ALIENFLASHFRAMES) + 1;
   palien->ttl = Random(30) + 10;
}

// adds to table , size = 0..3
void AddAsteroid(int sizeindex) {
   int index = FindFreeAsteroidSlot();
   if (index == -1) // table full so quietly exit
      return;
   if (sizeindex == -1) { // Use -1 to sepcify a random size
      sizeindex = Random(4) - 1;
   }
   int asize = sizes[sizeindex];
   struct asteroid *past = &asteroids[index];
   past->active = true;
   do {
      past->x = (float)Random(SCREENWIDTH - asize) + asize*2;
      past->y = (float)Random(SCREENHEIGHT - asize) + asize*2;
   // Prevent asteroids appearing in a 150 x 150 square in the centre. Otherwise player ship can get blown up immediately!
      if (InCentre((int)past->x, (int)past->y, 0)) continue;
      if (IsEmptySpace((int)past->x, (int)past->y)) break;
   } while (true);
   past->type = tAsteroid;
   past->r.y = (int)past->y;
   past->r.x = (int)past->x;
   past->rotdir = Random(24);
   past->rotclockwise = Random(2) == 2;
   past->xvel = (float)4 - Random(8);
   past->yvel = (float)4 - Random(8);
   past->xtime = 2 + Random(5);
   past->xcount = past->xtime;
   past->ytime = 2 + Random(5);
   past->ycount = past->ytime;
   past->rottime = 2 + Random(8);
   past->rotcount = past->rottime;
   past->sizeindex = sizeindex;
   past->r.w = sizes[sizeindex];
   past->r.h = sizes[sizeindex];
}

// add Explosion to table
void AddExplosion(int x, int y) {
   int index = -1;
   for (int i = 0; i < MAXEXPLOSIONS; i++) {
      if (explosions[i].frame == -1) {
         index = i;
         break;
      }
   }
   if (index == -1) return;
   PlayASound(EXPLOSIONSOUND + Random(NUMEXPLOSIONSOUNDS - 1));
   explosions[index].frame = 0;
   explosions[index].x = x;
   explosions[index].y = y;
   explosions[index].countdown = 1;
   explosions[index].dimension = Random(4) - 1; // 0..3
}

// DestroyAsteroid 2 from pointer, create 4 smaller plus 4 smallest (size 3 )
void DestroyThisAsteroid(struct asteroid *a) {
   a->active = false;
   int sizeindex = a->sizeindex;
// If it's the smallest size just destroy it, and exit otherwise add in 4 smaller and 4 smallest type asteroids
   if (sizeindex == 3)
      return;
   float xvel = a->xvel;
   float yvel = a->yvel;
   int x = (int)a->x;
   int y = (int)a->y;
   for (int i = 0; i < 8; i++) {
      int index = FindFreeAsteroidSlot();
      if (index == -1)
         return; // no more asteroid slots free so don't add any (rare!)
      struct asteroid *past = &asteroids[index];
      past->active = true;
      past->rotdir = 0;
      past->rotclockwise = Random(2) == 2;
      past->xvel = (xdir[i]*Random(3) + xvel)*speedFactor;
      past->yvel = (ydir[i]*Random(3) + yvel)*speedFactor;
      past->xtime = 2 + Random(5);
      past->xcount = past->xtime;
      past->ytime = 2 + Random(5);
      past->ycount = past->ytime;
      past->rottime = 2 + Random(8);
      past->rotcount = past->rottime;
      int newsizeindex = 3; // smallest
      if (i%2 == 1) // if i is odd add in next smaller size
         newsizeindex = sizeindex + 1;
      past->sizeindex = newsizeindex;
      past->r.w = sizes[newsizeindex];
      past->r.h = sizes[newsizeindex];
      past->x = (float)xdir[i]*(sizes[newsizeindex]*6/5) + x;
      past->y = (float)ydir[i]*(sizes[newsizeindex]*6/5) + y;
      past->r.y = (int)past->y;
      past->r.x = (int)past->x;
   }
}

// DestroyAsteroid 1 from index, create 4 smaller plus 4 size 3
void DestroyAsteroid(int deadIndex) {
   struct asteroid *a = &asteroids[deadIndex];
   AddExplosion(a->r.x + a->r.w/2, a->r.y + a->r.h/2);
   DestroyThisAsteroid(a);
}

// DestroyAlienShip
void DestroyAlienShip(struct alien *a) {
   a->active = false;
   AddExplosion(a->r.x + a->r.w/2, a->r.y + a->r.h/2);
}

// test code
void BlowUpAsteroids(void) {
   for (int i = 0; i < MAXASTEROIDS; i++) {
      if (asteroids[i].active && Random(10) < 4) {
         DestroyAsteroid(i);
      }
   }
}

// fire a buller- first find a slot and then work out where it should appear relative to player ship then add to bullets
void DoFireBullet(void) {
   if (playerBulletCount == MAXPLAYERBULLETS) return;
   int index = -1;
   for (int i = 0; i < MAXBULLETS; i++) {
      if (!bullets[i].active) { // found a slot
         index = i;
         break;
      }
   }
   if (index == -1) return; // no free slots as all bullets in play
   PlayASound(CANNONSOUND);
// This starts the bullet at the nose, irrespective of rotation.
   int x = (int)round(Player.x + bulletx[Player.dir]);
   int y = (int)round(Player.y + bullety[Player.dir]);
   struct bullet *pbullet = &bullets[index];
   pbullet->active = true;
   pbullet->type = tBullet;
   pbullet->ttl = 120;
   pbullet->x = x*1.0f;
   pbullet->y = y*1.0f;
   pbullet->r.x = (int)x;
   pbullet->r.y = (int)y;
   pbullet->r.h = 3;
   pbullet->r.w = 3;
   pbullet->timer = 3;
   pbullet->countdown = 1;
   pbullet->vx = Player.vx + thrustx[Player.dir]*5;
   pbullet->vy = Player.vy + thrusty[Player.dir]*5;
   pbullet->playerbullet = bPlayer;
}

// move bullets *
void MoveBullets(void) {
   for (int i = 0; i < MAXBULLETS; i++) {
      struct bullet *pbullet = &bullets[i];
      if (pbullet->active && pbullet->countdown > 0) {
         pbullet->countdown--;
         if (pbullet->countdown == 0) {
            if (pbullet->ttl > 0) {
               pbullet->ttl--;
               if (pbullet->ttl == 0) {
                  pbullet->active = false;
                  continue; // expired, onto next bullet
               }
            }
         // move it
            pbullet->countdown = pbullet->timer;
            float vx = pbullet->x + pbullet->vx;
            float vy = pbullet->y + pbullet->vy;
            if (vx <= -2) // check for screen edges
               vx += 1024;
            else if (vx > 1022)
               vx = 0;
            if (vy <= -2)
               vy += 768;
            else if (vy > 766)
               vy = 0;
            pbullet->x = vx;
            pbullet->y = vy;
            pbullet->r.x = (int)pbullet->x;
            pbullet->r.y = (int)pbullet->y;
         }
      // even though a bullet doesn't move every frame, it still has to be added in to a cell every frame
         l2("Adding pointer bullets", sltoa(i));
         AddObjectToCells((pfirstpart)&bullets[i]);
      }
   }
}

// Cycle Explosions through all the phases
void CycleExplosions(void) {
   for (int i = 0; i < MAXEXPLOSIONS; i++) {
      if (explosions[i].frame > -1) {
         if (explosions[i].countdown > 0) {
            explosions[i].countdown--;
            if (explosions[i].countdown == 0) {
               explosions[i].frame++;
               if (explosions[i].frame == 64) {
                  explosions[i].frame = -1; // bye bye bang
                  continue;
               }
               explosions[i].countdown = 1;
            }
         }
      }
   }
}

// Handle all key presses
bool ProcessEvents(void) {
   while (SDL_PollEvent(&event)) {
      switch (event.type) {
         case SDL_KEYDOWN:
            keypressed = event.key.keysym.sym;
            switch (keypressed) {
               case QUITKEY:
                  CloseFlag = true;
               return false;
               case BOMBKEY:
                  BlowUpAsteroids();
               break;
               case COUNTERCLOCKWISEKEY:
                  rotateFlag = 1;
               break;
               case CLOCKWISEKEY:
                  rotateFlag = 2;
               break;
               case DEBUGKEY:
                  debugFlag = !debugFlag;
                  showFPS = !showFPS;
               break;
               case JUMPKEY:
                  jumpFlag = true;
               break;
               case PAUSEKEY:
                  pauseFlag = true;
               break;
               case SHIELDKEY:
                  shieldFlag = true;
               break;
               case THRUSTKEY:
                  thrustFlag = true;
               break;
               case ADDASTEROIDKEY:
#if 0
                  for (int i = 0; i < 1; i++) {
                     AddAsteroid(0);
                  }
#endif
               break;
               case FIREKEY:
                  fireFlag = true;
               break;
            }
         break;
      // if mouse click to close window
         case SDL_QUIT: return false;
         case SDL_KEYUP:
            rotateFlag = 0;
            thrustFlag = false;
            fireFlag = false;
            pauseFlag = false;
            jumpFlag = false;
            shieldFlag = false;
            shieldRadius = 38;
         break;
      }
   }
   return true;
}

void CheckBulletsFired(void) {
   if (fireFlag && SDL_GetTicks() - fireTimer > 100) {
      fireTimer = SDL_GetTicks();
      DoFireBullet();
   }
}

void CheckPause(void) {
   if (pauseFlag && SDL_GetTicks() - pauseTimer > 100) {
      pauseTimer = SDL_GetTicks();
      paused = !paused;
   }
}

// doHyperJump() find empty space on screen fo PlayerShip
void CheckJump(void) {
   if (jumpFlag && SDL_GetTicks() - jumpTimer > 3000) {
      jumpTimer = SDL_GetTicks();
      jumpFlag = false;
      int hx, hy;
      do {
         hx = 65 + Random(SCREENWIDTH - 130);
         hy = 65 + Random(SCREENHEIGHT - 130);
         if (IsEmptySpace(hx, hy)) break;
      } while (true);
      Player.x = (float)hx;
      Player.y = (float)hy;
      Player.vx = 0;
      Player.vy = 0;
   }
}

// Called when player loses a life or clears Level
void ReInitGame(void) {
   InitPlayer(Player.lives);
   InitThrustAngles();
   InitAsteroids();
   InitBullets();
   InitExplosions();
   InitTextSprites();
}

// Initialises each level
void InitLevel(int glevel) {
   rotTimer = SDL_GetTicks();
   speedTimer = rotTimer;
   fireTimer = rotTimer;
   jumpTimer = rotTimer;
   pauseTimer = rotTimer;
   coolDown = 0;
   controlTimer = 0;
   shieldStrength = 100;
   shieldRadius = 38;
   playerDead = false;
   ReInitGame(); // Clear BEATT Bullets Explosions Asteroids Thrust and Text
   struct level *thislevel = &levels[glevel];
   for (int sizeindex = 0; sizeindex < 4; sizeindex++) {
      int numtoadd = thislevel->nums[sizeindex];
#if 0
      numtoadd = 0; // TBR
#endif
      if (numtoadd > 0) {
         for (int aindex = 0; aindex < numtoadd; aindex++) {
            AddAsteroid(sizeindex); // TBR Debugging
         }
      }
   }
   thislevel->aliens = 1; // TBR
   for (int alienindex = 0; alienindex < thislevel->aliens; alienindex++) {
      AddAlienShip();
   }
   speedFactor = thislevel->factor;
   DrawLevelStart();
}

void DestroyObject(pfirstpart object) {
   switch (object->type) {
      case tAsteroid: // asteroid
         DestroyThisAsteroid((struct asteroid *)object); // cast back from firstpart to...
      break;
      case tBullet: // bullet
         object->active = false;
      break;
      case tPlayer:
         if (shieldFlag) break;
         object->active = false;
         playerDead = true;
         coolDown = 150;
         InitTextSprites(); // clear so only either of these two messages visible
         if (Player.lives == 1) {
            AddTextSpriteString("Game over!", (SCREENHEIGHT - 20)/2, 3);
         } else {
            AddTextSpriteString("Your ship destroyed!", (SCREENHEIGHT - 20)/2, 3);
         }
      break;
      case tAlien:
         DestroyAlienShip((struct alien *)object);
         object->active = false;
      break;
   }
}

// Returns a pointer to the top left byte of the correct mask depending on object type
pbyte GetMask(int type, int rotation, int size) {
   int offset = rotation*size*size;
   switch (type) {
   // Asteroid
      case tAsteroid: switch (size) {
         case 280: return a1mask + offset;
         case 140: return a2mask + offset;
         case 70: return a3mask + offset;
         case 35: return a4mask + offset;
      }
   // Bullet
      case tBullet: return bulletmask + offset;
   // Player
      case tPlayer: return plmask + offset;
   // Alien
      case tAlien: return alienmask + offset;
   }
   return NULL; // null - should never get here!
}

// Checks to see if two objects whose bounding boxes overlap actually touch.
bool Overlap(pfirstpart object1, pfirstpart object2, SDL_Rect *rect, int *bangx, int *bangy) {
#ifdef SHOWOVERLAP
   SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, SDL_ALPHA_OPAQUE); // Sets green colour
#endif
   int y = rect->y;
   int x = rect->x;
   int w = rect->w;
   int h = rect->h;
   int y1 = object1->r.y;
   int x1 = object1->r.x;
   int y2 = object2->r.y;
   int x2 = object2->r.x;
   int dir1 = object1->type == tBullet ? 0 : object1->rotdir;
   int dir2 = object2->type == tBullet ? 0 : object2->rotdir;
   int size1 = object1->r.h;
   int size2 = object2->r.h;
   pbyte pm1 = GetMask(object1->type, dir1, size1);
   pbyte pm2 = GetMask(object2->type, dir2, size2);
   int oy1 = y - y1; // determines offset into object (and mask) where the bounding boxes touch
   int oy2 = y - y2;
   int ox1 = x - x1;
   int ox2 = x - x2;
   pm1 += (oy1*size1) + ox1; // adjust mask 1 pointer to point into the mask where the bounding boxes touch top left corner
   pm2 += (oy2*size2) + ox2; // Same for object 2/mask 2
   for (int iy = 0; iy < h; iy++) {
      pbyte pl1 = pm1;
      pbyte pl2 = pm2;
      *bangy = iy + y;
      for (int ix = 0; ix < w; ix++) {
         *bangx = ix + x; // Sets the location of explosion (if it happens)
#ifndef SHOWOVERLAP
         if (*pl1++&*pl2++) {
            return true; // hit!
         }
#else
         if (*pl1++&*pl2++) {
            if (object1->type == tPlayer || object2->type == tPlayer) { // comment this line out- it'll show all overlaps but run very very slowly
               SDL_RenderDrawPoint(renderer, *bangx, *bangy);
            }
         }
#endif
      }
      pm1 += size1; // move mask pointers down by one row
      pm2 += size2;
   }
   return false; // missed!
}

// Check if player/bullet combination and if hit own bullets
bool PlayerHitOwnBullet(pfirstpart object1, pfirstpart object2) {
   struct bullet *pbullet = NULL;
   if (object1->type == tBullet && object2->type == tPlayer) {
      pbullet = (struct bullet *)object1;
   } else if (object2->type == tBullet && object1->type == tPlayer) {
      pbullet = (struct bullet *)object2;
   }
   if (pbullet == NULL) return false;
   if (pbullet->playerbullet == 0) return true; // alien bullet hit
   return false; // can't hit own bullet
}

// Check if player/bullet combination and if hit own bullets
bool AlienHitOwnBullet(pfirstpart object1, pfirstpart object2) {
   struct bullet *pbullet = NULL;
   if (object1->type == tBullet && object2->type == tAlien) {
      pbullet = (struct bullet *)object1;
   } else if (object2->type == tBullet && object1->type == tAlien) {
      pbullet = (struct bullet *)object2;
   }
   if (pbullet == NULL) return false;
   if (pbullet->playerbullet > 0) return true; // alien bullet hit alien - no damage
   return false;
}

// Double loop to see if any pairs of objects in this overlap.
void CheckAllObjects(int x, int y) {
   struct Cell *c = &cells[x][y];
   for (int index1 = 0; index1 < c->numobjects; index1++)
      for (int index2 = index1 + 1; index2 < c->numobjects; index2++) {
         SDL_Rect rect;
         if (c->ptrs[index1]->active && c->ptrs[index2]->active && SDL_IntersectRect(&c->ptrs[index1]->r, &c->ptrs[index2]->r, &rect)) {
            int bangy, bangx;
            if (Overlap(c->ptrs[index1], c->ptrs[index2], &rect, &bangx, &bangy)) {
               int type1 = c->ptrs[index1]->type;
               int type2 = c->ptrs[index2]->type;
               AddExplosion(bangx, bangy);
               if ((type1 == tAsteroid && type2 == tBullet) || (type2 == tAsteroid && type1 == tBullet)) {
                  if (coolDown == 0) { // no score if player ship killed in this frame or after!
                     score += 50;
                     AddTextSpriteAt("50", bangx, bangy - 20, 2);
                  }
               } else if ((type1 == tAlien && type2 == tBullet) || (type2 == tAlien && type1 == tBullet)) {
                  if (coolDown == 0) { // no score if player ship killed in this frame or after!
                     score += 250;
                     AddTextSpriteAt("250", bangx, bangy - 20, 2);
                  }
               }
               if (!PlayerHitOwnBullet(c->ptrs[index1], c->ptrs[index2])) {
                  DestroyObject(c->ptrs[index1]);
                  DestroyObject(c->ptrs[index2]);
               }
               if (!AlienHitOwnBullet(c->ptrs[index1], c->ptrs[index2])) {
                  DestroyObject(c->ptrs[index1]);
                  DestroyObject(c->ptrs[index2]);
               }
            }
         }
      }
}

void CheckCollisions(void) {
   for (int i = 0; i < numcells; i++) {
      int x = cellList[i].x;
      int y = cellList[i].y;
      if (cells[x][y].numobjects > 1) {
         CheckAllObjects(x, y);
      }
   }
}

// After playerShip hit, calls this for 150 frames CoolDown
// it just lets thing run a little rather than abruptly stopping and lets the
// Your Ship Destroyed/Game over etc messages have time to fully scroll
bool DoCoolDown(void) {
   if (coolDown == 0) return false;
   coolDown--;
   if (coolDown > 0) return false;
   if (playerDead) {
      Player.lives--;
      playerDead = false;
   }
   return true;
}

// Debug - shows cells on screen when called from RenderEverything
void ShowCells(void) {
   SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, SDL_ALPHA_OPAQUE);
   for (int y = 0; y < CELLY; y++) {
      int y1 = y*64;
      int y2 = y1 + 64;
      for (int x = 0; x < CELLX; x++) {
         int x1 = x*64;
         int x2 = x1 + 64;
         SDL_RenderDrawLine(renderer, x1, y1, (x2 - x1)/2, y1); // horizontal
         SDL_RenderDrawLine(renderer, x1, y1, x1, (y2 - y1)/2); // horizontal
         if (cells[x][y].numobjects > 0) {
            TextAt(x1 + 10, y1 + 10, sltoa(cells[x][y].numobjects), 1);
         }
      }
   }
}

// Are all asteroids and alien ships dead?
bool AllObjectsDestroyed(void) {
   for (int i = 0; i < MAXASTEROIDS; i++) {
      if (asteroids[i].active) return false;
   }
   for (int i = 0; i < MAXALIENS; i++) {
      if (aliens[i].active) return false;
   }
   return true;
}

// Go through all asteroids, identify those within 100 pixels range
// return true if so and coordinates of it's centre.
bool AnyTargetsNearBy(struct alien *palien, int *x, int *y, int *size) {
   int alx = palien->r.x;
   int aly = palien->r.y;
   int mindist = 1000000;
   bool foundone = false;
   for (int i = 0; i < MAXASTEROIDS; i++) {
      struct asteroid *ast = &asteroids[i];
      if (ast->active) {
         int dist = ((ast->r.x - alx)*(ast->r.x - alx) + (ast->r.y - aly)*(ast->r.y - aly));
         if (dist < 10000 && dist < mindist) {
            mindist = dist;
            *x = ast->r.x;
            *y = ast->r.y;
            *size = ast->r.w;
            foundone = true;
         }
      }
   }
   return foundone;
}

// calulates direction 0-7
void GetDir(int fx, int fy, int tx, int ty, float *xvel, float *yvel) {
   if (fx == tx) {
      *xvel = 0;
      *yvel = 5;
   } else if (fy == ty) {
      *yvel = 0;
      *xvel = 5;
   } else {
      float gr = (float)(tx - fx)/(ty - fy);
      *yvel = (ty - fy)/80*gr;
      *xvel = (tx - fx)/80*gr;
   }
}

// Need target size to centre bullet on middle of target
void FireBullet(int fromx, int fromy, int tox, int toy, int targetSize) {
   if (alienBulletCount == MAXALIENS*MAXBULLETSPERALIEN) return;
   tox += targetSize/2;
   toy += targetSize/2;
   int index = -1;
   for (int i = 0; i < MAXBULLETS; i++) {
      if (!bullets[i].active) { // found a slot
         index = i;
         break;
      }
   }
   if (index == -1) return; // no free slots as all bullets in play
// PlayASound(CANNONSOUND);
// Start the bullet at 25 pixels from the centre of the alien ship towards the target.
   struct bullet *pbullet = &bullets[index];
   GetDir(fromx, fromy, tox, toy, &pbullet->vx, &pbullet->vy);
   pbullet->active = true;
   pbullet->type = tBullet;
   pbullet->ttl = 120;
   pbullet->x = fromx + (10*pbullet->vx);
   pbullet->y = fromy + (10*pbullet->vy);
   pbullet->r.x = (int)pbullet->x;
   pbullet->r.y = (int)pbullet->y;
   pbullet->r.h = 3;
   pbullet->r.w = 3;
   pbullet->timer = 3;
   pbullet->countdown = 1;
   pbullet->playerbullet = index + 1;
}

// Every few frames, look around, detect any asteroid threats and shoot at them or the player!
void ControlAlienShips(void) {
   for (int i = 0; i < MAXALIENS; i++) {
      if (!aliens[i].active) continue;
      struct alien *palien = &aliens[i];
      if (palien->ttl-- > 1) continue; // subtract 1 after checking
      palien->ttl = (int)((60 + Random(30))/speedFactor);
      int ax = Player.r.x;
      int ay = Player.r.y;
      int size;
      if (AnyTargetsNearBy(palien, &ax, &ay, &size)) {
         FireBullet(palien->r.x, palien->r.y, ax, ay, size);
      } else {
         FireBullet(palien->r.x, palien->r.y, Player.r.x, Player.r.y, 64);
      }
      if (palien->moveTimer > 0) {
         palien->moveTimer--;
      }
      if (palien->moveTimer == 0) { // Change direction
         palien->moveTimer = 10 + Random(6);
         palien->xvel = (float)3 - Random(6);
         palien->yvel = (float)3 - Random(6);
      }
   }
}

// Use global variables playerbulletcount and alienbulletcount to cap firing
void CountBullets(void) {
   alienBulletCount = 0;
   playerBulletCount = 0;
   for (int i = 0; i < MAXBULLETS; i++) {
      struct bullet *pbullet = &bullets[i];
      if (pbullet->active) {
         switch (pbullet->playerbullet) {
            case -1:
            break;
            case 0:
               playerBulletCount++;
            break;
            case 1:
            case 2:
            case 3:
               alienBulletCount++;
            break;
         }
      }
   }
}

// Updates mainFlashFlag and alien ship timers
void UpdateTimers(void) {
   flashTimer++;
   if (flashTimer == 20) {
      flashFlag = !flashFlag;
      flashTimer = 0;
   }
   for (int i = 0; i < MAXALIENS; i++) {
      struct alien *palien = &aliens[i];
      if (palien->active) {
         palien->flTimer++;
         if (palien->flTimer >= ALIENFLASHFRAMES) {
            palien->flTimer = 0;
            palien->flShow = !palien->flShow;
         }
      }
   }
}

// if !shieldFlag and the shieldStrength is < 100 then increement it every 3 frames
// if shieldFlag then decrement it one every 2 frames
void AdjustShield(void) {
   if (shieldFlag) { // while shield up, drop 1 every frame
      if (shieldStrength > 0) {
         shieldStrength--;
      }
      if (shieldStrength == 0) {
         shieldFlag = false;
      }
   } else { // Shields slowly rebuilding
      shieldTimer++;
      if (shieldTimer == 3) {
         shieldTimer = 0;
         if (shieldStrength < 100) {
            shieldStrength++;
         }
      }
   }
}

void DisplayHighScores(void) {
   int flashIndex = 1;
   int hsTimer = 0;
   while (ProcessEvents()) {
      UpdateTimers(); // needed or flashFlag doesn't work
      int y = 130;
      if (CloseFlag || fireFlag) return;
      RenderTexture(textures[PLBACKDROP], 0, 0);
      TextAt(330, 30, "High Scores", 3.0f);
      if (flashFlag) {
         TextAt(400, 650, "Press space to start", 1.0f);
      }
      for (int i = 0; i < NUMSCORES; i++) {
         struct HighScoreEntry *entry = &highscores[i];
         char buffer[50];
         if (entry->score) {
            snprintf(buffer, sizeof(buffer) - 1, "%02d/%02d/%4d    %3s   %02d   %06d", entry->d, entry->m, entry->y, entry->initials, entry->level, entry->score);
         } else {
            snprintf(buffer, sizeof(buffer) - 1, "--/--/----    ---   --   000000");
         }
         if (flashIndex != i) {
            TextAt(345, y, buffer, 1.0f);
         }
         y += 50;
      }
      RenderEverything();
      hsTimer++;
      if (hsTimer == 18) {
         flashIndex++;
         hsTimer = 0;
         if (flashIndex == 10) flashIndex = 0;
      }
   }
}

// main game loop handles game play
void GameLoop(void) {
   tickCount = SDL_GetTicks();
   InitLevel(gameLevel);
   frameCount = 0;
   while (frameCount < 59) {
      RenderTexture(textures[PLBACKDROP], 0, 0);
      DrawTextSprites();
      RenderEverything();
   }
#ifdef TIMEGAMELOOP
   int counter = 0;
   int numTimeSlices = 0;
   double totalTime = 0.0;
#endif
   while (ProcessEvents()) {
#ifdef TIMEGAMELOOP
      Ticks = BegTimer();
#endif
      UpdateTimers();
      CheckPause();
      if (Player.lives == 0) break;
      ClearCellList();
      if (!paused) {
         DrawEverything();
         ControlAlienShips();
         MoveAsteroids();
         MoveAlienShips();
         RotatePlayerShip();
         CheckBulletsFired();
         MoveBullets();
         CheckJump();
         ApplyThrust();
         MovePlayerShip();
         CheckCollisions();
         CycleExplosions();
         CountBullets();
         AdjustShield();
         if (DoCoolDown()) return; // exits loop if player destroyed
      // exit if all asteroids etc destroyed
         if (coolDown == 0 && AllObjectsDestroyed()) {
            gameLevel++; if (gameLevel >= MaxLevels) gameLevel = MaxLevels - 1; //(@) Bound the level.
            coolDown = 150;
         }
      }
#if 0
      ShowCells();
#endif
      RenderEverything();
#ifdef TIMEGAMELOOP
      counter++;
      numTimeSlices++;
      totalTime += EndTimer(Ticks);
      if (counter == 60) {
         snprintf(timebuff, sizeof(timebuff) - 1, "%d %12.8f ", numTimeSlices, totalTime/numTimeSlices/1000000);
         counter = 0;
      }
      Ticks = BegTimer();
#endif
   }
}

void SetTodaysDate(struct HighScoreEntry *entry) {
   time_t t = time(NULL);
   time(&t);
   struct tm *rawtimep;
   if ((rawtimep = localtime(&t)) != NULL) {
      entry->d = rawtimep->tm_mday;
      entry->m = rawtimep->tm_mon + 1;
      entry->y = rawtimep->tm_year + 1900;
   }
}

// Gets initials and puts in high score if score high enough
void CheckHighScore(void) {
   bool Entered = false;
   int scoreIndex = -1;
   for (int i = NUMSCORES - 1; i >= 0; i--) { // check in reverse order
      if (score > highscores[i].score) {
         scoreIndex = i;
      }
   }
   if (scoreIndex == -1) return;
// Score goo enough to be in high score table
   char newInitials[4] = { '-', '-', '-', 0 };
   coolDown = 0;
   int letterIndex = -1;
   while (!Entered) {
      RenderTexture(textures[PLBACKDROP], 0, 0);
      UpdateTimers();
      TextAt(350, 30, "New High Score!", 3.0f);
      TextAt(200, 200, "Enter Initials:", 1.5F);
      TextAt(500, 200, newInitials, 1.5F);
      if (flashFlag) {
         TextAt(350, 5000, "Press three keys", 1.0f);
      }
      RenderEverything();
      while (SDL_PollEvent(&event)) {
         if (coolDown == 0) {
            switch (event.type) {
               case SDL_KEYDOWN:
                  keypressed = event.key.keysym.sym;
                  if (keypressed >= SDLK_a && keypressed <= SDLK_z) {
                     letterIndex++;
                     newInitials[letterIndex] = keypressed - 32; // A--Z
                     if (letterIndex == 2) {
                        coolDown = 50; // Time for 3rd letter to be seen on screen
                     }
                  }
            }
         }
      }
      if (coolDown > 0) {
         coolDown--;
         if (coolDown == 0) {
            Entered = true;
            break;
         }
      }
   }
// Shuffle all highscores after new one, down one
   for (int i = NUMSCORES - 1; i > scoreIndex; i--) {
      highscores[i] = highscores[i - 1];
   }
// Set new scores
   struct HighScoreEntry *entry = &highscores[scoreIndex];
   SetTodaysDate(entry);
   for (int i = 0; i < 4; i++) {
      entry->initials[i] = newInitials[i];
   }
   entry->level = gameLevel;
   entry->score = score;
   WriteHighScores();
}

int main(int argc, char *args[]) {
#if 0
   InitLogging("BigLog.txt");
#endif
   InitSetup();
   if (errorCount > 0) {
      return EXIT_FAILURE;
   }
   gameLevel = 0;
   while (!CloseFlag) {
      DisplayHighScores();
      if (CloseFlag) break;
      InitGame();
      while (Player.lives > 0) {
         GameLoop();
         if (CloseFlag) break;
      }
   // Game over so check if made into high score table
      CheckHighScore();
   }
#if 0
   CloseLogging();
#endif
   FinishOff();
   return EXIT_SUCCESS;
}
