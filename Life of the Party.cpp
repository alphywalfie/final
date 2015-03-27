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
#include <SDL_ttf.h>
#include <sstream>

const float PI = 3.14159265;
using namespace std;
using namespace irrklang;
#include "boxCircleCollision.hpp"

//CONSTANTS FOR WINDOW AND FPS
SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
const int windowHeight = 600;//600
const int windowWidth = windowHeight*2;
const int viewPortWidth = windowWidth/2;
const int FRAMERATE = 60;
const double FRAME_TIME = 1000/FRAMERATE;
//bool running = false;

//RHYTHM GAME CONSTS AND VARS
//CONSTANTS FOR THE GAME'S GRAPHICS
const int numberOfSouls = 3;
const double soulThreshold = 0; //(this is the maximum height the soul can float to before it's a miss)
const double dyingBody = 400; //(this is the bottom or starting position of a soul)
const double soulNoteHeight = 70;
const double soulNoteWidth = 80;
const double scoreBoardWidth = 70;
const double scoreBoardLeft = viewPortWidth - scoreBoardWidth;
const double scoreBoardHeight = 50;
const double feverBarWidth = viewPortWidth - scoreBoardWidth;
const int maxNotes = 500;
const int soulDist = dyingBody - soulThreshold; //number of pixels
//vector where the souls are stored
SDL_Rect soulNotes[numberOfSouls];
//rectangle for when you should hit the notes
SDL_Rect hitZone = {0, soulThreshold, viewPortWidth, soulNoteHeight};
SDL_Rect feverBar;
SDL_Rect dyingBodies[numberOfSouls];
SDL_Rect lanes[numberOfSouls];
SDL_Rect startScreenRect = {0, 0, windowWidth, windowHeight};
SDL_Rect notifiers[numberOfSouls];
SDL_Rect buttonPress[numberOfSouls];

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
SDL_Rect scoreBoard = {viewPortWidth-70, windowHeight -50, 40, 50};
SDL_Rect comboNotif = {viewPortWidth/2-50, windowHeight/2, 100, 40};

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

//RENDERING TEXT
TTF_Font *font;
SDL_Color black={0,0,0};
SDL_Color red={255,0,0};
SDL_Color white={255,255,255};
SDL_Surface *text_surface;
SDL_Texture *textTex;
SDL_Texture *hitTex = NULL;
SDL_Texture *comboTex = NULL;
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

//SHOOTING GAME CONSTS AND VARS
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
bool laserStock[3] = {0, 0, 0};

struct Soul {double x, y, w, h;  float vx, vy;};

vector<Soul> souls(activeSouls);

vector<SDL_Rect> obstacles(0);
#include "custom lvl.txt"
int customObstacles[100];
SDL_Rect walls[4];

//TEXTURE DECLARATIONS
SDL_Texture *soulTex = NULL;
SDL_Texture *dyingBodyTex = NULL;
SDL_Texture *soulThresholdTex = NULL;
SDL_Texture *onBeatTex = NULL;
SDL_Texture *feverBarTex = NULL;
SDL_Texture *laneTex = NULL;
SDL_Texture *currentLaneTex = NULL;
SDL_Texture *startScreenTex = NULL;
SDL_Texture *missTex = NULL;
SDL_Texture *buttonPressTex = NULL;
SDL_Texture *crosshairTex = NULL;
SDL_Texture *bgTex = NULL;
SDL_Texture *groundTex = NULL;
SDL_Texture *gunLockTex = NULL;
SDL_Texture *gunHeatTex = NULL;
SDL_Texture *gunOverheatTex = NULL;
SDL_Texture *gunHeatFrameTex = NULL;
SDL_Texture *soulsToKill = NULL;

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

