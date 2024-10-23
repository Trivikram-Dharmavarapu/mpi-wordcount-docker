Word and Character Count Parallelization with MPI and Docker

Overview
--------
This project implements a parallelized solution to count the occurrences of characters and words in a file using MPI. 
The solution divides the workload among processes, aggregates results, and outputs the top 10 most frequent words and characters.
The project can be run using native processes or Docker containers with MPI support.

Files in the Submission
-----------------------
- parcount_td23z.cpp: The main source code file containing the implementation of the word and character counting program.
- Makefile: Contains instructions for building and running the program using MPI.
- analysis.pdf: A report analyzing the performance, complexity, and execution results for various test cases.

Extracting a .tgz Archive
-------------------------
- To unzip or extract a .tgz file, use the following command:

    tar -xzvf td23z_proj1.tgz

Prerequisites
-------------
To compile and run this project, you need the following:
1. MPI installed on your system (e.g., OpenMPI).
2. A working Docker environment if you want to test it in a containerized environment.

Makefile Details
----------------
The Makefile provided simplifies the process of compiling and running the MPI-based program.
Below are the details of the Makefile and its associated commands:

Variables
---------
- MPIRUN: This is set to `mpirun`, which is the MPI execution command.
- MPICC: This is set to `mpic++`, which is the MPI C++ compiler.

Targets
-------
1. run: 
   - This target executes the compiled program using `mpirun`. It assumes that the program is already compiled and executable.
        - Command:
            make run ARGS="-np <num_processes> ./parcount ./test1.txt"

        - Example:
            make run ARGS="-np 4 ./parcount ./test1.txt"

            Here, '-np 4' means to run the program with 4 processes, and './test1.txt' is the input file.

2. parcount:
   - This target compiles the program from the source file `parcount_td23z.cpp` using the `mpic++` compiler.
        - Command:
            make parcount

3. clean:
   - This target removes the generated executable `parcount.out` and the compiled binary from the `parcount_td23z` directory.
        - Command:
            make clean

How to Build and Run the Project
--------------------------------
1. Build the Program
    To compile the program, use the following command:

    make parcount
    
    This will generate the executable 'parcount' from the source file 'parcount_td23z.cpp'.

2. Run the Program with MPI
    To run the program, use the following command:
    make run ARGS="-np <num_processes> ./parcount <input_file>"

    - Replace '<num_processes>' with the number of parallel processes you want to use (e.g., 4 or 8).
    - Replace '<input_file>' with the path to the input text file (e.g., './test1.txt').

    Example:

        make run ARGS="-np 4 ./parcount ./test1.txt"
    
    This will execute the program with 4 parallel processes.

3. Clean Up

    To clean up the compiled files and binary executables, use the following command:

    make clean

    This will remove any existing compiled files.

Running in a Docker Environment
-------------------------------
To run this program inside a Docker container with MPI support:
1. Set up Docker containers with MPI installed.
2. Once the Docker container is set up, you can use the same 'make' commands to compile and run the program within the container.

Output Format
-------------
The program will print the following:
1. Top 10 Most Frequent Characters:
    - The character frequencies are displayed in descending order.
    - Tie-breaking is based on ASCII values.

2. Top 10 Most Frequent Words:
    - The word frequencies are displayed in descending order.
    - Tie-breaking is based on the first occurrence(ID) of the word in the input file.

Analysis
--------
For more details on the performance analysis, trends, and complexity, refer to the "analysis.pdf" file provided with the submission.
