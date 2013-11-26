/*
 * main.cpp
 *
 *  Created on: 14.10.2013
 *      Author: helleste
 */

#include <iostream>
#include <string.h>
#include <stack>
#include <mpi.h>
#include <omp.h>
#include <cstdlib>

#define JOB_REQUEST 0
#define JOB_ASSIGNMENT 1
#define SEND_RESULTS 98
#define RESULTS 99
#define MASTER_PROCESS 0
#define LENGTH 1000

using namespace std;


/* State definiton */
typedef struct {
	int id;
	int path[999];
	int val;
} state;

/* Globals */
/*int cities = 5;
int distances[5][5] = {
		{0,2,3,5,4},
		{2,0,6,3,7},
		{3,6,0,1,4},
		{5,3,1,0,2},
		{4,7,4,2,0}};
int bestRoute[6] = {0,0,0,0,0,0};
state start = {0,{-1,-1,-1,-1,-1},0};*/

int cities = 8;
int distances[8][8] = {
		{0,1,2,3,4,5,6,7},
		{1,0,6,3,7,8,9,5},
		{2,6,0,1,4,7,4,5},
		{3,3,1,0,2,8,2,1},
		{4,7,4,2,0,6,4,2},
		{5,8,7,8,6,0,7,1},
		{6,9,4,2,4,7,0,9},
		{7,5,5,1,2,1,9,1}};
int bestRoute[9] = {0,0,0,0,0,0,0,0,0};
state start = {0,{-1,-1,-1,-1,-1,-1,-1,-1},0};

/*int cities = 10;
int distances[10][10] = {
	{0,2,2,2,2,2,2,2,2,2},
	{2,0,2,2,2,2,2,2,2,2},
	{2,2,0,2,2,2,2,2,2,2},
	{2,2,2,0,2,2,2,2,2,2},
	{2,2,2,2,0,2,2,2,2,2},
	{2,2,2,2,2,0,2,2,2,2},
	{2,2,2,2,2,2,0,2,2,2},
	{2,2,2,2,2,2,2,0,2,2},
	{2,2,2,2,2,2,2,2,0,2},
	{2,2,2,2,2,2,2,2,2,0}};
int bestRoute[11] = {0,0,0,0,0,0,0,0,0,0,0};
state start = {0,{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},0};*/
int bestLength = 1<<16;

/* Stack of states to process */
stack <state> jobs;

int stop = 0;

/* Save the state to the stack to process */
void toQueue(state inputState) {

	#pragma omp critical 
	{
		jobs.push(inputState);
		stop = 0;
		#pragma omp flush(stop)
	}
}

/* Calculate distance between two states */
int calcVal(state startState, state destState) {

	return distances[startState.id][destState.id];
}

/* Find out if we already have this state in our path */
bool isInPath(state currentState, int state) {

	for (int i = 0; i < sizeof(currentState.path)/sizeof(int); i++) {
		if (currentState.path[i] == state)
			return true;
	}

	return false;
}

/* Add state to the path */
void toPath(state &currentState, state parentState) {

	/* Copy parent path */
	memcpy(currentState.path, parentState.path, (cities-1)*sizeof(int));

	/* Add parent's id to the end of the path */
	for (int i = 0; i < sizeof(currentState.path)/sizeof(int); i++) {
		if (currentState.path[i] == -1) {
			currentState.path[i] = parentState.id;
			return;
		}
	}
}

/* Print the structure of the state */
void printState(state currentState) {

	cout << "State ID: " << currentState.id << " Path:";
	for (int i = 0; i < cities; i++) {
		cout << " " << currentState.path[i];
	}
	cout << " Val: " << currentState.val << endl;
}

/* Print the best length and path */
void printBest() {

	cout << "BestLength: " << bestLength << " Best route:";
	for (int j = 0; j < sizeof(bestRoute)/sizeof(int); j++) {
			cout << " " << bestRoute[j];
		}
	cout << endl;
}

/* Find the successors of given state */
void findSuccessors(state parentState) {

	state currentState = {0,{-1,-1,-1,-1,-1,-1,-1,-1},0};
	//jobs.pop();
	int flag = 0; // Indication that successor was find

	for(int i = (cities -1); i >= 0 ; i--) {
		if (i != parentState.id && !isInPath(parentState, i) && i != start.id) {
			flag = 1;
			currentState.id = i;
			toPath(currentState, parentState);
			currentState.val = parentState.val + calcVal(parentState, currentState);
			//printState(currentState);
			toQueue(currentState);
		}
	}

	/* All states are in the path. Calculate the distance to the start */
	if (flag == 0) {
		if ((parentState.val + distances[parentState.id][start.id]) < bestLength) {
			#pragma omp critical 
			{
				bestLength = parentState.val + distances[parentState.id][start.id];
				memcpy(bestRoute, parentState.path, (cities-1)*sizeof(int));
				/* Add next-to-last state to the path */
				bestRoute[sizeof(bestRoute)/sizeof(int) - 2] = parentState.id;
			}
			//printBest();
		}
	}
}