SDL_Texture *renderTEXTure(string textToRender, SDL_Color color)
{
	SDL_Surface *renderedTextSurface = NULL;
	SDL_Texture *renderedTextTexture = NULL;
	renderedTextSurface = TTF_RenderText_Solid(font, textToRender.c_str() ,color);

	renderedTextTexture = SDL_CreateTextureFromSurface( ren, renderedTextSurface );
	SDL_FreeSurface( renderedTextSurface );
	return renderedTextTexture;
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

	if (TTF_Init() ==-1)
	{
		printf("Failed to initialize TTF!\n");
		success=false;
	}

	font = TTF_OpenFont("calibri.ttf", 14);
	if( font == NULL ) 
	{
		printf("Failed to load font!\n");
		success=false;
	}

	hitTex = renderTEXTure("Great!", black);
	if(hitTex ==NULL)
	{
		printf("Failed to render hitTex!\n");
		success=false;
	}

	missTex = renderTEXTure("Miss!",red);
	if(missTex == NULL)
	{
		printf("Failed to render missTex!\n");
		success=false;
	}

	buttonPressTex = loadTexture("pipes.png");
	if (buttonPressTex == NULL)
	{
		printf("Failed to load buttonTex!\n");
	}
    return success;
}

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

	SDL_DestroyTexture(onBeatTex);
	onBeatTex = NULL;

	SDL_DestroyTexture(crosshairTex);
	crosshairTex = NULL;

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

	SDL_DestroyTexture(hitTex);
	onBeatTex = NULL;

	SDL_DestroyTexture(textTex);
	textTex = NULL;

	SDL_DestroyTexture(missTex);
	missTex = NULL;

	SDL_DestroyTexture(comboTex);
	comboTex=NULL;

	engine->drop();
	if(currentSound != NULL)
	{
		currentSound->drop();
	}

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

