#Life of the Party (pls think of a better death pun game title)

Your goal, as Death, is to create enough space in the underworld for the Lord of the Underworld's new carousel before the song finishes. 

To do this, obliterate souls from the underworld(right screen) and prevent new souls from entering by stopping death in the real world(left screen).

This game is split-screen and requires multi-tasking. On the left side of the screen is a rhythm game. On the right side of the screen is a shooter.

##Building the Game

###Additional Libraries

This project requires two libraries aside from SDL, SDL_image and irrKlang. 

To include these in your projects:

1)  Link to the "include" folders found in the attached zip files. Select your project's properties, and under "C/C++" >> "General", edit "Additional Include Directories" to add the "include" directories for irrKlang and SDL_image.

2) Link to the "lib" folders in the attached zip files. Under the project properties, for "Linker>>General", edit "Additional Library Directories" to add the "lib" directories for irrKlang and SDL_image.

3) Go to "Linker>>Input" and edit "Additional Dependencies". Add "SDL2_image.lib" and "irrKlang.lib".

###Game Media and File Dependencies

Make sure to include all the files found in "media" and "text files and dependencies" in your project folder. (The folder in your solution where the "".vcxproj" file is located)

##Playing the Game - Controls

Click on souls in the underworld to obliterate them. There are obstacles in place where your mouse cannot pass through and thus you will have to move around them.

Souls will regenerate at a regular rate.

Your performance in the rhythm game will affect the rate of soul regeneration in the underworld. Press 1, 2 or 3 for the notes when they reach the top bar. The more times you miss, the faster souls will regenerate in the underworld.

To win the game, there needs to be a certain amount of space free in the underworld by the time the song is over.

##Customization

To edit the position of the obstacles, edit ___________________.

To include your own song in the game, put the audio file in the music folder, find the BPM of your song, and edit "gameSettings.txt" Be sure not to change the line spacings.