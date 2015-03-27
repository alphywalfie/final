#This is Hell.

Your goal, as Death, is to create enough space in the underworld for the Lord of the Underworld's new carousel before the song finishes. 

To do this, obliterate souls from the underworld(right screen) and prevent new souls from entering by stopping death in the real world(left screen).

This game is split-screen and requires multi-tasking. On the left side of the screen is a rhythm game. On the right side of the screen is a shooter.

##Building the Game

###Additional Libraries

This project requires three libraries aside from SDL, SDL_image, SDL_TTF and irrKlang. 

To include these in your projects:

1)  Link to the "include" folders found in the attached zip files. Select your project's properties, and under "C/C++" >> "General", edit "Additional Include Directories" to add the "include" directories for irrKlang, SDL_TTF and SDL_image.

2) Link to the "lib" folders in the attached zip files. Under the project properties, for "Linker>>General", edit "Additional Library Directories" to add the "lib" directories for irrKlang, SDL_TTF and SDL_image.

3) Go to "Linker>>Input" and edit "Additional Dependencies". Add "SDL2_image.lib", "irrKlang.lib" and "SDL2_ttf.lib"

###Game Media and File Dependencies

Make sure to include all the files found in "media" and "text files and dependencies" in your project folder. (The folder in your solution where the "".vcxproj" file is located)

##Playing the Game - Controls

Click on souls in the underworld to obliterate them. There are obstacles in place where your mouse cannot pass through and thus you will have to move around them.

Souls will regenerate at a regular rate.

Your performance in the rhythm game will affect the rate of soul regeneration in the underworld. Press 1, 2 or 3 for the notes when they reach the top bar. The more times you miss, the faster souls will regenerate in the underworld.

There is a bar at the bottom that fills up until you reach "fever mode". Reaching fever mode grants you a laser in the underworld. Activate this laser using space bar.

To win the game, there must be 50 or less souls in the underworld by the time the song ends.

##Customization

To edit the position of the obstacles, edit "custom level.bmp". Black dots are the obstacles. Don't touch the red dot in the middle.

To include your own song in the game, put the audio file in the project folder, find the BPM of your song, and edit "gameSettings.txt" Be sure not to change the line spacings.