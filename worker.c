#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

/* The function get_word should be added to this file */
FreqRecord *get_word(const char *word, Node *head, char **filenames){

	Node *index = head;
	int i, j = 0;
	FreqRecord *new;
	
	while (index != NULL && (strcmp(word, index->word) != 0)) {
		index = index->next;
	}
	
	if (index == NULL) {
		//word doesn't exist in the index; allocate only one FreqRecord array
		new = malloc(sizeof(FreqRecord));
		if (new == 0) {
			perror("malloc");
			exit(1);
		}
		new[0].freq=0;
		return new;
	}

	new = malloc(sizeof(FreqRecord)*MAXFILES+1);
	if (new == 0) {
		perror("malloc");
		exit(1);
	}
	
	for(i = 0; i < MAXFILES && filenames[i] != NULL; i++) {
		if (index->freq[i] != 0) {
			//non-zero frequency, so add it in the array of FreqRecord
			new[j].freq = index->freq[i];
			strncpy(new[j].filename, filenames[i], PATHLENGTH);
			j++;
		}
	}
	new[j].freq = 0;
	return new;
}


/* Print to standard output the frequency records for a word.
* Used for testing.
*/
void print_freq_records(FreqRecord *frp) {
	int i = 0;
	while(frp != NULL && frp[i].freq != 0) {
		printf("%d    %s\n", frp[i].freq, frp[i].filename);
		i++;
	}
}

/* run_worker
* - load the index found in dirname
* - read a word from the file descriptor "in"
* - find the word in the index list
* - write the frequency records to the file descriptor "out"
*/
void run_worker(char *dirname, int in, int out){
	
	char *listfile, *namefile, buf[MAXWORD];
	Node *head = NULL;
	char **filenames = init_filenames();
	int i, r;

	if ((listfile = malloc(sizeof(char)*PATHLENGTH)) == 0) {
		perror("malloc");
		exit(1);
	}
	strncat(listfile, dirname, PATHLENGTH);
	strncat(listfile, "/index", PATHLENGTH);

	if ((namefile = malloc(sizeof(char)*PATHLENGTH)) == 0) {
		perror("malloc");
		exit(1);
	}
	strncat(namefile, dirname, PATHLENGTH);
	strncat(namefile, "/filenames", PATHLENGTH);

	//load the index
	read_list(listfile, namefile, &head, filenames);

	if ((r = read(in, buf, MAXWORD)) < 0) {
 		perror("read");
		exit(1); 	
	}
	
	//while EOF is not reached
	while (r != 0) {
		buf[r-1] = '\0';
		
		FreqRecord *new = get_word(buf, head, filenames);
		for (i = 0; new[i].freq != 0; i++) {
			//write out one FreqRecord at a time to out
			if (write(out, &new[i], sizeof(FreqRecord)) < 0) {
				perror("write");
				exit(1);
			}
		}
		if (write(out, &new[i], sizeof(FreqRecord)) < 0) {
			perror("write");
			exit(1);
		}
		if ((r = read(in, buf, MAXWORD)) < 0) {
			perror("read");
			exit(1);
		}
	}
	close(in); close(out);
}