/* Simple custom broadcast function */
void myBcast(int root, int tag) {
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	for (int i = 0; i < world_size; i++) {
		if (i != root) {
			MPI_Send(0, 0, MPI_INT, i, tag, MPI_COMM_WORLD);
		}
	}
}

bool finished(int waiting[]) {
	int sum = 0;

	for (int i = 0; i < 4; i++) {
		sum += waiting[i];
	}

	if(sum == 4) return true;

	return false;
}


int main(int argc, char *argv[]) {
	int rank, size, data;
	int position;
	char buffer[LENGTH];

	MPI_Init(&argc, &argv);
	/* Find the rank of the process and number of processes */
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	/* Set number of threads to 4 */
	omp_set_num_threads(4);

	if (rank == 0)		
	{
		/* Master process */
		MPI_Status status;
		int slave_length;
		int slave_route[cities + 1];
		jobs.push(start);

		/* Generate jobs from the root */
		findSuccessors(jobs.top());
		
		while(jobs.size() > 0) {
			/* Wait for job requests */
			MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, JOB_REQUEST, MPI_COMM_WORLD, &status); // potencialni problem, ze chci jen JOB_REQUEST
			
			/* Send job to the slave */
			MPI_Send(&jobs.top().id, 1, MPI_INT, status.MPI_SOURCE, JOB_ASSIGNMENT, MPI_COMM_WORLD);

			/* Clear assigned job from stack */
			jobs.pop();
		}

		/* Tell all slaves that that you are out of jobs */
		cout << "Computation finished!" << endl;
		myBcast(MASTER_PROCESS, SEND_RESULTS);

		/* Collect results */
		for (int i = 1; i < size; i++) {
			position = 0;
			MPI_Recv(buffer, LENGTH, MPI_PACKED, MPI_ANY_SOURCE, RESULTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Unpack(buffer, LENGTH, &position, slave_route, (cities + 1)*sizeof(int), MPI_INT, MPI_COMM_WORLD);
			MPI_Unpack(buffer, LENGTH, &position, &slave_length, 1, MPI_INT, MPI_COMM_WORLD);

			if (slave_length < bestLength) {
				bestLength = slave_length;
				memcpy(bestRoute, slave_route, (cities + 1) * sizeof(int));
			}
		}

		printBest();

	} else {
		/* Slave process */

		while(1) {
			int city_id;
			int flag = 0;
			int waiting[4] = {0,0,0,0};
			MPI_Status status;
			state curr_state;

			/* Ask for a job */
			MPI_Send(0, 0, MPI_INT, MASTER_PROCESS, JOB_REQUEST, MPI_COMM_WORLD);

			/* Wait for the job */
			MPI_Recv(&city_id, 1, MPI_INT, MASTER_PROCESS, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // tady asi muze nastat deadlock, kdyz master nema JOB_ASSIGNMENT
			//cout << "Process " << rank << " rcvd job, tag: " << status.MPI_TAG << endl;

			/* Check if the job arrived */
			if (status.MPI_TAG == SEND_RESULTS) break;

			/* Reconstruct the state */
			state state_tbd = {city_id, {0,-1,-1,-1,-1,-1,-1,-1}, calcVal(state_tbd, start)};
			jobs.push(state_tbd);

			/* Parallel section */
			#pragma omp parallel shared(jobs, stop, waiting) private(curr_state, flag)
			{
				/* Count the path */

				while(!finished(waiting)) {
					/* Indicate that you are waiting for work */
					waiting[omp_get_thread_num()] = 1;

					while(!stop) {
						flag = 0;

						#pragma omp critical 
						{
							if (jobs.size() > 0 && !stop) {
								curr_state = jobs.top();
								jobs.pop();
								flag = 1;
								waiting[omp_get_thread_num()] = 0;
							} else {
								stop = 1;
								#pragma omp flush(stop)
							}
							
						}
						if(flag) findSuccessors(curr_state);
					}
				}
			}
		}

		/* Send best case to master */
		position = 0;
		MPI_Pack(bestRoute, (cities + 1)*sizeof(int), MPI_INT, buffer, LENGTH, &position, MPI_COMM_WORLD);
		MPI_Pack(&bestLength, 1, MPI_INT, buffer, LENGTH, &position, MPI_COMM_WORLD);
		MPI_Send(buffer, position, MPI_PACKED, MASTER_PROCESS, RESULTS, MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}
