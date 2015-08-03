#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

int main(int argc, char **argv) {
	
	char ch;
	char path[PATHLENGTH], buf[MAXWORD], d_paths[MAXPATHS][PATHLENGTH];
	FreqRecord master_array[MAXRECORDS];
	char *startdir = ".";
	pid_t pid;
	int i, j, k, num = 0;

	while((ch = getopt(argc, argv, "d:")) != -1) {
		switch (ch) {
			case 'd':
			startdir = optarg;
			break;
			default:
			fprintf(stderr, "Usage: query [-d DIRECTORY_NAME]\n");
			exit(1);
		}
	}
	// Open the directory provided by the user (or current working directory)
	
	DIR *dirp;
	if((dirp = opendir(startdir)) == NULL) {
		perror("opendir");
		exit(1);
	} 
	
	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the index file contained in the directory.
 	* Note that this implementation of the query engine iterates
	* sequentially through the directories, and will expect to read
	* a word from standard input for each index it checks.
	*/
		
	struct dirent *dp;
	while((dp = readdir(dirp)) != NULL) {

		if(strcmp(dp->d_name, ".") == 0 || 
		   strcmp(dp->d_name, "..") == 0 ||
		   strcmp(dp->d_name, ".svn") == 0){
			continue;
		}
		strncpy(path, startdir, PATHLENGTH);
		strncat(path, "/", PATHLENGTH - strlen(path) - 1);
		strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);

		struct stat sbuf;
		if(stat(path, &sbuf) == -1) {
			//This should only fail if we got the path wrong
			// or we don't have permissions on this entry.
			perror("stat");
			exit(1);
		} 

		if(S_ISDIR(sbuf.st_mode)) {
			//copy the path of each subdirectory to the d_paths array
			strncpy(d_paths[num], path, PATHLENGTH);
			//count the number of subdirectories
			num++;
		}
	}
	
	//create a buffer that will read the FreqRecord from a worker
	FreqRecord *buffer = malloc(sizeof(FreqRecord));
	if (buffer == 0) {
		perror("malloc");
		exit(1);
	}
	
	//two 2D array of pipes to stores all the necessary pipes
	int p1[num][2], p2[num][2];

	for (j = 0; j < num; j++) {

		//create pipes before forking so that master and worker have access to the same pipe
		if (pipe(p1[j]) == -1) {
			perror("pipe");
			exit(1);
		}
		if (pipe(p2[j]) == -1) {
			perror("pipe");
			exit(1);
		}
	}
	
	for (j = 0; j < num; j++) {
				
		pid = fork();
	
		switch (pid) {

			//fork fails
			case -1:
				perror("fork");
        		exit(1);

			//child
			case 0:
				//close the writing end of pipe 1
				if (close(p1[j][1]) != 0) {
					perror("close");
					exit(1);
				}
				//close the reading end of pipe 2
				if (close(p2[j][0]) != 0) {
					perror("close");
					exit(1);
				}
				
				for (k = 0; k < j; k++) {
					//close the writing end of pipe 1
					if (close(p1[k][1]) != 0) {
						perror("close");
						exit(1);
					}
					//close the reading end of pipe 2
					if (close(p2[k][0]) != 0) {
						perror("close");
						exit(1);
					}
				}
				//run worker for each child
				//reads from reading end of pipe1, writes to writing of pipe2
				run_worker(d_paths[j], p1[j][0], p2[j][1]);
				exit(0);

			//parent			
			default:
				//close the reading end of pipe 1
				if (close(p1[j][0]) != 0) {
					perror("close");
					exit(1);
				}
					
				//close the writing end of pipe 2
				if (close(p2[j][1]) != 0) {
					perror("close");
					exit(1);
				}
				break;
		}
	}

	while (1) {
		printf("Enter the word:\n");
		int r, s = 0;
		//if user enters Ctrl-D, break out of the infinite loop
		if ((r = read(STDIN_FILENO, buf, MAXWORD)) == 0) {
			printf("Exiting\n");
			break;
		}
		//otherwise continue the algorithm
		else {
			buf[r-1] = '\0';
			for (i = 0; i < MAXRECORDS; i++) {
				master_array[i].freq = 0;
				strncpy(master_array[i].filename, "", PATHLENGTH); 
			}
			for (i = 0; i < num; i++) {
				//write the word to all workers
				if (write(p1[i][1], buf, MAXWORD) < 0) {
					perror("write");
					exit(1);
				}
			}
			//initialize the master frequency array
			
			for (i = 0; i < num; i++) {
				//read one FreqRecord from each worker
				if (read(p2[i][0], buffer, sizeof(FreqRecord)) < 0) {
					perror("read");
					exit(1);
				}
				
				if(buffer->freq == 0) continue;
				master_array[s].freq = buffer->freq;
				strncpy(master_array[s].filename, buffer->filename, PATHLENGTH);
				s++;

				//while workers still have data
				while (buffer->freq != 0) {
					master_array[s].freq = buffer->freq;
					strncpy(master_array[s].filename, buffer->filename, PATHLENGTH);
					if (read(p2[i][0], buffer, sizeof(FreqRecord)) < 0) {
						perror("read");
						exit(1);
					}
					s++;
				}
			}
			print_freq_records(master_array);
		}
	}

	//close all pipes before exiting
	for (i = 0; i < num; i++) {
		if (close(p1[i][1]) != 0) exit(1);
		if (close(p2[i][0]) != 0) exit(1);
		if (close(p2[i][1]) != 0) exit(1);
		if (close(p1[i][0]) != 0) exit(1);
	}

	for (i = 0; i < num; i++) {
		if (wait(NULL) == -1) {
			perror("wait");
			exit(1);
		}	
	}
	free(buffer);
	return 0;
}
