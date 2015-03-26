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

//CONSTANTS FOR WINDOW AND FPS
SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
const int windowWidth = 1000;//1000
const int viewPortWidth = windowWidth/2;
const int windowHeight = 600;//600
const int FRAMERATE = 60;
const double FRAME_TIME = 1000/FRAMERATE;

//CONSTANTS FOR THE GAME'S GRAPHICS
const int numberOfSouls = 3;
const double soulThreshold = 0; //(this is the maximum height the soul can float to before it's a miss)
const double dyingBody = 400; //(this is the bottom or starting position of a soul)
const double soulHeight = 70;
const double soulWidth = 80;
const int maxNotes = 500;
const int soulDist = dyingBody - soulThreshold; //number of pixels
const int beatDist = soulDist/4; //distance a soul travels PER BEAT
//vector where the souls are stored
SDL_Rect souls[numberOfSouls];
//rectangle for when you should hit the notes
SDL_Rect hitZone = {0, soulThreshold, viewPortWidth, soulHeight};
SDL_Rect feverBar;
SDL_Rect dyingBodies[numberOfSouls];
SDL_Rect lanes[numberOfSouls];

//CONSTANTS FOR THE GAME'S MUSIC
ISoundEngine* engine = createIrrKlangDevice();
ISound* currentSound = 0;
double bpm;// = 114.9;//121.37 Livin on a Prayer //114.9 Highway to Hell //104.63 The Kill
double crotchet;// = 60/bpm; //beat duration in seconds
string songFilename;// = "AC-DC - Highway to Hell.mp3";

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
//-------------------------------------------------------------------------------------------------------------------------------------
//TEXTURE DECLARATIONS
SDL_Texture *soulTex = NULL;
SDL_Texture *dyingBodyTex = NULL;
SDL_Texture *soulThresholdTex = NULL;
SDL_Texture *onBeatTex = NULL;
SDL_Texture *feverBarTex = NULL;
SDL_Texture *laneTex = NULL;
SDL_Texture *currentLaneTex = NULL;
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
//----------------------------------------------------INITIALIZE SDL--------------------------------------------------------------------
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
//-----------------------------------------------------------LOAD MEDIA------------------------------------------------------
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

	//Mix_FreeMusic(music);
	//Mix_CloseAudio();
	engine->drop();
	currentSound->drop();

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
	souls[i].x = ((i*2+1)*viewPortWidth)/(numberOfSouls*2)-soulWidth/2;
	souls[i].y = startingPosition;
	souls[i].w = soulWidth;
	souls[i].h = soulHeight;
}
//------------------------------------------------------INITIALIZE THE DYING BODIES--------------------------------------------
void initializeDyingBodies()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		dyingBodies[i].x = ((1+i*2)*viewPortWidth)/(numberOfSouls*2)-(soulWidth/2);
		dyingBodies[i].y = dyingBody;
		dyingBodies[i].w = soulWidth;
		dyingBodies[i].h = soulHeight;
	}
}
//----------------------------------------------------INITIALIZE LANES-------------------------------------------------------------
void initializeLanes()
{
	for (int i = 0; i < numberOfSouls; i++)
	{
		lanes[i].x = i*(viewPortWidth/numberOfSouls);
		lanes[i].y = soulThreshold + soulHeight;
		lanes[i].w = viewPortWidth/3;
		lanes[i].h = windowHeight - soulHeight;
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
	souls[startingIndex].y -= soulThreshold + (songTime/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 2
	//souls[startingIndex].y -= soulThreshold + (currentPlayheadPosition/(FRAME_TIME*sequenceID*soulDist) + pixelsPerFrame); //sync equation version 3
	if (souls[startingIndex].y + souls[startingIndex].h <= soulThreshold)
	{
		initializeSoulPosition(startingIndex, dyingBody);
	}
	SDL_RenderCopy(ren, soulTex, NULL, &souls[startingIndex]);
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
	if (combo >= 20)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth;
		feverBar.h = 40;
		fever = true;
	}
	else if (combo >= 15)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth*0.75;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= 10)
	{
		feverBar.x = 0;
		feverBar.y = windowHeight-40;
		feverBar.w = viewPortWidth*0.5;
		feverBar.h = 40;
		fever = false;
	}
	else if (combo >= 5)
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
//-------------------------------------------MAIN LOOP-----------------------------

int main(int argc, char* argv[]) {
	
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
	//************************************************USER GENERATED CONTENT AND HIGH SCORE STORAGE****************************************************
	loadFile("gameSettings.txt", userSettings);
	loadFile("highScore.txt", savedHighScore);
	songFilename = userSettings[3];
	bpm = stod(userSettings[6]);//121.37 Livin on a Prayer //114.9 Highway to Hell //104.63 The Kill
	crotchet = 60/bpm; //beat duration in seconds
	msCrotchet = crotchet*1000; //time duration of a beat in ms
	framesPerBeat = msCrotchet/FRAME_TIME;
	pixelsPerFrame = soulDist/framesPerBeat;
	double highScore = stoi(savedHighScore[1]);
	double highestComboScore = stoi(savedHighScore[6]);
	string playerNameScore = savedHighScore[3];
	string playerNameCombo = savedHighScore[8];
	//**************************************************************************************************************************************************
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
		if(SDL_HasIntersection(&souls[soulSequence[sequenceID]], &hitZone) && !alreadyHit)
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
			if(souls[soulSequence[sequenceID]].y == dyingBody && sequenceID < sequenceCount)
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
			if(scoreRecorded == false && souls[soulSequence[sequenceID]].y == dyingBody)
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