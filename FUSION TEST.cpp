#include <iostream>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <gfx/SDL2_gfxPrimitives.h>
#include <vector>
#include <cmath>
#include <random>
#include <irrklang.h>
#include <ik_ISound.h>
#include <string>
#include <fstream>

const float PI = 3.14159265;

using namespace std;
using namespace irrklang;
#include "boxCircleCollision.hpp"

//CONSTANTS FOR WINDOW AND FPS
SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
const int windowHeight = 600;//600
const int windowWidth = windowHeight*2;//1000
const int viewPortWidth = windowWidth/2;
const int FRAMERATE = 60;
const double FRAME_TIME = 1000/FRAMERATE;

//POSSIBLE bool running = false;


//RHYTHM GAME
//CONSTANTS FOR RHYTHM GAME'S GRAPHICS
const int numberOfSouls = 3;
const double soulThreshold = 0; //(this is the maximum height the soul can float to before it's a miss)
const double dyingBody = 400; //(this is the bottom or starting position of a soul)
const double soulNoteHeight = 70;
const double soulNoteWidth = 80;
const int maxNotes = 500;
const int soulDist = dyingBody - soulThreshold; //number of pixels
const int beatDist = soulDist/4; //distance a soul travels PER BEAT
//vector where the souls are stored
SDL_Rect soulNotes[numberOfSouls];
//rectangle for when you should hit the notes
SDL_Rect hitZone = {0, soulThreshold, viewPortWidth, soulNoteHeight};
SDL_Rect feverBar;
SDL_Rect dyingBodies[numberOfSouls];
SDL_Rect lanes[numberOfSouls];
SDL_Rect startScreenRect = {0, 0, windowWidth, windowHeight};

//CONSTANTS FOR THE GAME'S MUSIC
ISoundEngine* engine = createIrrKlangDevice();
ISound* currentSound = 0;
double bpm;// = 114.9;//121.37 Livin on a Prayer //114.9 Highway to Hell //104.63 The Kill
double crotchet;// = 60/bpm; //beat duration in seconds
string songFilename;// = "AC-DC - Highway to Hell.mp3";
int playerSelectedSong = 1;

//VARIABLES FOR SYNCING GRAPHICS AND MUSIC
double songTime; //global variable to use for the beat
double lastReportedPlayheadPosition;
double currentPlayheadPosition = 0;
//---------------------------------
double msCrotchet = crotchet*1000; //time duration of a beat in ms
double framesPerBeat = msCrotchet/FRAME_TIME;
double pixelsPerFrame = soulDist/framesPerBeat;

//VARIABLES FOR SCORING
int timesMissed = 0;
int timesHit = 0;
int combo = 0;
int highestPlayerCombo = 0;
int playerScore;
int minimumFever;
bool playerStrum1 = false;
bool playerStrum2 = false;
bool playerStrum3 = false;
bool alreadyHit = false;
bool fever = false;

//DECLARATIONS FOR THE LATENCY TESTS
SDL_Rect visual_latency_flash; //flashes the screen a different color at a regular interval
double visualLatencySetting = 0;

//RANDOM NUMBER GENERATOR STUFF
const int sequenceCount=maxNotes;
int soulSequence[sequenceCount];
//int soulsAtOnce[sequenceCount];
int sequenceID = 0;
//int soulsAtOnceID = 0;
//double seed2 = bpm;
double seedStep = 1232.57129;

//READING FILES
string userSettings[10];
string savedHighScore[10];

//-----------------------------------------------------------GENERATE SEQUENCE OF NOTES-----------------------------------------------------------------------
void initializeSequence()
{	
	hash<string> str_hash;
	double seed1 = str_hash(songFilename);
	uniform_int_distribution<int> distribution1(0,2);
	//uniform_int_distribution<int> distribution2(1,3);
	for (int i = 0; i < maxNotes; i++)
	{
		//using subtract with carry engine
		ranlux24_base generator1(seed1);
		//ranlux24_base generator2(seed2);
		int randomNumber1 = distribution1(generator1);
		//int randomNumber2 = distribution2(generator2);
		soulSequence[i] = randomNumber1;
		//soulsAtOnce[i] = randomNumber2;
		seed1 += seedStep;
		//seed2 += seedStep;
	}
}

