#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

using namespace std;

void printCarDetails(char license[10], int time);
int getDirection(char ch);

class car {
public:
	char license[10];
	int time;
} cars[4][1001];

char directions[][12] = { "Northbound", "Eastbound", "Southbound", "Westbound" };

int carsAvailable[4];
int carsUsed[4];

int main() {
	char license[10];
	char l[25];
	char dir;
	int time, usedCar, startingTime;
	int currentDirection, maximumCars, carsLeft;
	char ch = getchar();
	carsLeft = 0;
	currentDirection = getDirection(ch);
	ch = getchar();
	ch = getchar();
	maximumCars = (int)ch - 48;
	ch = getchar();
	int i;
	while (cin >> license >> dir >> time) {
		i = getDirection(dir);
		strcpy(cars[i][carsAvailable[i]].license, license);
		cars[i][carsAvailable[i]].time = time;
		carsAvailable[i] += 1;
		carsLeft += 1;
	}
	for (; carsLeft > 0;) {
		i = maximumCars;
		startingTime = 1;
		while (i > 0 && carsLeft > 0) {
			if (carsAvailable[currentDirection] > carsUsed[currentDirection]) {
				if (startingTime) {
					cout << "Current Direction: " << directions[currentDirection] << "\n";
					startingTime = 0;
				}

				usedCar = carsUsed[currentDirection]++;
				time = cars[currentDirection][usedCar].time;
				strcpy(license, cars[currentDirection][usedCar].license);

				// Fork is used to create a child process
				pid_t process = fork();
				if (!process) {
					printCarDetails(license, time);
					sleep(time);
					return 0;
				}
				else {
					wait(NULL);
					carsLeft -= 1;
					i -= 1;
				}
			}
			else {
				i = 0;
			}

		}
		sleep(time);
		currentDirection = (currentDirection + 1) % 4;
	}
}

void printCarDetails(char license[10], int time) {
	if (!time) {
		return;
	}
	cout << "Car  " << license << " is using the intersection for " << time
		<< " sec(s).\n";
}

int getDirection(char ch)
{
	if (ch == 'N') return 0;
	if (ch == 'E') return 1;
	if (ch == 'S') return 2;
	else  return 3;
}
