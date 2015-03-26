#include <iostream>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <gfx/SDL2_gfxPrimitives.h>
#include <vector>
#include <cmath>

const float PI = 3.14159265;
using namespace std;
#include "boxCircleCollision.hpp"

const int windowHeight = 600;
const int windowWidth = windowHeight*2;
const int FRAMERATE = 60;
const double FRAME_TIME = 1000/FRAMERATE;
const int wallThickness = 75;
const int blockWidth = ((windowWidth/2)-(2*wallThickness))/10;
const int blockHeight = blockWidth;

int cursorRadius = 15;

int maxSouls = 500;
int activeSouls = 50;
int soulHeight = 30; //30
int soulWidth = 15;
double soulSpeed = 0.5;
double soulSpawnRate = 0.2;

struct Soul {double x, y, w, h;  float vx, vy;};

vector<Soul> souls(activeSouls);

vector<SDL_Rect> obstacles(0);
#include "custom lvl.txt"
int customObstacles[100];
SDL_Rect walls[4];


SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;

SDL_Texture *crosshairTex = NULL;
SDL_Texture *soulTex = NULL;
SDL_Texture *bgTex = NULL;
SDL_Texture *groundTex = NULL;
SDL_Texture *gunLockTex = NULL;
SDL_Texture *gunHeatTex = NULL;
SDL_Texture *gunOverheatTex = NULL;
SDL_Texture *gunHeatFrameTex = NULL;


SDL_Texture *loadTexture(string path)
{
	SDL_Texture *newTexture = NULL;

	SDL_Surface *loadedSurface = IMG_Load( path.c_str() );
    if( loadedSurface == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
    }
    else
    {
        //Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface(ren, loadedSurface );
        if( newTexture == NULL )
        {
            printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
        }

        //Get rid of old loaded surface
        SDL_FreeSurface( loadedSurface );
    }

	return newTexture;
}

bool init()
{
    bool initSuccess = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        initSuccess = false;
    }
    else
    {
        //Create window
        win = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (windowWidth), windowHeight, SDL_WINDOW_SHOWN );
        if( win == NULL )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            initSuccess = false;
        }
        else
        {
			ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (ren == NULL)
			{
				printf( "Renderer could not be created! SDL_Error: %s\n", SDL_GetError() );
				initSuccess = false;
			}
			else
			{
				SDL_SetRenderDrawColor(ren,0xff,0x00,0xff,0x00);

				int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
				if( !( IMG_Init( imgFlags ) & imgFlags ) )
				{
					printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
					initSuccess = false;
				}		
			}
        }
    }

    return initSuccess;
}

