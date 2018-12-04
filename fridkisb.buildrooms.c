/***************************************************************************
** Program name: fridkisb.buildrooms.c
** Class name: CS344
** Author: Ben Fridkis
** Date: 10/10/2018
** Description: This program generates the basis of an "adventure"-esque
**				game by creating a directory 7 of "rooms", each with 3-6
**				connections to other "rooms". The connections are assigned
**				randomly. One room is given a "type" of START_ROOM, one a
**				type of END_ROOM, and all other rooms are given a type of
**				MID_ROOM. These type assignments are also random. The 7 
**				rooms are choosen at random from a total possibility of 10
**				rooms available for selection.
****************************************************************************/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

//Function to pick 7 of 10 hard-coded "rooms".
int* pickRooms(){
	
	//Generate array of 7 integers in range
	//of 0 - 9 randomly, with no repeated
	//integers.
	static int roomPicker[7];
	memset(roomPicker, -1, sizeof(roomPicker));
	int i;
	for(i = 0; i < 10; i++){
		int random = rand() % 10;
		int j;
		for(j = 0; j < 7; j++){
			if(random == roomPicker[j]){
				if(random == 9){
					random = 1;
					j = -1;
				}
				else{
					random++;
					j = -1;
				}
			}
		}
		roomPicker[i] = random;
	}

	return roomPicker;
}

//Function to create room files.
FILE** createRooms(char** roomFileNames, char** roomNames, int* roomAssignment){
	
	//Create and open 7 files to correspond to randomly
	//generated room numbers (in roomPicker array).
	//Write the first line (ROOM NAME: <room name>) to each.
	static FILE* roomFiles[7];
	int i;
	for(i = 0; i < 7; i++){
		if((roomFiles[i] = (FILE*)(fopen(roomFileNames[roomAssignment[i]], "w+"))) == NULL){
			perror("Unable to open room file.");
			exit(EXIT_FAILURE);
		}
		fprintf(roomFiles[i], "ROOM NAME: %s\n", roomNames[roomAssignment[i]]);
		
		//Rewind each file stream for future use.
		//Alternate: rewind(roomFiles[i]);
		fseek(roomFiles[i], 0L, SEEK_SET);
	}
	return roomFiles;
}

//Function to randomly assign room connections.
void loadConnections(char **roomNames, int* roomAssignment,	FILE** roomFiles){

	//This array will hold the number of connections established
	//for each room.
	int connectionCount[] = {0, 0, 0, 0, 0, 0, 0};

	int i;
	for(i = 0; i < 7; i++){
		
		//This string will hold the file contents of 
		//each room file as its connections are processed/added,
		//which is at maximum 157 characters (before adding room type).
		char fileContents[158];
		memset(fileContents, '\0', sizeof(fileContents));

		//Rewind file position pointer of current room
		rewind(roomFiles[i]);
		//Read current room file contents into fileContent string.
		fread(fileContents, 158, 1, roomFiles[i]);

		//Generate random number of room connections
		//in range 3 - 6 (inclusive).
		//(cc is short for 'connection count'.)
		int cc = rand() % 4 + 3;
		//While this file's connection count is less than the

		//randomly determined number of connections (between 3 - 6),
		//add connections.
		while(connectionCount[i] < cc){
			//Generate random room.
			int rr = rand() % 7;
			//Make sure randomly selected room isn't already a connection for
			//the current room, and if so move through rooms until an unused
			//(i.e. not yet connected) room is found.
			while(strstr(fileContents, roomNames[roomAssignment[rr]]) != NULL){
				if(rr == 6){
					rr = 0;
				}
				else{
					rr++;
				}
			}
			//Place file pointer at end of file.
			fseek(roomFiles[i], 0L, SEEK_END);
			
			//Add (forward) room connection, from current room to randomly selected
			//room, and increment number of connections for current room.
			fprintf(roomFiles[i], "CONNECTION %d: %s\n", ++connectionCount[i], 
				roomNames[roomAssignment[rr]]);
			
			//Update the fileContent string to include the new connection.
			snprintf(fileContents + strlen(fileContents), 158 - strlen(fileContents), 
				"CONNECTION %d: %s\n", connectionCount[i], roomNames[roomAssignment[rr]]);
			
			//Place file pointer at end of file for the room just added.
			fseek(roomFiles[rr], 0L, SEEK_END);
		
			//Add (backward) room connection, from randomly selected room to
			//current room, and increment number of connections for randomly 
			//selected room.
			fprintf(roomFiles[rr], "CONNECTION %d: %s\n",
				++connectionCount[rr], roomNames[roomAssignment[i]]);
		}
	}
}

