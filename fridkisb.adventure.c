/****************************************************************************
** Program name: fridkisb.adventure.c
** Class name: CS344
** Author: Ben Fridkis
** Date: 10/11/2018
** Description: This program is an "adventure"-esque game that uses a
**				directory of "room" files (see fridkisb.buildrooms.c) to
**				establish a "maze" through which a player is to navigate.
**				A player starts in the "START_ROOM", and moves through the
**				connected rooms until reaching the "END_ROOM". The number
**				of "steps" (i.e. moves) and the path traversed is displayed
**				back to the user on successful completion. The user may
**				enter the "time" command to get the system time during
**				gameplay, which utilizes a separate thread to return the
**				system time (blocking the main thread while doing so).
****************************************************************************/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>

//The following typedef and 3 functions are used to create
//a dynamic array of strings. The code is relies heavily on this post:
//https://stackoverflow.com/questions/3536153/c-dynamically-growing-array
typedef struct {
	char** path;
	size_t used;
	size_t size;
} Path;

void initPath(Path* p, size_t initSize, size_t wordSize) {
	
	if((p->path = (char**)malloc(initSize * sizeof(char*))) == NULL){
		perror("Error allocating memory for dynamic array");
		exit(EXIT_FAILURE);
	}
	int i;
	for(i = 0; i < (int)initSize; i++){
		if(((p->path)[i] = malloc((wordSize + 1) * sizeof(char))) == NULL){
			perror("Error allocating memory for dynamic array");
			exit(EXIT_FAILURE);
		}
	}
	p->used = 0;
	p->size = initSize;
}

void insertPath(Path* p, char* room, size_t wordSize){
	
	int i = (int)p->used;
	if(p->used == p->size){
		p->size *= 2;
		if((p->path = (char**)realloc(p->path, p->size * sizeof(char*))) == NULL){
			perror("Error allocating memory for dynamic array");
			exit(EXIT_FAILURE);
		}
		for(i = (int)p->used; i < (int)p->size; i++){
			if(((p->path)[i] = malloc((wordSize + 1) * sizeof(char))) == NULL){
				perror("Error allocating memory for dynamic array");
				exit(EXIT_FAILURE);
			}
		}
	}
	strcpy((p->path)[(int)p->used], room);
	p->used++;
}

void freePath(Path* p){
	
	int i;
	for(i = 0; i < (int)p->size; i++){
		free((p->path)[i]);
	}
	free(p->path);
	p->path = NULL;
	p->used = p->size = 0;
}

//Function to change the process's working directory to that
//which has been most recently modified with a given target prefix.
//Relies heavily on https://oregonstate.instructure.com/courses/1692912/pages/2-dot-4-manipulating-directories
void setDirectory(){
	int newestDirTime = -1;							//Last modified timestamp of newest subdir examined
	char targetDirPrefix[16] = "fridkisb.rooms.";   //Target directory prefix
	static char newestDirName[21];					//Holds name of newest directory with target prefix

	DIR* dirToCheck;								//Holds starting directory
	struct dirent* fileInDir;						//Holds the current subdir/file of starting directory
	struct stat dirAttributes;						//Holds information about the subdir/file (fileInDir)

	dirToCheck = opendir(".");						//Open current directory (i.e. directory from which
													//the program was executed
	
	if (dirToCheck > 0){							//Make sure directory was successfully opened
		while((fileInDir = readdir(dirToCheck)) != NULL){
			if(strstr(fileInDir->d_name, targetDirPrefix) != NULL){
				stat(fileInDir->d_name, &dirAttributes);				//If directory entry contains target prefix, 
																		//get stats (i.e. store in dirAttributes)

				if((int)dirAttributes.st_mtime > newestDirTime){		//If the directory is newer than the previously
					newestDirTime = (int)dirAttributes.st_mtime;		//determined newest, update newest.
					memset(newestDirName, '\0', sizeof(newestDirName));
					strcpy(newestDirName, fileInDir->d_name);
				}
			}
		}
	}
	
	//Close directory opened
	closedir(dirToCheck);						

	//Navigate process to the newest rooms directory
	chdir(newestDirName);
}

typedef struct {
	char name[9];
	char connections[6][9];
	char type;
	int numConnections;
} Room;