//RHYTHM GAME METHODS
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
//--------------------------------------------------------SCORE BOARD------------------------------------------------------
void renderScoreBoard()
{
	stringstream strm;
	strm << timesHit;
	text_surface=TTF_RenderText_Solid(font, strm.str().c_str() ,black);

	textTex = SDL_CreateTextureFromSurface( ren, text_surface );
	SDL_FreeSurface( text_surface );
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
//--------------------------------------------------------INITIALIZE NOTIFIERS-------------------------------------------------------------------
void initializeNotifiers()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		notifiers[i].x = ((1+i*2)*viewPortWidth)/(numberOfSouls*2)-(soulNoteWidth/2);
		notifiers[i].y = soulThreshold+soulNoteHeight;
		notifiers[i].w = soulNoteWidth;
		notifiers[i].h = soulNoteHeight;
	}
}
//---------------------------------------------------------INITIALIZE BUTTON PRESS RECTANGLES----------------------------------------------
void initializeButtonPress()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		buttonPress[i].x = i*(viewPortWidth/numberOfSouls);
		buttonPress[i].y = soulThreshold;
		buttonPress[i].w = viewPortWidth/3;
		buttonPress[i].h = soulNoteHeight;
	}
}
//----------------------------------------------------------RENDER BUTTON PRESS-----------------------------------------------------------
void checkAndRenderButtonPress()
{
	if (playerStrum1 == true)
	{
		SDL_RenderCopy(ren, buttonPressTex, NULL, &buttonPress[0]);
	}
	if (playerStrum2 == true)
	{
		SDL_RenderCopy(ren, buttonPressTex, NULL, &buttonPress[1]);
	}
	if (playerStrum3 == true)
	{
		SDL_RenderCopy(ren, buttonPressTex, NULL, &buttonPress[2]);
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
	soulNotes[startingIndex].y -= (songTime/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 2
	//souls[startingIndex].y -= soulThreshold + (currentPlayheadPosition/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 3
	//soulNotes[startingIndex].y -= (songTime/(FRAME_TIME*(soulNotes[startingIndex].y/soulDist)) + pixelsPerFrame);
	if (soulNotes[startingIndex].y + soulNotes[startingIndex].h <= soulThreshold)
	{
		initializeSoulPosition(startingIndex, dyingBody);
	}
	if (!alreadyHit)
	{
		SDL_RenderCopy(ren, soulTex, NULL, &soulNotes[startingIndex]);
	}
	else
	{

	}
}
//----------------------------------------------SCORE TRACKING-----------------------------------------------
void scoringCheck(int soulIndex)
{
		if(soulIndex == 0&&playerStrum1 == true)
		{
			timesHit++;
			if (soulSpawnRate <= 0.2)
			{
				soulSpawnRate = 0.2;
			}
			else
			{
				soulSpawnRate -= 0.05;
			}
			combo++;
			if (combo > 0 && combo % minimumFever == 0)
			{
				for (int i = 0; i < 3; i++)
				{
					if (!laserStock[i])
					{
						laserStock[i] = 1; 
						break;
					}
				}
			}
			cout << "good job x" <<timesHit << endl;
			alreadyHit = true;
			playerStrum1 = false;
			SDL_RenderCopy(ren, hitTex, NULL, &notifiers[soulIndex]);
		}
		else if(soulIndex==1&&playerStrum2==true)
		{
			timesHit++;
			if (soulSpawnRate <= 0.2)
			{
				soulSpawnRate = 0.2;
			}
			else
			{
				soulSpawnRate -= 0.05;
			}
			combo++;
			if (combo > 0 && combo % minimumFever == 0)
			{
				for (int i = 0; i < 3; i++)
				{
					if (!laserStock[i])
					{
						laserStock[i] = 1; 
						break;
					}
				}
			}
			cout << "good job x" <<timesHit << endl;
			cout << "combo! x" <<combo<<endl;
			alreadyHit = true;
			playerStrum2 = false;
			SDL_RenderCopy(ren, hitTex, NULL, &notifiers[soulIndex]);
		}
		else if (soulIndex==2&playerStrum3==true)
		{
			timesHit++;
			if (soulSpawnRate <= 0.2)
			{
				soulSpawnRate = 0.2;
			}
			else
			{
				soulSpawnRate -= 0.05;
			}
			combo++;
			if (combo > 0 && combo % minimumFever == 0)
			{
				for (int i = 0; i < 3; i++)
				{
					if (!laserStock[i])
					{
						laserStock[i] = 1; 
						break;
					}
				}
			}
			cout << "good job x" <<timesHit << endl;
			cout << "combo! x" <<combo<<endl;
			alreadyHit = true;
			playerStrum3 = false;
			SDL_RenderCopy(ren, hitTex, NULL, &notifiers[soulIndex]);
		}
}
//-------------------------------------------------------------------FEVER STATUS-------------------------------------------------------------------
void comboFever()
{
	stringstream ss;
	ss<<"Combo! x"<< combo;
	if (combo >= minimumFever)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = feverBarWidth;
		feverBar.h = 40;
		fever = true;
	}
	else if (combo >= minimumFever*0.75)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = feverBarWidth*0.75;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= minimumFever*0.5)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = feverBarWidth*0.5;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= minimumFever*0.25)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = feverBarWidth*0.25;
	}
	if(combo >= 5)
	{
		comboTex = renderTEXTure(ss.str(), black);
		SDL_RenderCopy(ren, comboTex, NULL, &comboNotif);
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

//SHOOTER GAME METHODS
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
			if ((int)r <= 50)
			{
				customObstacles[(j*10)+i] = 1;
			}
		}
	}
	
	//cout << (int)r << " " << (int)g << " " << (int)b <<endl;

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
					//cout << "PROBLEMATIC SOUL INDEX: "<< i << " " << obstacles.size() << endl;
					problematicSoul = true;
				}
			}
			for (int j = 0; j < 4; j++)
			{
				if (SDL_HasIntersection(&soulTestRect, &walls[j]) == SDL_TRUE)
				{
					//cout << "PROBLEMATIC SOUL INDEX: "<< i << " " << obstacles.size() << endl;
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
				//cout << "PROBLEMATIC SOUL INDEX: "<< "new spawn " << obstacles.size() << endl;
				problematicSoul = true;
			}
		}
		for (int j = 0; j < 4; j++)
		{
			if (SDL_HasIntersection(&soulTestRect, &walls[j]) == SDL_TRUE)
			{
				//cout << "PROBLEMATIC SOUL INDEX: "<< "new spawn" << " " << obstacles.size() << endl;
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

//-------------------------------------------MAIN LOOP-----------------------------
void playGame()
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

	initObstacles(playerSelectedSong);
	loadWalls();
	initSouls();	


	bool running = true;

	double songStartTime = playMusic();
	initializeSequence();
	initializeDyingBodies();
	initializeLanes();
	initializeNotifiers();
	initializeButtonPress();
	bool scoreRecorded = false;
	for (int i = 0; i < numberOfSouls; i++)
	{
		initializeSoulPosition(i,dyingBody);
	}

	float existingSouls = 0.5;
	int gameFrame = 0;

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

	bool laserOn = false;
	bool spacePressed = false;
	int laserDuration = FRAMERATE*3;
	SDL_Rect laserPos = {windowWidth*3/4 - 5, windowHeight/2 - 5, cursorRadius*2 + 10, cursorRadius*2 + 10};
	SDL_Rect laserCharges[3];
	for (int i = 0; i < 3; i++)
	{
		laserCharges[i].x = windowWidth - (i+2)*wallThickness;
		laserCharges[i].y = windowHeight-(wallThickness*2/3);
		laserCharges[i].w = wallThickness;
		laserCharges[i].h = wallThickness*2/3;
	}

	int killCount = 0;

	SDL_Rect soulDisplay = {windowWidth/2, windowHeight-(wallThickness*2/3), wallThickness, wallThickness*2/3};

	/*SDL_Rect rightSide;
		rightSide.x = windowWidth/2;
		rightSide.y = 0;
		rightSide.w = windowWidth/2;
		rightSide.h = windowHeight;
		SDL_RenderSetViewport(ren, &rightSide);*/

	SDL_Rect gunHeatBar;
		gunHeatBar.x = windowWidth - (wallThickness*2/3);
		gunHeatBar.y =  windowHeight-wallThickness/2;
		gunHeatBar.w = wallThickness/2;
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
				if (ev.key.keysym.sym == SDLK_SPACE)
				{
					spacePressed = true;
				}
			}
			else if (ev.type == SDL_MOUSEMOTION)
			{
				if (!laserOn)
				{
					mouseMotionX = ev.motion.xrel*2;
					mouseMotionY = ev.motion.yrel*2;
				}
				else
				{
					mouseMotionX = ev.motion.xrel;
					mouseMotionY = ev.motion.yrel;
				}
				
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

		gameFrame++;

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

		//if gun can shoot
		if (mouseButtonDown && bulletDelay == 0 && !gunLock)
		{
			firingBullets = true;
			bulletDelay = FRAMERATE/5;
			gunHeat += 0.25;
			//cout << gunHeat << endl;
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
				//cout << gunHeat << endl;
			}
			else
			{
				gunHeat -= 0.05;
				//cout << gunHeat << endl;
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

		//laser REMOVE spacepressed=true TO MAKE CONTIUOUS LASER A LA ALL-STAR MODE
		if (spacePressed && !laserOn && (laserStock[0] || laserStock[1] || laserStock[2]))
		{
			spacePressed = false;
			laserOn = true;
			/*for (int i = 2; i >= 0; i--)
			{
				if (laserStock[i])
				{
					laserStock[i] = 0;
					break;
				}
			}*/
		}
		if (laserOn)
		{
			firingBullets = false;
			laserDuration--;
			if (laserDuration <= 0)
			{
				for (int i = 2; i >= 0; i--)
				{
					if (laserStock[i])
					{
						laserStock[i] = 0;
						break;
					}
				}
				laserOn = false;
				laserDuration = FRAMERATE*3;
			}
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
			laserPos.x += mouseMotionX;
			mouseMotionX = 0;			
		}
		if (!cursorCollidedY)
		{
			mousePos.y += mouseMotionY;
			laserPos.y += mouseMotionY;
			mouseMotionY = 0;
		}
		cursorCollidedX = false, cursorCollidedY = false;

		//if(gameFrame % ((FRAMERATE/4) == 0 && souls.size() < maxSouls && fRand() < soulSpawnRate)
		if(gameFrame % (int)((FRAMERATE/4)*(1/soulSpawnRate)) == 0 && souls.size() < maxSouls)
		{
			souls.push_back(spawnNewSoul());
			//cout << souls.size() << endl;
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
				//cout << i << " ";
				markedForDestruction = i;
				//cout << markedForDestruction << " ";
				soulDead = true;
				killCount++;
				//cout << killCount << endl;
				//cout << souls[i].x << " " << souls[i].y << endl << souls[i].y+souls[i].h << " " << souls[i].x+souls[i].w << endl;
			}
			else if (laserOn && !soulKilledThisFrame && collisionDetectedBoxCircle(testSoul, 0, mousePos, cursorRadius + 5))
			{
				soulKilledThisFrame = true;
				//cout << i << " ";
				markedForDestruction = i;
				//cout << markedForDestruction << " ";
				soulDead = true;
				killCount++;
				//cout << killCount << endl;
				//cout << souls[i].x << " " << souls[i].y << endl << souls[i].y+souls[i].h << " " << souls[i].x+souls[i].w << endl;
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
		renderScoreBoard();
		SDL_RenderCopy(ren, textTex, NULL, &scoreBoard);
		SDL_RenderCopy(ren, soulThresholdTex, NULL, &hitZone);
		checkAndRenderButtonPress();
		//check if the note is in the "beatZone", and check if a note has already been hit. if it hasn't already been hit, check if the user hit it
		if(SDL_HasIntersection(&soulNotes[soulSequence[sequenceID]], &hitZone) && !alreadyHit)
		{
			scoringCheck(soulSequence[sequenceID]);
			SDL_RenderCopy(ren, onBeatTex, NULL, &hitZone);
		}
		else
		{
			//if the note is NOT in the beatZone or if the note has already been hit but the user presses the button again, return a miss
			//
			if(playerStrum1 == true)
			{
				timesMissed++;
				if (soulSpawnRate >= 1)
				{
					soulSpawnRate = 1;
				}
				else
				{
					soulSpawnRate += 0.05;
				}
				combo = 0;
				cout<<"miss! x"<<timesMissed<<endl;
				SDL_RenderCopy(ren, missTex, NULL, &notifiers[0]);
				playerStrum1 = false;
			}
			if(playerStrum2 == true)
			{
				timesMissed++;
				if (soulSpawnRate >= 1)
				{
					soulSpawnRate = 1;
				}
				else
				{
					soulSpawnRate += 0.05;
				}
				combo = 0;
				playerStrum2 = false;
				cout<<"miss! x"<<timesMissed<<endl;
				SDL_RenderCopy(ren, missTex, NULL, &notifiers[1]);
			}
			if(playerStrum3 == true)
			{
				timesMissed++;
				if (soulSpawnRate >= 1)
				{
					soulSpawnRate = 1;
				}
				else
				{
					soulSpawnRate += 0.05;
				}
				combo = 0;
				playerStrum3 = false;
				cout<<"miss! x"<<timesMissed<<endl;
				SDL_RenderCopy(ren, missTex, NULL, &notifiers[2]);
			}
		}
		comboFever();
		if(currentSound->isFinished() == false)
		{
			renderSoulFloating(soulSequence[sequenceID]);
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
					if (soulSpawnRate >= 1)
					{
						soulSpawnRate = 1;
					}
					else
					{
						soulSpawnRate += 0.05;
					}
					combo = 0;
					cout<<"miss! x"<<timesMissed<<endl;
					SDL_RenderCopy(ren, missTex, NULL, &notifiers[soulSequence[sequenceID]]);
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
				bool beginGame = false;
			}
		}

		//draw
		//SHOOTING
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
			if (souls[i].vx >= 0)
			{
				SDL_RenderCopy(ren, soulTex, NULL, &soulRect);
			}
			else
			{
				SDL_RenderCopyEx(ren,soulTex, NULL, &soulRect, 0, NULL, SDL_FLIP_HORIZONTAL);
			}
		}

	
		//cursor
		if (laserOn)
		{
			SDL_RenderCopyEx(ren, soulTex, NULL, &laserPos, -gameFrame*3, NULL, SDL_FLIP_NONE);
		}
		if (!gunLock)
		{
			SDL_RenderCopyEx(ren, crosshairTex, NULL, &mousePos, gameFrame, NULL, SDL_FLIP_NONE);
		}
		else
		{
			SDL_RenderCopyEx(ren, gunLockTex, NULL, &mousePos, gameFrame, NULL, SDL_FLIP_NONE);
		}
		//cout <<"x" << mousePos.x << endl;

		//cooldown bar
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

		for (int i = 0; i < 3; i++)
		{
			if (laserStock[i] && laserOn)
			{
				SDL_RenderCopy(ren, gunOverheatTex, NULL, &laserCharges[i]);
			}
			else if (laserStock[i] && !laserOn)
			{
				SDL_RenderCopy(ren, soulTex, NULL, &laserCharges[i]);
			}
			else
			{
				SDL_RenderCopy(ren, groundTex, NULL, &laserCharges[i]);
			}
		}

		//DISPLAY NUMBER OF SOULS REMAINING using soulDisplay Rect hahahahah #rekt
		/*stringstream strm;
	strm << timesHit;
	text_surface=TTF_RenderText_Solid(font, strm.str().c_str() ,black);

	textTex = SDL_CreateTextureFromSurface( ren, text_surface );
	SDL_FreeSurface( text_surface );*/
		stringstream target;
		target << souls.size();
		soulsToKill = renderTEXTure(target.str(), white);
		SDL_RenderCopy(ren, soulsToKill, NULL, &soulDisplay);

		SDL_RenderPresent(ren);

		//delay
		if ((FRAME_TIME - (SDL_GetTicks() - frameTimeStart)) > 0)
		{
			SDL_Delay(FRAME_TIME - (SDL_GetTicks() - frameTimeStart));
		}
	}
}

