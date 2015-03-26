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
#include "custom lvl.txt"

const int windowHeight = 600;
const int windowWidth = windowHeight*2;
const int FRAMERATE = 60;
const double FRAME_TIME = 1000/FRAMERATE;
const int blockWidth = (windowWidth/2)/12;
const int blockHeight = blockWidth;

int cursorRadius = 15;

int maxSouls = 500;
int activeSouls = 50;
int soulHeight = 30; //30
int soulWidth = 15;
double soulSpeed = 1.00;
double soulSpawnRate = 0.5;

struct Soul {double x, y, w, h;  float vx, vy;};

vector<Soul> souls(activeSouls);

vector<SDL_Rect> obstacles(0);


SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;

SDL_Texture *crosshairTex = NULL;
SDL_Texture *soulTex = NULL;
SDL_Texture *bgTex = NULL;
SDL_Texture *groundTex = NULL;
SDL_Texture *gunLockTex = NULL;


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

void initObstaclesCustom()
{
	for (int i = 0; i < 100; i++)
	{
		if (obstacleLayout[i] != 0)
		{
			SDL_Rect obstacleBlock = {blockWidth+(i%10)*blockWidth, blockHeight+(i/10)*blockHeight, blockWidth, blockHeight};
			obstacles.push_back(obstacleBlock);
		}
	}
}

void initSouls() // MUST RUN AFTER OBSTACLE INIT
{
	for (int i = 0; i < souls.capacity(); i++)
	{
		bool soulPosOk = false;
		while (!soulPosOk)
		{
			souls[i].x = (fRand() * (windowWidth/2)-blockWidth*2) + blockWidth;
			souls[i].y = (fRand() * windowHeight-blockHeight*2) + blockHeight;
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
		soul.x = (fRand() * (windowWidth/2)-blockWidth*2) + blockWidth;
		soul.y = (fRand() * windowHeight-blockHeight*2) + blockHeight;
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

	//SDL_ShowCursor(0);	
	SDL_SetRelativeMouseMode(SDL_TRUE);
	
	initObstaclesCustom();
	initSouls();
	

	bool running = true;
	float existingSouls = 0.5;
	int frame = 0;

	int mouseMotionX = 0, mouseMotionY = 0;
	//int newMouseX = 0, newMouseY = 0;
	//SDL_GetMouseState(&mouseMotionX, &mouseMotionY);
	SDL_Rect mousePos = {(windowWidth/2)/2, windowHeight/2, cursorRadius*2, cursorRadius*2};
	bool cursorCollidedX = false;
	bool cursorCollidedY = false;

	bool firingBullets = false;
	bool mouseButtonDown = false;
	double gunHeat = 0.0;
	bool gunLock = false;
	int bulletDelay = FRAMERATE / 5;

	int killCount = 0;

	SDL_Rect walls[4];
	walls[0].x = 0;
	walls[0].y = 0;
	walls[0].w = (windowWidth/2);
	walls[0].h = blockHeight;
	
	walls[1].x = 0;
	walls[1].y = 0;
	walls[1].w = blockWidth;
	walls[1].h = windowHeight;

	walls[2].x = 0;
	walls[2].y = windowHeight-blockHeight;
	walls[2].w = (windowWidth/2);
	walls[2].h = blockHeight;
	
	walls[3].x = (windowWidth/2)-blockWidth;
	walls[3].y = 0;
	walls[3].w = blockWidth;
	walls[3].h = windowHeight;

	SDL_Rect rightSide;
		rightSide.x = windowWidth/2;
		rightSide.y = 0;
		rightSide.w = windowWidth/2;
		rightSide.h = windowHeight;
		SDL_RenderSetViewport(ren, &rightSide);
	
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
		/*if (newMouseX < blockWidth)
		{
			mousePos.x = blockWidth;
			cout << mousePos.x << endl;
		}
		else if (newMouseX > (windowWidth/2)-blockWidth-cursorRadius*2)
		{
			mousePos.x = (windowWidth/2)-blockWidth-cursorRadius*2;
		}
		else
		{
			mousePos.x = newMouseX;
		}
		if (newMouseY < blockHeight)
		{
			mousePos.y = blockHeight;
		}
		else if (newMouseY > windowHeight-blockHeight-cursorRadius*2)
		{
			mousePos.y = windowHeight-blockHeight-cursorRadius*2;
		}
		else
		{
			mousePos.y = newMouseY;
		}*/
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

		if(frame % (FRAMERATE/4) == 0 && souls.size() < maxSouls && fRand() < soulSpawnRate)
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
				if (souls[i].x < blockWidth)
				{
					souls[i].vx *= -1;
					souls[i].x = blockWidth;
				}
				else if (souls[i].x > (windowWidth/2) - blockWidth-soulWidth)
				{
					souls[i].vx *= -1;
					souls[i].x = (windowWidth/2) - blockWidth - soulWidth - 1;
				}
				if (souls[i].y < blockHeight)
				{
					souls[i].vy *= -1;
					souls[i].y = blockHeight;
				}
				else if (souls[i].y > windowHeight - blockHeight-soulHeight)
				{
					souls[i].y = windowHeight - blockHeight-soulHeight;
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