Room* loadRooms(){
	
	DIR* roomDIR;
	if((roomDIR = opendir(".")) <= 0){
		perror("Room directory could not be opened.");
		exit(EXIT_FAILURE);
	}
	struct dirent* fileInDir;

	static Room rooms[7];
	int i;
	for(i = 0; i < 7; i++){
		//Determine name of next file in directory and open
		//(Skip hidden files and "." and ".." directory files!)
		do{
			fileInDir = readdir(roomDIR);
		}while(fileInDir->d_name[0] ==  '.');
		FILE* fp;
		if((fp = fopen((char*)fileInDir->d_name, "r")) == NULL){
			printf("Error opening %s\n", fileInDir->d_name);
			exit(EXIT_FAILURE);
		}
		
		int c, cc = 0;
	
		//Determine connection count by counting newlines,
		//then subtracting 1 (as first newline is for room name).
		while((c = fgetc(fp)) != EOF){
			if(c == '\n'){
				cc++;
			}
		}

		rooms[i].numConnections = cc - 1;

		//Reset file pointer to start of room name
		fseek(fp, 11L, SEEK_SET);
		//Load name into Room struct
		int d = 0;
		memset(rooms[i].name, '\0', sizeof(rooms[i].name));
		while((c = fgetc(fp)) != '\n'){
			rooms[i].name[d++] = c;
		}
		
		//Load each connection into Room struct
		int j;
		for(j = 0; j < rooms[i].numConnections; j++){
			fseek(fp, 14L, SEEK_CUR);
			d = 0;
			memset(rooms[i].connections[j], '\0', sizeof(rooms[i].connections[j]));
			while((c = fgetc(fp)) != '\n'){
				rooms[i].connections[j][d++] = c;
			}
		}
		
		//Load room type into Room struct
		fseek(fp, 11L, SEEK_CUR);
		rooms[i].type = (char)fgetc(fp);

		fclose(fp);
	}

	//Close room directory
	closedir(roomDIR);

	//Navigate back up a directory after loading the rooms
	//into memory.
	chdir("..");

	return rooms;
}