//http://upshots.org/actionscript/jsas-understanding-easing
double easeIn(int framesPassed, int duration, double power) {
	return pow(((double)framesPassed / (double)duration), power);

}

double easeOut(int framesPassed, int duration, double power) {
	return 1 - pow(1 - ((double)framesPassed / (double)duration), power);
}

//http://gizma.com/easing/#quint3
double easeInOut(int framesPassed, int duration, double power) {
	double percentage = ((double)framesPassed / ((double)duration/2));
	if (percentage < 1)
	{
		return pow(percentage, power)/2;
	}
	percentage -= 2;
	int weirdFactor = pow(-1, power+1);
	return ((pow(percentage, power)+(2*weirdFactor))/2*weirdFactor);
}

int main(int argc, char* argv[]) 
{
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
	
	SDL_Rect firstOption = {windowWidth/3, windowHeight/2, windowWidth/3, 100};
	SDL_Rect secondOption = {windowWidth + windowWidth/3, firstOption.y, firstOption.w, firstOption.h};
	SDL_Rect thirdOption = {2*windowWidth + windowWidth/3, firstOption.y, firstOption.w, firstOption.h};
	SDL_Rect fourthOption = {3*windowWidth + windowWidth/3, firstOption.y, firstOption.w, firstOption.h};
	int frame = 0;
	bool rightPressed = false;
	bool movingRight = false;
	bool leftPressed = false;
	bool movingLeft = false;
	int frameBegun = 0;
	int firstStartPosition = firstOption.x;
	int secondStartPosition = secondOption.x;
	int thirdStartPosition = thirdOption.x;
	int fourthStartPosition = fourthOption.x;
	int easeDuration = 30;
	int easeDistance = windowWidth;

	while (startScreen)	
	{	
		frame++;
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
					//playerSelectedSong = 1;
				}
				if (ev.key.keysym.sym == SDLK_2)
				{
					//playerSelectedSong = 2;
				}
				if (ev.key.keysym.sym == SDLK_3)
				{
					//playerSelectedSong = 3;
				}
				if (ev.key.keysym.sym == SDLK_4)
				{
					//playerSelectedSong = 4;
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
				if (ev.key.keysym.sym == SDLK_RIGHT)
				{
					if (!(movingLeft || movingRight) && playerSelectedSong < 4)
					{
						frameBegun = frame;
						rightPressed = true;
					}					
				}
				if (ev.key.keysym.sym == SDLK_LEFT)
				{
					if (!(movingLeft || movingRight) && playerSelectedSong > 1)
					{
						frameBegun = frame;
						leftPressed = true;
					}					
				}
			}			
		}

		if (leftPressed && !(movingLeft || movingRight))
		{	
			movingRight = true;
			leftPressed = false;
			playerSelectedSong--;
		}
		if (movingRight)
		{
			firstOption.x = firstStartPosition + easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			secondOption.x = secondStartPosition + easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			thirdOption.x = thirdStartPosition + easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			fourthOption.x = fourthStartPosition + easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			if (frame-frameBegun == easeDuration)
			{
				firstStartPosition = firstOption.x;
				secondStartPosition = secondOption.x;
				thirdStartPosition = thirdOption.x;
				fourthStartPosition = fourthOption.x;
				movingRight = false;
			}
		}

		if (rightPressed && !(movingLeft || movingRight))
		{	
			movingLeft = true;
			rightPressed = false;
			playerSelectedSong++;
		}
		if (movingLeft)

		{
			firstOption.x = firstStartPosition - easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			secondOption.x = secondStartPosition - easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			thirdOption.x = thirdStartPosition - easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			fourthOption.x = fourthStartPosition - easeInOut(frame-frameBegun, easeDuration, 4)*(easeDistance);
			if (frame-frameBegun == easeDuration)
			{
				firstStartPosition = firstOption.x;
				secondStartPosition = secondOption.x;
				thirdStartPosition = thirdOption.x;
				fourthStartPosition = fourthOption.x;
				movingLeft = false;
			}

		}

		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, startScreenTex, NULL, &startScreenRect);

		SDL_RenderCopy(ren, soulTex, NULL, &firstOption);
		SDL_RenderCopy(ren, dyingBodyTex, NULL, &secondOption);
		SDL_RenderCopy(ren, onBeatTex, NULL, &thirdOption);
		SDL_RenderCopy(ren, feverBarTex, NULL, &fourthOption);

		SDL_RenderPresent(ren);
	}
	if(beginGame)
	{
		playGame();
	}
	
	
	close();
	return 0;
}