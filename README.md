# SimpleSearchEngine

## Introduction
A search engine (like Google) has three main components: a crawler that finds and stores copies of files on the web, an indexer that creates a data structure that is efficient for searching, and a query engine that performs the searches requested by users. The query engine uses the indexes produced by the second component to identify documents that match the search term.

The goal of this project is to implement a simple parallel query engine. For simplicity let's assume that the indexes have already been created. An index simply tracks word frequency in each document.

If Google used a program with a single process to query all of the indexes to find the best matches for a search term, it would take a very long time to get the results. In this project, I have written a parallel program using fork and pipes to identify documents that match a search term.

## Index Files
You won't be writing any of the code that builds the indexes themselves. The program uses `read_list` to load an index from `index` and `filenames` into memory. Words and their frequencies are stored in an ordered linked list. The picture below shows what the linked list looks like.



Each list node contains three elements: the word, an array that stores the number of times the word has been seen in each file, and a pointer to the next element of the list. Another data structure (an array of strings) stores the name of each file that is indexed. The index of a file name corresponds to the index of the freq array in a list node. Storing the file names separately means that we don't need to store the name of each file many times.

In the diagram above, four words have been extracted from two files. The two files are called Menu1 and Menu2. The linked list shows that the word spinach appears 2 times in the file Menu1 and 6 times in the file Menu2. Similarly, the word potato appears 11 times in the file Menu1 and 3 times in Menu2. The words in the linked list are in alphabetical order.

The program `indexer` builds the linked list data structure and the string array with the file names and will store them in two files named `index` and `filenames`, respectively. The indexer program is implemented in the file indexer.c and will be automatically compiled by the provided Makefile. When run without arguments indexer will index all the files in the current working directory. It also supports a -d command line option that can be used to specify a directory where indexer should look for files to index (instead of using the current working directory).

The file index that stores the linked list data structure is in binary format, so you will not able to look at the file contents and read them. The program called `printindex` (implemented in printindex.c) looks for two files named `index` and `filenames`, respectively, builds the index data structure based on these two files and prints the data structure in human readable format. The output of printindex will be one line for each word found in the text files covered by the index, followed by the frequency counts for this word for each of the text files.

The function `get_word` returns an array of FreqRecords for the word that the function looks up in the index. Each individual FreqRecord provides the number of occurrences of the word for a particular file. The array only contains FreqRecords for files that have at least one occurrence of the word. The definition of a FreqRecord struct is shown below and is included in worker.h.

The `run_worker` function takes as an argument the name of the directory that contains the index files it will use. It also takes as arguments the file descriptors representing the read end (in) and the write end (out) of the pipe that connects it to the parent.

`run_worker` first loads the index into a data structure. It then reads one word at a time from the file descriptor in until the file descriptor is closed. When it reads a word from in, it looks for the word in the index, and write to the file descriptor out one FreqRecord for each file in which the word has a non-zero frequency. The reason for writing the full struct is to make each write a fixed size, so that each read can also be a fixed size.

The program in queryone.c  calls `run_worker` so that run_worker will read from stdin and write to stdout.

The program called `query` takes one optional argument giving the name of a directory. If no argument is given, the current working directory is used. (The main function is in a file called query.c).

The number of subdirectories of the command line argument (or of the current directory, if a commandline argument is not provided) determines the number of processes created.

Each worker process is connected to the master process by two pipes, one for writing data to the master, and one for reading data from the master. The worker processes carry out the same operations as the `run_worker`. The master process reads one word at a time from standard input, sends the word to each of the worker processes, and reads the FreqRecords that the workers write to the master process. The master process collects all the FreqRecords from the workers by reading one record at a time from each worker process, and builds one array of FreqRecords, ordered by frequency. It prints this array to standard output, and then waits for the user to type another word on standard input. It continues until standard input is closed (using ^D on the command line). When standard input is closed, it closes the pipes to all the workers, and exits.

## Details
Here is a high-level overview of the algorithm `query`:

* Initialization
* Create one process for each subdirectory (of either the current working directory or the directory passed as a command line argument to the program)
* while(1)
  * read a word from stdin (it is okay to prompt the user)
  * using pipes, write the word to each worker process
  * while(workers still have data)
    * read one FreqRecord from each worker and add it to the master frequency array
  * print to standard output the frequency array in order from highest to lowest
* The master process will not terminate until all of the worker processes have terminated.

### Reading and writing the pipes:

* The master process will be writing words to the child processes. How does the child process know when one word ends and the next word begins? The easiest way to solve this problem is to always write and read the same number of bytes. In other words, when the master process writes a word to a child, it should always write the same number of bytes. You can assume that a word is no bigger than 32 characters (see MAXWORD in freq_list.h). So the master process will write 32 bytes (including the null termination character), and the child process will always read 32 bytes.
* The FreqRecords have a fixed size, so the master process knows how to read one record at a time from a worker. The worker process notifies the master that it has no more records to send by sending a FreqRecord with a value of 0 for the freq field, and an empty string for the filename field.
* The master process will read one record at a time from each worker, so you need to keep track of the case when the master has read all of the records from a worker.