//Function to write current time to currentTime.txt file. 
//Creates currentTime.txt file if it does not previously exist.
void* writeTime(void* mutex){
	
	//The main thread will have this locked first, and will
	//only unlock it when the user inputs the "time" command.
	//This will ensure that this function's thread only executes when this
	//command (i.e. the "time" command) is input. The main thread will
	//"wait" for the thread running this function  via the use of pthread_join 
	//and so it in turn will be blocked during its execution.
	pthread_mutex_lock((pthread_mutex_t*)mutex);

	//Open file currentTime.text (and create, if necessary).
	int fd;
	if((fd = open("currentTime.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
		perror("Error opening currentTime.txt");
		exit(EXIT_FAILURE);
	}
	
	//Get time and write to file in specified format.
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char outputTime[41];
	memset(outputTime, '\0', sizeof(outputTime));

	strftime(outputTime, 41, "%I:%M%p, %A, %B %d, %Y", timeinfo);

	//Write each character of the datetime string (outputTime)
	//until the null terminator is encountered.
	int i = 0;
	while(outputTime[i] != '\0'){
		if((write(fd, outputTime + i++, 1) == -1)){
			perror("Error writing to file currentTime.txt");
		}
	}

	if((close(fd)) == -1){
		perror("Error closing currentTime.txt");
	}
	
	//Unlock the mutext
	pthread_mutex_unlock((pthread_mutex_t*)mutex);

	return NULL;
}

int main(int argc, char* argv[]){
	
	//Navigate process to newest rooms directory 
	setDirectory();

	//Load the contents of each room into memory
	Room* rooms = loadRooms();
	
	int cr = 0, er = 0;

	//Establish starting room as current room (cr) and assign
	//end room to er variable
	int i;
	for(i = 0; i < 7; i++){
		if(rooms[i].type == 'S'){
			cr = i;
		}
		if(rooms[i].type == 'E'){
			er = i;
		}
	}
	
	
	//Declare and initialize a dynamic array to hold series of room names,
	//which will be used for the user's path.
	//(Note this could have been implemented as an array of ints but I wanted
	// some extra practice with memory allocation so opted for the slightly more
	// complicated and memory intensive option of using a dynamic array of strings.)
	Path* path = malloc(sizeof(Path));
	initPath(path, 5, 8);
	
	//Use a mutex to block the thread (about to be created)
	//until it is needed.
	pthread_mutex_t lock;
	if((pthread_mutex_init(&lock, NULL) != 0)){
		perror("Failed to establish mutext");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_lock(&lock);		//Lock mutex until thread is needed! (see writeTime())

	//Prompt user to navigate to a current room connection
	//until user reaches end room!
	while(cr != er){
		

		printf("CURRENT LOCATION: %s\n"
			   "POSSIBLE CONNECTIONS: ", rooms[cr].name);
		for(i = 0; i < rooms[cr].numConnections; i++){
			if(i != rooms[cr].numConnections - 1){
				printf("%s ", rooms[cr].connections[i]);
			}
			else{
				printf("%s.\n"
					   "WHERE TO >", rooms[cr].connections[i]);
			}
		}
	
		//Get user input
		char* input = NULL;					//Used for user input via getline function
		size_t len = 0;						//Also used for getline function
		getline(&input, &len, stdin);
		//Strip trailing newline from user input
		input[strlen(input) - 1] = '\0';

		//Check for valid input
		int stepTaken = 0;
		for(i = 0; i < rooms[cr].numConnections; i++){
			//If a valid connection was entered,
			//determine connection room #, assign current room
			//to room entered by user, and update steps taken
			//and path array.
			if(strcmp(input, rooms[cr].connections[i]) == 0){
				int j;
				for(j = 0; j < 7; j++){
					if(strcmp(input, rooms[j].name) == 0){
						insertPath(path, rooms[j].name, 8);	
						cr = j;
						stepTaken++;
						printf("\n\n");
						break;
					}
				}
				break;
			}
		}
		//If user input is 'time', display system time using seperate thread.
		//Existing/main thread is blocked via use of mutex. The secondary thread
		//writes the system time to the file currentTime.txt, and then the main
		//thread reads this information after it resumes execution.
		if(!stepTaken && strcmp(input, "time") == 0){

			//Create a thread to be used for writeTime function, passing mutex as 
			//argument. Thread is locked until main thread (i.e. calling thread, 
			//or "this" thread) unlocks mutex below.
			pthread_t tid;
			if((pthread_create(&tid, NULL, writeTime, &lock)) != 0){
				perror("Error creating thread");
				exit(EXIT_FAILURE);
			}
		
			//Unlocks mutex so tid (secondary thread that runs writeTime)
			//can run.
			pthread_mutex_unlock(&lock);

			//Wait until writeTime thread (tid) returns. (i.e. block the main
			//thread until writeTime thread returns).
			pthread_join(tid, NULL);

			//Relock the mutex
			pthread_mutex_lock(&lock);

			int fd;
			if((fd = open("currentTime.txt", O_RDONLY)) == -1){
				perror("Error opening currentTime.txt");
			}
			char curTime[41];
			memset(curTime, '\0', sizeof(curTime));
			int i = 0, errorDetector = 0;
			while((errorDetector = read(fd, curTime + i++, 1)) > 0){}
			if(errorDetector < 0){
				perror("Cannot read file currentTime.txt");
			}
			printf("\n\n  %s\n\n\n"
				   "WHERE TO? >", curTime);
			
		}
		//Else if input is not a valid room connection, print message to user
		//accordingly.
		else if(!stepTaken){
			printf("\n\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n\n");
		}
		
		//Free user input memory
		free(input);
	}

	//Print congratulations message and game stats (steps taken & path).
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n"
		   "YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", (int)path->used);
	for(i = 0; i < path->used; i++){
		printf("%s\n", path->path[i]);
	}

	//Free memory used to hold user's path.
	freePath(path); 
	free(path); 

	//Kill the mutex
	pthread_mutex_destroy(&lock);

	return 0;
}



/************************************************************************************************************
 * References *
 * https://stackoverflow.com/questions/3536153/c-dynamically-growing-array
 * https://oregonstate.instructure.com/courses/1692912/pages/2-dot-4-manipulating-directories
 * https://www.thegeekstuff.com/2012/05/c-mutex-examples/
 * "C Programming Language". Kernighan, Brian and Dennis Ritchie. 2nd Edition. Pearson Education, Inc. 1988.
 * https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
 * https://linux.die.net/man/3/strftime
************************************************************************************************************/