//Function to assign room types.
void assignRT(FILE** roomFiles){
	
	fprintf(roomFiles[0], "ROOM TYPE: START_ROOM");
	fprintf(roomFiles[1], "ROOM TYPE: END_ROOM");
	
	int i;
	for(i = 2; i < 7; i++){
		fprintf(roomFiles[i], "ROOM TYPE: MID_ROOM");
	}
}

int main(int argc, char* argv[]){
	
	//Seed the random number generator
	srand(time(NULL));

	//Establish directory name string, with extra 5 bytes
	//for process id.
	char roomsDir[20] = "fridkisb.rooms.";
	
	//This function gets the current process ID
	//and stores it in the special data type pid_t
	pid_t mypid = getpid();
	
	//Need a 6 byte char array to store PID,
	//which will will be a max of 5 integers.
	//(Sixth byte needed for null-terminator.)
	char pid[6];

	//Convert mypid to string, store in pid.
	snprintf(pid, 5, "%d", (int)mypid);

	//Concatenate pid to end of roomsDir string.
	strcat(roomsDir, pid);

	//Make directory with string roomsDir as directory name.
	//Exit if there is an error.
	if(mkdir(roomsDir, 0755) != 0){
		perror("Rooms directory could not be created.");
		exit(EXIT_FAILURE);
	}

	//Create an array of 10 strings, 11 characters each.
	//These will store the names of the 10 filenames that
	//will potentially store the room data, each
	//with a maximum (name) length of 10 characters, which
	//will then be followed by '_room'.
	char* roomFileNames[10];
	int i;
	for (i = 0; i < 10; i++){
		roomFileNames[i] = malloc(16 * sizeof(char));
	}

	//Hard code names of each room file
	strcpy(roomFileNames[0], "CORN_SYRUP_room");
	strcpy(roomFileNames[1], "CANE_JUICE_room");
	strcpy(roomFileNames[2], "DEXTROSE_room");
	strcpy(roomFileNames[3], "FRUCTOSE_room");
	strcpy(roomFileNames[4], "GLUCOSE_room");
	strcpy(roomFileNames[5], "LACTOSE_room");
	strcpy(roomFileNames[6], "MALTOSE_room");
	strcpy(roomFileNames[7], "MOLASSES_room");
	strcpy(roomFileNames[8], "RAW_SUGAR_room");
	strcpy(roomFileNames[9], "SUCROSE_room");

	//Repeat the above process for the room names themselves,
	//this time limiting names to 8 characters or less
	char* roomNames[10];
	for (i = 0; i < 10; i++){
		roomNames[i] = malloc(9 * sizeof(char));
	}

	//Hard code names of each room
	strcpy(roomNames[0], "CORN_SYR");
	strcpy(roomNames[1], "CANE_JUI");
	strcpy(roomNames[2], "DEXTROSE");
	strcpy(roomNames[3], "FRUCTOSE");
	strcpy(roomNames[4], "GLUCOSE");
	strcpy(roomNames[5], "LACTOSE");
	strcpy(roomNames[6], "MALTOSE");
	strcpy(roomNames[7], "MOLASSES");
	strcpy(roomNames[8], "RAW_SUGA");
	strcpy(roomNames[9], "SUCROSE");

	int* roomAssignment = pickRooms();

	//Change directory to roomsDir
	chdir(roomsDir);

	//Create room files and pass pointer to open handlers
	FILE** roomFiles = createRooms(roomFileNames, roomNames, roomAssignment);
	//Load room connections
	loadConnections(roomNames, roomAssignment, roomFiles);
	//Assign room types
	assignRT(roomFiles);

	//Free memory
	for(i = 0; i < 10; i++){
		free(roomFileNames[i]);
		free(roomNames[i]);
	}

	//Close files
	for(i = 0; i < 7; i++){
		fclose(roomFiles[i]);
	}

	return 0;
}

/************************************************************************************************************
 * References *
 * https://stackoverflow.com/questions/190229/where-is-the-itoa-function-in-linux
 * https://stackoverflow.com/questions/822323/how-to-generate-a-random-int-in-c
 * https://www.geeksforgeeks.org/generating-random-number-range-c/
 * https://stackoverflow.com/questions/1293660/is-there-any-way-to-change-directory-using-c-language
 * https://en.cppreference.com/w/c/io/fread
 * https://stackoverflow.com/questions/12784766/check-substring-exists-in-a-string-in-c
 * https://gist.github.com/lesovsky/d32a984f97cfcb991c54b27b5d6d3e0d
 * "C Programming Language". Kernighan, Brian and Dennis Ritchie. 2nd Edition. Pearson Education, Inc. 1988.
*************************************************************************************************************/