//SHOOTING GAME
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

//-------------------------------------------------------------------------------------------------------------------------------------
//TEXTURE DECLARATIONS
SDL_Texture *soulTex = NULL;
SDL_Texture *dyingBodyTex = NULL;
SDL_Texture *soulThresholdTex = NULL;
SDL_Texture *onBeatTex = NULL;
SDL_Texture *feverBarTex = NULL;
SDL_Texture *laneTex = NULL;
SDL_Texture *currentLaneTex = NULL;
SDL_Texture *startScreenTex = NULL;
SDL_Texture *crosshairTex = NULL;
//POSSIBLE SDL_Texture *soulTex = NULL;
SDL_Texture *bgTex = NULL;
SDL_Texture *groundTex = NULL;
SDL_Texture *gunLockTex = NULL;
SDL_Texture *gunHeatTex = NULL;
SDL_Texture *gunOverheatTex = NULL;
SDL_Texture *gunHeatFrameTex = NULL;

//--------------------------------------------------LOAD TEXTURE METHOD-----------------------------------------------------------
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
        win = SDL_CreateWindow( "Life of the Party", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN );
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
				else
				{
					if (!engine)
					{
						initSuccess = false;
					}
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

 	soulTex = loadTexture("airship.png"); //use airshipBounded for visible boundaries
    if( soulTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	dyingBodyTex = loadTexture("pipes.png");
	if( dyingBodyTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	soulThresholdTex = loadTexture("floor.png");
	if( soulThresholdTex == NULL )
    {
        printf( "Failed to load texture image!\n" );
        success = false;
    }

	onBeatTex = loadTexture("damShip.bmp");
	if( onBeatTex == NULL )
	{
		printf( "Failed to load texture image!\n" );
        success = false;
	}

	feverBarTex = loadTexture("bg.jpg");
	if( feverBarTex == NULL )
	{
		printf( "Failed to load texture image!\n" );
        success = false;
	}

	laneTex = loadTexture("laneDefault.png");
	if( laneTex == NULL )
	{
		printf( "Failed to load texture image!\n" );
        success = false;
	}

	currentLaneTex = loadTexture("laneCurrent.png");
	if( currentLaneTex == NULL )
	{
		printf( "Failed to load texture image!\n" );
        success = false;
	}

	startScreenTex = loadTexture("startScreen.png");
	if( startScreenTex == NULL )
	{
		printf( "Failed to load texture image!\n" );
        success = false;
	}

	crosshairTex = loadTexture("Ship.bmp"); //use airshipBounded for visible boundaries
    if( crosshairTex == NULL )
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

//---------------------------------------------------CLOSING METHOD---------------------------------------------
void close()
{
    //Destroy textures
	SDL_DestroyTexture(soulTex);
	soulTex = NULL;

	SDL_DestroyTexture(dyingBodyTex);
	dyingBodyTex = NULL;

	SDL_DestroyTexture(soulThresholdTex);
	soulThresholdTex = NULL;

	SDL_DestroyTexture(feverBarTex);
	feverBarTex = NULL;

	SDL_DestroyTexture(laneTex);
	laneTex = NULL;

	SDL_DestroyTexture(currentLaneTex);
	currentLaneTex = NULL;

	SDL_DestroyTexture(startScreenTex);
	startScreenTex = NULL;

	engine->drop();
	if(currentSound != NULL)
	{
		currentSound->drop();
	}

	SDL_DestroyTexture(crosshairTex);
	crosshairTex = NULL;

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


//RHYTHM GAME
//-----------------------------------------------------LOAD A FILE-------------------------------------------------------
void loadFile(char *fileName, string fileContents[])
{
	string line;
	ifstream myfile;
	myfile.open(fileName);
	if (myfile.is_open())
	{
		int i=0;
		while ( getline (myfile,line) )
		{
			fileContents[i] = line;
			i++;
		}
		myfile.close();
	}
}
//---------------------------------------------BEGIN SONG-------------------------------
double playMusic()
{
	double musicStartTime;
	musicStartTime = SDL_GetTicks();
	currentSound = engine->play2D(songFilename.c_str(), false,false,true);
	lastReportedPlayheadPosition = 0;
	return musicStartTime;
}
//------------------------------------INITIALIZE STARTING SOUL POSITIONS----------------------------------------
void initializeSoulPosition(int i, double startingPosition)
{
	soulNotes[i].x = ((i*2+1)*viewPortWidth)/(numberOfSouls*2)-soulNoteWidth/2;
	soulNotes[i].y = startingPosition;
	soulNotes[i].w = soulNoteWidth;
	soulNotes[i].h = soulNoteHeight;
}
//------------------------------------------------------INITIALIZE THE DYING BODIES--------------------------------------------
void initializeDyingBodies()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		dyingBodies[i].x = ((1+i*2)*viewPortWidth)/(numberOfSouls*2)-(soulNoteWidth/2);
		dyingBodies[i].y = dyingBody;
		dyingBodies[i].w = soulNoteWidth;
		dyingBodies[i].h = soulNoteHeight;
	}
}
//----------------------------------------------------INITIALIZE LANES-------------------------------------------------------------
void initializeLanes()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		lanes[i].x = i*(viewPortWidth/numberOfSouls);
		lanes[i].y = soulThreshold + soulNoteHeight;
		lanes[i].w = viewPortWidth/3;
		lanes[i].h = windowHeight - soulNoteHeight;
	}
}
//-------------------------------------------RENDERING METHOD FOR LOOP------------------------
void renderSoulFloating(int startingIndex)//, int floatingSouls)
{	
	/*for(int j = 0; j < floatingSouls; j++)
	{
		if(soulSequence[startingIndex] != soulSequence[startingIndex+j] || j == 0)
		{
			souls[soulSequence[startingIndex+j]].y -= soulThreshold + (songTime/(FRAME_TIME*sequenceCount*soulDist) + pixelsPerFrame);
			SDL_RenderCopy(ren, soulTex, NULL, &souls[soulSequence[startingIndex+j]]);
			if (souls[soulSequence[startingIndex+j]].y + souls[soulSequence[startingIndex+j]].h <= soulThreshold)
			{
				initializeSoulPosition(soulSequence[startingIndex+j], dyingBody);
			}
		}
	}*/

	//souls[startingIndex].y -= soulThreshold + (songTime/(FRAME_TIME*sequenceCount*soulDist) + pixelsPerFrame); //sync equation version 1
	soulNotes[startingIndex].y -= soulThreshold + (songTime/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 2
	//souls[startingIndex].y -= soulThreshold + (currentPlayheadPosition/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 3
	if (soulNotes[startingIndex].y + soulNotes[startingIndex].h <= soulThreshold)
	{
		initializeSoulPosition(startingIndex, dyingBody);
	}
	SDL_RenderCopy(ren, soulTex, NULL, &soulNotes[startingIndex]);
}
//----------------------------------------------SCORE TRACKING-----------------------------------------------
void scoringCheck(int soulIndex)
{
		if(soulIndex == 0&&playerStrum1 == true)
		{
			timesHit++;
			combo++;
			cout << "good job x" <<timesHit << endl;
			alreadyHit = true;
			playerStrum1 = false;
		}
		else if(soulIndex==1&&playerStrum2==true)
		{
			timesHit++;
			combo++;
			cout << "good job x" <<timesHit << endl;
			alreadyHit = true;
			playerStrum2 = false;
		}
		else if (soulIndex==2&playerStrum3==true)
		{
			timesHit++;
			combo++;
			cout << "good job x" <<timesHit << endl;
			alreadyHit = true;
			playerStrum3 = false;
		}
}
//-------------------------------------------------------------------FEVER STATUS-------------------------------------------------------------------
void comboFever()
{
	if (combo >= minimumFever)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth;
		feverBar.h = 40;
		fever = true;
	}
	else if (combo >= minimumFever*0.75)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth*0.75;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= minimumFever*0.5)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth*0.5;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= minimumFever*0.25)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth*0.25;
		feverBar.h = 40;
		fever = false;
	}
	else
	{
		feverBar.x = 0;
		feverBar.y = 0;
		feverBar.w = 0;
		feverBar.h = 0;
		fever = false;
	}
	SDL_RenderCopy(ren, feverBarTex, NULL, &feverBar);
	if (combo > highestPlayerCombo)
	{
		highestPlayerCombo = combo;
	}
}
//------------------------------------------------------------SET GAME DIFFICULTY-------------------------------------------------------------
void setDifficulty(string diffSetting)
{
	if (diffSetting == "Easy")
	{
		minimumFever = 20;
	}
	else if (diffSetting == "Normal")
	{
		minimumFever = 40;
	}
	else if (diffSetting == "Hard")
	{
		minimumFever = 80;
	}
}
//------------------------------------------------------------------SONG SELECT----------------------------------------------------------------------------
void songSelect(int selectedOption)
{
	if(selectedOption == 1)
	{
		songFilename="AC-DC - Highway to Hell.mp3";
		bpm = 114.9;
	}
	else if(selectedOption == 2)
	{
		songFilename="Bon Jovi - Living on a Prayer.mp3";
		bpm = 121.37;
	}
	else if(selectedOption == 3)
	{
		songFilename="03 30 Seconds To Mars - The Kill.mp3";
		bpm = 104.63;
	}
	else if(selectedOption == 4)
	{
		songFilename=userSettings[3];
		bpm = stod(userSettings[6]);
	}
}

