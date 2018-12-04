					fridkisb.adventure
					
	This program is an "adventure"-esque game that uses a
	directory of "room" files (see fridkisb.buildrooms.c) to
	establish a "maze" through which a player is to navigate.
	A player starts in the "START_ROOM", and moves through the
	connected rooms until reaching the "END_ROOM". The number
	of "steps" (i.e. moves) and the path traversed is displayed
	back to the user on successful completion. The user may
	enter the "time" command to get the system time during
	gameplay, which utilizes a separate thread to return the
	system time (blocking the main thread while doing so).
	(The time command will also store the time in a file called 
	"currentTime.txt".)
	
Getting Started:

1.  Copy all repository files to a directory of your choice.

2.  Type the command "compile" (without quotes)

3.  Type "fridkisb.adventure" (without quotes) to launch the program.

