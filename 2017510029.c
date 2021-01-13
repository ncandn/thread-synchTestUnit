#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_PATIENT 24
#define UNIT_NUMBER 8

/* Function prototypes. */
void *patient(void *number);
void *staff(void *number);
void randwait(int secs);

/* Global variables. */
int allDone = 0; // Loop flag to iterate until all patients are sent home.
int unit_counter = 0; // Store index to operate on a desired unit.
int room_status; // Get the value of no. of people in a waiting room.
int displayid[3]; // Store patient ID's in a waiting room - display purposes.
int displayindex = 0; // Index counter for the var. above ^.

/* Semaphore declarations. */
sem_t waitingRoom[UNIT_NUMBER]; // Waiting room cap.
sem_t healthStaff[UNIT_NUMBER]; // Test condition for staff.
sem_t ventilation[UNIT_NUMBER]; // 0- Ventilation; 1- No Ventilation.
sem_t onebyone; // Arbitrary waiting room constraint for patients not to rush into the room all together.

int main(int argc, char *argv[])
{
	/* Thread initialize. */
    pthread_t utid[UNIT_NUMBER];
    pthread_t ptid[MAX_PATIENT];
    int patient_id[MAX_PATIENT];
    int staff_id[UNIT_NUMBER];
    
	// Grant each patient an ID.
    for (int i = 0; i < MAX_PATIENT; i++) {
        patient_id[i] = i;
    }
    // Grant each staff and ID.
    for (int i = 0; i < UNIT_NUMBER; i++) {
        staff_id[i] = i;
    }
    
    // Initialize the semaphores.
    for (int i = 0; i < UNIT_NUMBER; i++){
		sem_init(&waitingRoom[i], 0, 3);
		sem_init(&healthStaff[i], 0, 0);
		sem_init(&ventilation[i], 0, 0);
	}
	sem_init(&onebyone, 0, 1);
	
    // Create unit threads.
    for (int i = 0; i < UNIT_NUMBER; i++){
		pthread_create(&utid[i], NULL, staff, (void *)&staff_id[i]);
	}

    // Create patient threads.
    for (int i = 0; i < MAX_PATIENT; i++) {
        pthread_create(&ptid[i], NULL, patient, (void *)&patient_id[i]);
    }

    // Join each patient thread.
    for (int i = 0; i < MAX_PATIENT; i++) {
        pthread_join(ptid[i],NULL);
    }
    
    // All patients are sent home after the loop above ^.
	allDone = 1;
	
	// Let the staff ventilate at the end of the day.
	for (int i = 0; i < UNIT_NUMBER; i++) {
		sem_post(&ventilation[i]);
	}
	
	// Join each unit thread.
	for (int i = 0; i < UNIT_NUMBER; i++) {
		pthread_join(utid[i],NULL);
	}
	
	// Destroy semaphores afterwards.
	for (int i = 0; i < UNIT_NUMBER; i++) {
		sem_destroy(&waitingRoom[i]);
		sem_destroy(&healthStaff[i]);
		sem_destroy(&ventilation[i]);
	}
	sem_destroy(&onebyone);
	
	printf("!!! No patients left.\n");
    system("PAUSE");   

    return 0;
}

void *patient(void *number) {
	
    int num = *(int *)number;
    randwait(rand()%4 + 1);
    printf(">>> Patient %d has arrived at the hospital.\n", num);
    sem_wait(&onebyone); // No other patient can walk into a waiting room besides this one.
    displayid[displayindex] = num; // Get the ID.
    displayindex++;
    if(displayindex >= 3){ // Reset array if the room gets full.
		displayindex = 0;
	}
    sem_wait(&waitingRoom[unit_counter]); // Decrement room availability.
    printf(">>> Patient %d is entering the unit %d waiting room.\n", num, unit_counter);
    printf(">>> Patient %d is filling the form for preperation.\n", num);
    sem_post(&ventilation[unit_counter]); // Staff stops ventilating.
    sem_wait(&healthStaff[unit_counter]); // Staff prepares for the test.
    printf("<<< Patient %d is leaving from the hospital.\n", num);
}

void *staff(void *number)
{
	int num = *(int *)number; 
	while (!allDone) {
		randwait(rand()%1 + 1);
		sem_wait(&ventilation[num]); 
		sem_getvalue(&waitingRoom[unit_counter], &room_status); // Get the waiting room availability.
		
		if (!allDone) {
			
			if(room_status == 2){ // 2 remaining for a test.
				printf("*** Covid-19 Test Unit %d waiting room:\n", num);
				printf("\t[X(%d)][-][-]\n",displayid[0]);
				sem_post(&onebyone); // Remove the constraint.
				printf("!!! Last 2 people for Covid-19 test! Please pay attention to social distancing and hygiene, use a mask!\n");
			}
			else if(room_status == 1){ // 1 remaining for a test.
				printf("*** Covid-19 Test Unit %d waiting room:\n", num);
				printf("\t[X(%d)][X(%d)][-]\n",displayid[0],displayid[1]);
				sem_post(&onebyone); // Remove the constraint.
				printf("!!! 1 people remaining for Covid-19 test! Please pay attention to social distancing and hygiene, use a mask!\n");
			}
			else if(room_status == 0){ // Test begins.
				unit_counter++; // Increment the index since this room is full.
				if(unit_counter >= 8){ // If exceed 8, reset.
					unit_counter = 0;
				}
				printf("*** Covid-19 Test Unit %d waiting room:\n", num);
				printf("\t[X(%d)][X(%d)][X(%d)]\n",displayid[0],displayid[1],displayid[2]);
				sem_post(&onebyone); // Remove the constraint.
				printf(">>> Staff is applying Covid-19 test on patients in unit %d.\n",num);
				randwait(rand()%3 + 1);
				/* Empty out the waiting room and free the staff. */
				for (int j = 0; j < 3; j++){
					sem_post(&waitingRoom[num]);
					sem_post(&healthStaff[num]);
				}
				printf("*** The staff is ventilating the unit %d.\n",num);	
			}
			
		}
		else {
			break;
		}
	}
}

void randwait(int secs) {
     sleep(secs);
}
