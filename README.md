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
