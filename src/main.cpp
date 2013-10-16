/*
 * main.cpp
 *
 *  Created on: 14.10.2013
 *      Author: helleste
 */

#include <iostream>
#include <string.h>
#include <stack>
using namespace std;

// Deklarace stavu
typedef struct {
	int id;
	int path[4];
	int val;
} state;

// Globalni promenne
int distances[4][4] = {{0,1,3,5},{1,0,4,2},{3,4,0,3},{5,2,3,0}};
int bestLength = 1<<16;
int currentLength = 0;
int bestRoute[5] = {0,0,0,0,0};
// Pocatecni stav
state start = {0,{-1,-1,-1,-1},0};

// Fronta stavu k vyrizeni
state queue[4];
stack <state> jobs;

void toQueue(state inputState) {
	// TODO implementovat metodu toQueue
	jobs.push(inputState);
}

int calcVal(state startState, state destState) {

	return distances[startState.id][destState.id];
}

// Zjisti, jestli jsme jiz prosli zadanym stavem
bool isInPath(state currentState, int state) {

	for (int i = 0; i < sizeof(currentState.path)/sizeof(int); i++) {
		if (currentState.path[i] == state)
			return true;
	}

	return false;
}

// Vloz do cesty jiz projity stav
void toPath(state &currentState, state parentState) {

	// Zkopirujem si cestu rodice
	memcpy(currentState.path, parentState.path, 4*sizeof(int));
	// Pridame rodicovske id na konec
	for (int i = 0; i < sizeof(currentState.path)/sizeof(int); i++) {
		if (currentState.path[i] == -1) {
			currentState.path[i] = parentState.id;
			return;
		}
	}
}

// Vypis strukturu stavu
void printState(state currentState) {

	cout << "State ID: " << currentState.id << " Path:";
	for (int i = 0; i < sizeof(currentState.path)/sizeof(int); i++) {
		cout << " " << currentState.path[i];
	}
	cout << " Val: " << currentState.val << endl;
}

// Vypis nejlepsi vzdalenost a okruh
void printBest() {

	cout << "BestLength: " << bestLength << " Best route:";
	for (int j = 0; j < sizeof(bestRoute)/sizeof(int); j++) {
			cout << " " << bestRoute[j];
		}
	cout << endl;
}

// Najdi potomky zadaneho stavu
void findSuccessors(state parentState) {

	state currentState = {0,{-1,-1,-1,-1},0};
	jobs.pop();
	int flag = 0; // Indikace nalezeni potomka

	for(int i = 3; i >= 0 ; i--) {
		if (i != parentState.id && !isInPath(parentState, i) && i != start.id) {
			flag = 1;
			currentState.id = i;
			toPath(currentState, parentState);
			printState(currentState);
			currentState.val = parentState.val + calcVal(parentState, currentState);
			toQueue(currentState);
		}
	}

	// Si na konci. Dopocitej cestu do startu.
	if (flag == 0) {
		if ((parentState.val + distances[parentState.id][start.id]) < bestLength) {
			bestLength = parentState.val + distances[parentState.id][start.id];
			memcpy(bestRoute, parentState.path, 4*sizeof(int));
			// Vloz predposledni mesto do nejlepsi cesty
			bestRoute[sizeof(bestRoute)/sizeof(int) - 2] = parentState.id;
			printBest();
		}
	}
}

int main() {
	jobs.push(start);

	while(jobs.size() != 0) {
		cout << "Zpracovavam stav: " << jobs.top().id << endl;
		findSuccessors(jobs.top());
	}

	cout << "Computation finished!" << endl;
	printBest();
	return 0;
}