bool loadMedia()
{
    //Loading success flag
    bool success = true;

 	crosshairTex = loadTexture("Ship.bmp"); //use airshipBounded for visible boundaries
    if( crosshairTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	soulTex = loadTexture("pipes.png");
	if( soulTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	bgTex = loadTexture("bg.jpg");
	if( bgTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	groundTex = loadTexture("ROCKS.bmp"); //floor.png
	if( groundTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	gunLockTex = loadTexture("damShip.bmp");
	if( gunLockTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	gunHeatTex = loadTexture("green.jpg");
	if( gunLockTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	gunOverheatTex = loadTexture("red.jpg");
	if( gunLockTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	gunHeatFrameTex = loadTexture("pipes.png");
	if( gunLockTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

    return success;
}

void close()
{
    //Destroy textures
	SDL_DestroyTexture(crosshairTex);
	crosshairTex = NULL;

	SDL_DestroyTexture(soulTex);
	soulTex = NULL;

	SDL_DestroyTexture(bgTex);
	bgTex = NULL;

	SDL_DestroyTexture(groundTex);
	groundTex = NULL;

	SDL_DestroyTexture(gunLockTex);
	gunLockTex = NULL;

	SDL_DestroyTexture(gunHeatTex);
	gunHeatTex = NULL;

	SDL_DestroyTexture(gunOverheatTex);
	gunOverheatTex = NULL;

	SDL_DestroyTexture(gunHeatFrameTex);
	gunHeatFrameTex = NULL;

	//Destroy Renderer
	SDL_DestroyRenderer(ren);
	ren = NULL;

    //Destroy window
    SDL_DestroyWindow( win );
    win = NULL;

    //Quit SDL subsystems
	IMG_Quit();
    SDL_Quit();
}

float fRand()
{
	//dont forget to add srand(time(NULL)); to the start of the main method
	return rand()/(float)RAND_MAX;
}

/*pixel getter courtesy of http://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideo.html*/
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	//cout << bpp << endl;
    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
		//cout << (p[0] << 16) << endl;
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

//https://wiki.libsdl.org/SDL_GetRGB
bool readCustomLevel() {
	bool success = true;
	for (int i = 0; i < 100; i++)
	{
		customObstacles[i] = 0;
	}
	SDL_Surface *customLevel = SDL_LoadBMP("customlvl.BMP");
	if( customLevel == NULL )
    {
        printf( "Failed to load custom level image!\n" );
        success = false;
    }
	Uint8 r = 0, g = 0, b = 0;

	SDL_LockSurface(customLevel);

	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			SDL_GetRGB(getpixel(customLevel, i, j), customLevel->format, &r, &g, &b);
			if ((int)r == 0)
			{
				customObstacles[(i*10)+j] = 1;
			}
		}
	}
	
	cout << (int)r << " " << (int)g << " " << (int)b <<endl;

	return success;
}

void initObstacles(int levelChoice)
{
	obstacles.clear();
	switch (levelChoice) {
	case 1:
		for (int i = 0; i < 100; i++)
		{
			if (highwayLayout[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	case 2:
		for (int i = 0; i < 100; i++)
		{
			if (livingLayout[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	case 3:
		for (int i = 0; i < 100; i++)
		{
			if (killLayout[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	default:
		for (int i = 0; i < 100; i++)
		{
			if (customObstacles[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	}
}

void loadWalls() {
	walls[0].x = (windowWidth/2);
	walls[0].y = 0;
	walls[0].w = (windowWidth/2);
	walls[0].h = wallThickness;
	
	walls[1].x = (windowWidth/2);
	walls[1].y = 0;
	walls[1].w = wallThickness;
	walls[1].h = windowHeight;

	walls[2].x = (windowWidth/2);
	walls[2].y = windowHeight-wallThickness;
	walls[2].w = (windowWidth/2);
	walls[2].h = wallThickness;
	
	walls[3].x = windowWidth-wallThickness;
	walls[3].y = 0;
	walls[3].w = wallThickness;
	walls[3].h = windowHeight;
}

void initSouls() // MUST RUN AFTER OBSTACLE INIT
{
	for (int i = 0; i < souls.capacity(); i++)
	{
		bool soulPosOk = false;
		while (!soulPosOk)
		{
			souls[i].x = (windowWidth/2)+(fRand() * ((windowWidth/2)-wallThickness*2)) + wallThickness;
			souls[i].y = (fRand() * (windowHeight-wallThickness*2)) + wallThickness;
			souls[i].w = soulWidth;
			souls[i].h = soulHeight;
			float theta = fRand()*2*PI;
			souls[i].vx = cos(theta)*soulSpeed;
			souls[i].vy = sin(theta)*soulSpeed;

			SDL_Rect soulTestRect = {souls[i].x, souls[i].y, souls[i].w, souls[i].h};
			bool problematicSoul = false; 
			for (int j = 0; j < obstacles.size(); j++)
			{
				if (SDL_HasIntersection(&soulTestRect, &obstacles[j]) == SDL_TRUE)
				{
					cout << "PROBLEMATIC SOUL INDEX: "<< i << " " << obstacles.size() << endl;
					problematicSoul = true;
				}
			}
			for (int j = 0; j < 4; j++)
			{
				if (SDL_HasIntersection(&soulTestRect, &walls[j]) == SDL_TRUE)
				{
					cout << "PROBLEMATIC SOUL INDEX: "<< i << " " << obstacles.size() << endl;
					problematicSoul = true;
				}
			}
			if (!problematicSoul)
			{
				soulPosOk = true;
			}
		}
	}
}

Soul spawnNewSoul() {
	bool soulPosOk = false;
	Soul soul;
	while (!soulPosOk)
	{
		soul.x = (windowWidth/2)+(fRand() * ((windowWidth/2)-wallThickness*2)) + wallThickness;
		soul.y = (fRand() * (windowHeight-wallThickness*2)) + wallThickness;
		soul.w = soulWidth;
		soul.h = soulHeight;
		float theta = fRand()*2*PI;
		soul.vx = cos(theta)*soulSpeed;
		soul.vy = sin(theta)*soulSpeed;

		SDL_Rect soulTestRect = {soul.x, soul.y, soul.w, soul.h};
		bool problematicSoul = false; 
		for (int j = 0; j < obstacles.size(); j++)
		{
			if (SDL_HasIntersection(&soulTestRect, &obstacles[j]) == SDL_TRUE)
			{
				cout << "PROBLEMATIC SOUL INDEX: "<< "new spawn " << obstacles.size() << endl;
				problematicSoul = true;
			}
		}
		for (int j = 0; j < 4; j++)
		{
			if (SDL_HasIntersection(&soulTestRect, &walls[j]) == SDL_TRUE)
			{
				cout << "PROBLEMATIC SOUL INDEX: "<< "new spawn" << " " << obstacles.size() << endl;
				problematicSoul = true;
			}
		}
		if (!problematicSoul)
		{
			soulPosOk = true;
		}
	}
	return soul;
}



int main(int argc, char* argv[]) {

	srand(time(NULL));
	
	if( !init() )
    {
        printf( "Failed to initialize!\n" );
		return 0;
    }
	if( !loadMedia() )
    {
        printf( "Failed to load media!\n" );
		return 0;
    }
	if (!readCustomLevel())
	{
		printf( "Failed to load custom level!\n" );
		return 0;
	}

	//SDL_ShowCursor(0);	
	SDL_SetRelativeMouseMode(SDL_TRUE);

	initObstacles(8);
	loadWalls();
	initSouls();	

	bool running = true;
	float existingSouls = 0.5;
	int frame = 0;

	int mouseMotionX = 0, mouseMotionY = 0;
	//int newMouseX = 0, newMouseY = 0;
	//SDL_GetMouseState(&mouseMotionX, &mouseMotionY);
	SDL_Rect mousePos = {windowWidth*3/4, windowHeight/2, cursorRadius*2, cursorRadius*2};
	SDL_Rect bgRect = {windowWidth/2, 0, windowWidth/2, windowHeight};
	bool cursorCollidedX = false;
	bool cursorCollidedY = false;

	bool firingBullets = false;
	bool mouseButtonDown = false;
	double gunHeat = 0.0;
	bool gunLock = false;
	int bulletDelay = FRAMERATE / 5;

	int killCount = 0;

	/*SDL_Rect rightSide;
		rightSide.x = windowWidth/2;
		rightSide.y = 0;
		rightSide.w = windowWidth/2;
		rightSide.h = windowHeight;
		SDL_RenderSetViewport(ren, &rightSide);*/

	SDL_Rect gunHeatBar;
		gunHeatBar.x = windowWidth - (wallThickness*2/3);
		gunHeatBar.y =  windowHeight-wallThickness/2;
		gunHeatBar.w = wallThickness/3;
		gunHeatBar.h = windowHeight/4;
	SDL_Rect gunHeatBarFill;
		gunHeatBarFill.x = gunHeatBar.x;
		gunHeatBarFill.y = gunHeatBar.y;
		gunHeatBarFill.w = gunHeatBar.w;
		gunHeatBarFill.h = gunHeatBar.h * (gunHeat/(double)5);
	SDL_Point turningPoint = {gunHeatBar.w/2,0};
	
	while (running)	
	{
		int frameTimeStart = SDL_GetTicks();

		//read input
		SDL_Event ev;
		while(SDL_PollEvent(&ev) != 0) {
			if(ev.type == SDL_QUIT) 
			{
				running = false;
			}
			else if (ev.type == SDL_KEYDOWN)
			{
				switch (ev.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						running = false;
						break;
					default:
						break;
				}
			}
			else if (ev.type == SDL_MOUSEMOTION)
			{
				mouseMotionX = ev.motion.xrel*2;
				mouseMotionY = ev.motion.yrel*2;
			}
			else if(ev.type == SDL_MOUSEBUTTONDOWN)
			{
				mouseButtonDown = true;
				//firingBullets = true;
			}
			else if (ev.type == SDL_MOUSEBUTTONUP)
			{
				mouseButtonDown = false;
				//firingBullets = false;
			}
		}

		//simulate
		frame++;
		/*if (frame % FRAMERATE == 0)
		{
			cout << SDL_GetTicks() << endl;
		}*/

		//if gun can shoot
		if (mouseButtonDown && bulletDelay == 0 && !gunLock)
		{
			firingBullets = true;
			bulletDelay = FRAMERATE/5;
			gunHeat += 0.25;
			cout << gunHeat << endl;
		}
		else if (bulletDelay > 0)
		{
			firingBullets = false;
			bulletDelay--;
		}
		
		//cool down gun
		if ((gunHeat > 0 && bulletDelay == 0))
		{
			if (gunLock)
			{
				gunHeat -= 0.025;
				cout << gunHeat << endl;
			}
			else
			{
				gunHeat -= 0.05;
				cout << gunHeat << endl;
			}
			
		}

		//lock gun
		if (gunHeat >= 5)
		{
			gunLock = true;
		}
		else if (gunHeat <= 0.25)
		{
			gunLock = false;
		}




		//MOUSE BLOCK
		SDL_Rect projectedMousePositionX = {mousePos.x+mouseMotionX, mousePos.y, cursorRadius*2, cursorRadius*2};
		SDL_Rect projectedMousePositionY = {mousePos.x, mousePos.y+mouseMotionY, cursorRadius*2, cursorRadius*2};
		for (int i = 0; i < 4; i++)
		{
			if (collisionDetectedBoxCircle(walls[i], 0, projectedMousePositionX, cursorRadius))
			{
				cursorCollidedX = true;
			}
			if (collisionDetectedBoxCircle(walls[i], 0, projectedMousePositionY, cursorRadius))
			{
				cursorCollidedY = true;
			}
		}
		for (int i = 0; i < obstacles.size(); i++)
		{
			if (collisionDetectedBoxCircle(obstacles[i], 0, projectedMousePositionX, cursorRadius))
			{
				cursorCollidedX = true;
			}
			 if (collisionDetectedBoxCircle(obstacles[i], 0, projectedMousePositionY, cursorRadius))
			{
				cursorCollidedY = true;
			}
		}
		if (!cursorCollidedX)
		{
			mousePos.x += mouseMotionX;			
			mouseMotionX = 0;			
		}
		if (!cursorCollidedY)
		{
			mousePos.y += mouseMotionY;
			mouseMotionY = 0;
		}
		cursorCollidedX = false, cursorCollidedY = false;

		//if(frame % ((FRAMERATE/4) == 0 && souls.size() < maxSouls && fRand() < soulSpawnRate)
		if(frame % ((FRAMERATE/4)*(int)(1/soulSpawnRate)) == 0 && souls.size() < maxSouls)
		{
			souls.push_back(spawnNewSoul());
			cout << souls.size() << endl;
		}

		//SOUL BLOCK
		//int newTime = SDL_GetTicks();
		int markedForDestruction = -1;
		bool soulKilledThisFrame = false;
		for (int i = 0; i < souls.size(); i++)
		{
			//CHECK IF DEAD
			bool soulDead = false;
			SDL_Rect testSoul = {souls[i].x, souls[i].y, soulWidth, soulHeight};
			if (firingBullets && !soulKilledThisFrame && collisionDetectedBoxCircle(testSoul,0,mousePos,cursorRadius))
			{
				soulKilledThisFrame = true;
				cout << i << " ";
				markedForDestruction = i;
				cout << markedForDestruction << " ";
				soulDead = true;
				killCount++;
				cout << killCount << endl;
				cout << souls[i].x << " " << souls[i].y << endl << souls[i].y+souls[i].h << " " << souls[i].x+souls[i].w << endl;
			}
			
			if (!soulDead)
			{		
				SDL_Rect astralX = {souls[i].x + souls[i].vx, souls[i].y, soulWidth, soulHeight};
				SDL_Rect astralY = {souls[i].x, souls[i].y + souls[i].vy, soulWidth, soulHeight};
				//BOUNCE OFF WALLS
				if (souls[i].x < (windowWidth/2)+wallThickness)
				{
					souls[i].vx *= -1;
					souls[i].x = (windowWidth/2)+wallThickness;
				}
				else if (souls[i].x > windowWidth - wallThickness-soulWidth)
				{
					souls[i].vx *= -1;
					souls[i].x = windowWidth - wallThickness - soulWidth;
				}
				if (souls[i].y < wallThickness)
				{
					souls[i].vy *= -1;
					souls[i].y = wallThickness;
				}
				else if (souls[i].y > windowHeight - wallThickness-soulHeight)
				{
					souls[i].y = windowHeight - wallThickness-soulHeight;
					souls[i].vy *= -1;
				}

				bool xBounce = false, yBounce = false;
				//BOUNCE OFF OBSTACLES
				for (int j = 0; j < obstacles.size(); j++)
				{
					if (!xBounce && SDL_HasIntersection(&astralX, &obstacles[j]) == SDL_TRUE)
					{
						souls[i].vx *= -1; 
						xBounce = true;
					}
					if (!yBounce && SDL_HasIntersection(&astralY, &obstacles[j]) == SDL_TRUE)
					{
						souls[i].vy *= -1;
						yBounce = true;
					}
				}				
				
				//MOVE
				souls[i].x += souls[i].vx/**(soulSpeed*((newTime-frameTimeStart)/1000.00))*/;
				souls[i].y += souls[i].vy/**(soulSpeed*((newTime-frameTimeStart)/1000.00))*/;
			}
		}
		if (markedForDestruction >= 0)
		{
			souls.erase(souls.begin() + markedForDestruction);
		}

		//draw
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, bgTex, NULL, &bgRect);

		
		//SDL_RenderCopyEx(ren, soulTex, NULL, &target, frame, NULL, SDL_FLIP_NONE);

		
		//obstacles
		for (int i = 0; i < obstacles.size(); i++)
		{
			SDL_RenderCopy(ren, groundTex, NULL, &obstacles[i]);
		}
		
		//walls
		for (int i  = 0; i < 4; i ++)
		{
			SDL_RenderCopy(ren, groundTex, NULL, &walls[i]);
		}

		//souls
		for (int i = souls.size()-1; i >= 0; i--)
		{
			SDL_Rect soulRect = {souls[i].x, souls[i].y, souls[i].w, souls[i].h};
			SDL_RenderCopy(ren, soulTex, NULL, &soulRect);
		}

	
		//cursor
		if (!gunLock)
		{
			SDL_RenderCopyEx(ren, crosshairTex, NULL, &mousePos, frame, NULL, SDL_FLIP_NONE);
		}
		else
		{
			SDL_RenderCopyEx(ren, gunLockTex, NULL, &mousePos, frame, NULL, SDL_FLIP_NONE);
		}
		//cout <<"x" << mousePos.x << endl;

		gunHeatBarFill.h = gunHeatBar.h * (gunHeat/(double)5);
		SDL_RenderCopyEx(ren, gunHeatFrameTex, NULL, &gunHeatBar, 180, &turningPoint, SDL_FLIP_VERTICAL);
		if (!gunLock)
		{
			SDL_RenderCopyEx(ren, gunHeatTex, NULL, &gunHeatBarFill, 180, &turningPoint, SDL_FLIP_VERTICAL);
		}
		else
		{
			SDL_RenderCopyEx(ren, gunOverheatTex, NULL, &gunHeatBarFill, 180, &turningPoint, SDL_FLIP_VERTICAL);
		}

		SDL_RenderPresent(ren);		

		//delay
		if ((FRAME_TIME - (SDL_GetTicks() - frameTimeStart)) > 0)
		{
			SDL_Delay(FRAME_TIME - (SDL_GetTicks() - frameTimeStart));
		}
	}

	close();
	return 0;
}