//SHOOTING GAME
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
	case 0:
		for (int i = 0; i < 100; i++)
		{
			if (highwayLayout[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	case 1:
		for (int i = 0; i < 100; i++)
		{
			if (livingLayout[i] != 0)
			{
				SDL_Rect obstacleBlock = {(windowWidth/2)+wallThickness+(i%10)*blockWidth, wallThickness+(i/10)*blockHeight, blockWidth, blockHeight};
				obstacles.push_back(obstacleBlock);
			}
		}
		break;
	case 2:
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
	walls[0].x = (windowWidth/2)+0;
	walls[0].y = 0;
	walls[0].w = (windowWidth/2);
	walls[0].h = wallThickness;
	
	walls[1].x = (windowWidth/2)+0;
	walls[1].y = 0;
	walls[1].w = wallThickness;
	walls[1].h = windowHeight;

	walls[2].x = (windowWidth/2)+0;
	walls[2].y = windowHeight-wallThickness;
	walls[2].w = (windowWidth/2);
	walls[2].h = wallThickness;
	
	walls[3].x = (windowWidth/2)+(windowWidth/2)-wallThickness;
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
			souls[i].x = (windowWidth/2) + (fRand() * ((windowWidth/2)-wallThickness*2)) + wallThickness;
			//cout << souls[i].x << endl;
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

void playGame(int levSel)
{
	//************************************************USER INPUT AND HIGH SCORE STORAGE************************************************************
	loadFile("gameSettings.txt", userSettings);
	loadFile("highScore.txt", savedHighScore);
	songSelect(playerSelectedSong);
	//songFilename = userSettings[3];
	//bpm = stod(userSettings[6]);//121.37 Livin on a Prayer //114.9 Highway to Hell //104.63 The Kill
	crotchet = 60/bpm; //beat duration in seconds
	msCrotchet = crotchet*1000; //time duration of a beat in ms
	framesPerBeat = msCrotchet/FRAME_TIME;
	pixelsPerFrame = soulDist/framesPerBeat;
	double highScore = stoi(savedHighScore[1]);
	double highestComboScore = stoi(savedHighScore[6]);
	string playerNameScore = savedHighScore[3];
	string playerNameCombo = savedHighScore[8];
	string playerDifficulty = userSettings[9];
	//**************************************************************************************************************************************************
	setDifficulty(playerDifficulty);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	initObstacles(levSel);
	loadWalls();
	initSouls();

	bool running = true;
	double songStartTime = playMusic();
	initializeSequence();
	initializeDyingBodies();
	initializeLanes();
	bool scoreRecorded = false;
	for (int i = 0; i < numberOfSouls; i++)
	{
		initializeSoulPosition(i,dyingBody);
	}

	float existingSouls = 0.5;
	int frame = 0;

	int mouseMotionX = 0, mouseMotionY = 0;
	//int newMouseX = 0, newMouseY = 0;
	//SDL_GetMouseState(&mouseMotionX, &mouseMotionY);
	SDL_Rect mousePos = {(windowWidth/2)+(windowWidth/2)/2, windowHeight/2, cursorRadius*2, cursorRadius*2};
	bool cursorCollidedX = false;
	bool cursorCollidedY = false;

	bool firingBullets = false;
	bool mouseButtonDown = false;
	double gunHeat = 0.0;
	bool gunLock = false;
	int bulletDelay = FRAMERATE / 5;

	int killCount = 0;

	SDL_Rect rightSide;
		rightSide.x = (windowWidth/2)+windowWidth/2;
		rightSide.y = 0;
		rightSide.w = windowWidth/2;
		rightSide.h = windowHeight;
		//SDL_RenderSetViewport(ren, &rightSide);

	SDL_Rect gunHeatBar;
		gunHeatBar.x = (windowWidth/2)+(windowWidth/2) - (wallThickness*2/3);
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
		while(SDL_PollEvent(&ev) != 0) 
		{
			if(ev.type == SDL_QUIT) 
			{
				running = false;
			}
			else if (ev.type == SDL_KEYDOWN)
			{
				if (ev.key.keysym.sym == SDLK_1)
				{
					playerStrum1 = true;
				}
				if (ev.key.keysym.sym == SDLK_2)
				{
					playerStrum2 = true;
				}
				if (ev.key.keysym.sym == SDLK_3)
				{
					playerStrum3 = true;
				}
				if (ev.key.keysym.sym == SDLK_ESCAPE)
				{
					running = false;
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

		//keeps track of the songTime, songTime is kept manually until new playhead positions are reported, at which point it eases towards that
		songTime = SDL_GetTicks() - songStartTime;
		songStartTime = SDL_GetTicks();
		currentPlayheadPosition = currentSound->getPlayPosition();
		if(currentPlayheadPosition != lastReportedPlayheadPosition) 
		{
			songTime = (songTime + currentPlayheadPosition)/2;
			lastReportedPlayheadPosition = currentPlayheadPosition;
		}
		//CITE THE SOURCE OF THIS ALGORITHM HERE LATER ON OKAY

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
					souls[i].x = wallThickness;
				}
				else if (souls[i].x > (windowWidth) - wallThickness-soulWidth)
				{
					souls[i].vx *= -1;
					souls[i].x = (windowWidth/2) - wallThickness - soulWidth - 1;
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

		SDL_RenderClear(ren);
		//render the lanes and the dyingBodies
		for (int i = 0; i <numberOfSouls; i++)
		{
			if(i == soulSequence[sequenceID])
			{
				SDL_RenderCopy(ren, currentLaneTex, NULL, &lanes[i]);
			}
			else
			{
				SDL_RenderCopy(ren, laneTex, NULL, &lanes[i]);
			}
			SDL_RenderCopy(ren, dyingBodyTex, NULL, &dyingBodies[i]);
		}
		//check if the note is in the "beatZone", and check if a note has already been hit. if it hasn't already been hit, check if the user hit it
		if(SDL_HasIntersection(&soulNotes[soulSequence[sequenceID]], &hitZone) && !alreadyHit)
		{
			scoringCheck(soulSequence[sequenceID]);
			SDL_RenderCopy(ren, onBeatTex, NULL, &hitZone);
			//find a way to track combos or maybe a fever bar <3
		}
		else
		{
			//if the note is NOT in the beatZone or if the note has already been hit but the user presses the button again, return a miss
			SDL_RenderCopy(ren, soulThresholdTex, NULL, &hitZone);
			if(playerStrum1 == true)
			{
				timesMissed++;
				combo = 0;
				cout<<"miss! x"<<timesMissed<<endl;
				playerStrum1 = false;
			}
			if(playerStrum2 == true)
			{
				timesMissed++;
				combo = 0;
				playerStrum2 = false;
				cout<<"miss! x"<<timesMissed<<endl;
			}
			if(playerStrum3 == true)
			{
				timesMissed++;
				combo = 0;
				playerStrum3 = false;
				cout<<"miss! x"<<timesMissed<<endl;
			}
		}
		renderSoulFloating(soulSequence[sequenceID]);
		comboFever();
		if(currentSound->isFinished() == false)
		{
		//once the soul has reinitialized its position, increment the sequenceID. afterwards, check if the note was alreadyHit and reset it to false. if the note wasn't hit, the user missed the note.
			if(soulNotes[soulSequence[sequenceID]].y == dyingBody && sequenceID < sequenceCount)
			{
				if(alreadyHit == true)
				{
					alreadyHit = false;
				}
				else if(sequenceID != 0)
				{
					timesMissed++;
					combo = 0;
					cout<<"miss! x"<<timesMissed<<endl;
				}
				sequenceID++;
			}
		}
		else 
		{
			if(scoreRecorded == false && soulNotes[soulSequence[sequenceID]].y == dyingBody)
			{
				cout << "Your final score: " << timesHit << endl;
				if(timesHit > highScore)
				{
					highScore = timesHit;
					cout << "Congratulations! High score! Enter your name:";
					cin >> playerNameScore;
				}
				if(highestPlayerCombo > highestComboScore)
				{
					highestComboScore = highestPlayerCombo;
					cout << "Congratulations! Longest Streak! Enter your name:";
					cin >> playerNameCombo;
				}
				ofstream myfile;
				myfile.open ("highScore.txt");
				myfile << "High Score: " << endl;
				myfile << highScore << endl;
				myfile << "By: " << endl;
				myfile << playerNameScore << endl;
				myfile << " " << endl;
				myfile << "Longest Combo: " << endl;
				myfile << highestComboScore << endl;
				myfile << "By: " << endl;
				myfile << playerNameCombo << endl;
				myfile.close();
				scoreRecorded = true;
				running = false;
			}
		}

		//draw
		
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
}

int main(int argc, char* argv[]) 
{
	srand(time(NULL));

	if( !init() )
    {
        printf( "Failed to initialize!\n" );
		return 0;
    }
	if( !loadMedia() )
    {
        printf( "Failed to load media!\n" );
		return 9;
    }
    if (!readCustomLevel())
	{
		printf( "Failed to load custom level!\n" );
		return 0;
	}
	bool startScreen = true;
	bool beginGame = false;
	while (startScreen)	
	{	
		//read input
		SDL_Event ev;
		while(SDL_PollEvent(&ev) != 0) 
		{
			if(ev.type == SDL_QUIT) 
			{
				startScreen = false;
			}
			else if (ev.type == SDL_KEYDOWN)
			{
				if (ev.key.keysym.sym == SDLK_1)
				{
					playerSelectedSong = 1;
				}
				if (ev.key.keysym.sym == SDLK_2)
				{
					playerSelectedSong = 2;
				}
				if (ev.key.keysym.sym == SDLK_3)
				{
					playerSelectedSong = 3;
				}
				if (ev.key.keysym.sym == SDLK_4)
				{
					playerSelectedSong = 4;
				}
				if (ev.key.keysym.sym == SDLK_SPACE)
				{
					beginGame = true;
					startScreen = false;
				}
				if (ev.key.keysym.sym == SDLK_ESCAPE)
				{
					startScreen = false;
				}
			}
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, startScreenTex, NULL, &startScreenRect);
			SDL_RenderPresent(ren);
		}
	}
	if(beginGame)
	{
		playGame(1);
	}
	close();
	return 0;
}