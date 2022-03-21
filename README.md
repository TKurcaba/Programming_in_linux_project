# Programming_in_linux_project

Program 1: Poszukwiacz

The program takes one argument: a positive integer with an optionally added unit (Ki = 1024, Mi = 10242). It also requires that its standard input be pinned to a pipe file. (The condition must be verified at the start of the operation.)

The run argument specifies the number of 2-byte numbers to be read from standard input. For each number that appears for the first time, the program generates a record of the result which it sends to the standard output. The record is binary and consists of two fields: the number found and the PID of this process.

After completing the search, the program exits with the status informing about the number of repetitions in the data:

0 - no repetitions,
1 - up to 10% (inclusive),
2 - up to 20% (inclusive),
...
If there are other errors (e.g. incorrect calling), the returned status is to be greater than 10.

Program 2: Kolekcjoner
The collector program employs seekers to search your data. Finders of the first copies are honored by entries in the guest book. It also keeps notes on employees and the "quality" of the data.

Launch parameters:

-d <source> - path to the file with data to be downloaded,
-s <volume> - positive integer, with an optionally added unit (Ki = 1024, Mi = 10242),
the amount of data to be downloaded from the source,
-w <block> - positive integer, with an optionally added unit (Ki = 1024, Mi = 10242),
the amount of data to be processed by each child,
-f <sukcesy> - path to the file for recording achievements,
-l <reports> - path to the file with child reports,
-p <prac> - positive integer, maximum number of children.
The and parameters define the number of 2-byte numbers. So an area of ​​2 · (2 · ) bytes. Action

The program creates descendants and, up to a certain point, makes sure that everyone who died is replaced with a new one. (This does not apply if the children exit with an error.) Each child executes the finder program with . The program uses two anonymous pipes to communicate with descendants, one for reading and one for writing. Each child created has standard input and output connected to these pipes.

The main program copies data of 2 · bytes from the file indicated by the parameter to the output pipe. Since such an operation may require multiple approaches, subsequent approaches are to be performed intertwined with the rest of the activities. Other activities are: reading the results provided by descendants and recording their deaths. Both operations are to be performed in a non-blocking mode and may trigger further operations. If both operations have failed, enter a delay of 0.48 seconds before bypassing the cycle again. Reading the results

The process reads one result at a time, i.e. one structure as described in the finder specification. Based on its content, it modifies the achievements file in such a way that in the cell specified with the first value in the structure, it enters the PID number from the second position. Remarks. The contents of the file should be interpreted as an array of cells with sizes corresponding to the pid_t type. Cells that did not have a value entered should contain 0. Once a cell is saved, it cannot be modified anymore. Reading task statuses and new children

The status of the task is to be read as soon as possible after its completion. Until the achievement file is 75% complete, a new one is generated for each completed child. The exception is termination for reasons other than "natural" (status above 10). Information about child-related events is to be recorded in the log file. Each entry is to contain: reading the MONOTONIC clock, the type of event (ending or creation) and the PID of the process it concerns.
