#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

using namespace std;

struct User {
    int id;
    int group;
    int access_position;
    int arrival_time;
    int usage;
};

const int NUM_OF_RECORDS = 10;

int CURRENT_GROUP = 0;

int CURRENT_GROUP_TOTAL_COUNT = 0;
int CURRENT_GROUP_COMPLETED = 0;

int GROUP_TOTAL_COUNT[2] = {0};

int DB_LOCKS[10] = {0};

pthread_mutex_t mutex_db = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_group = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t *cond_db;
pthread_cond_t cond_group = PTHREAD_COND_INITIALIZER;

bool change = true;

int DB_WAIT_COUNT = 0;
int GROUP_WAIT_COUNT = 0;

void change_current_group(){
    if (change) {
        pthread_mutex_lock(&mutex_group);
        if (CURRENT_GROUP_COMPLETED == CURRENT_GROUP_TOTAL_COUNT){
            printf("\nAll users from Group %d finished their execution\n", CURRENT_GROUP);
            CURRENT_GROUP = (CURRENT_GROUP == 1) ? 2 : 1;
            CURRENT_GROUP_COMPLETED = 0;
            CURRENT_GROUP_TOTAL_COUNT = GROUP_TOTAL_COUNT[CURRENT_GROUP-1];
            change = false;
            printf("The users from Group %d start their execution\n\n", CURRENT_GROUP);
            pthread_cond_broadcast(&cond_group);
        }
        pthread_mutex_unlock(&mutex_group);
    }
}
void* request_thread(void* arg){
    User * user_ptr = (User*)arg;

    // Arrival message
    printf("User %d from Group %d arrives to the DBMS\n", user_ptr->id, user_ptr->group);

    //  Check Current Group
    pthread_mutex_lock(&mutex_group);
    if (CURRENT_GROUP != user_ptr->group){
        printf("User %d is waiting due to its group\n", user_ptr->id);
        GROUP_WAIT_COUNT++;
        pthread_cond_wait(&cond_group, &mutex_group);
    }
    pthread_mutex_unlock(&mutex_group);

    // If the position is free, lock for current process else wait
    pthread_mutex_lock(&mutex_db);
    if (DB_LOCKS[user_ptr->access_position-1] != 0){
        printf("User %d is waiting: position %d of the database is being used by user %d\n", user_ptr->id, user_ptr->access_position, DB_LOCKS[user_ptr->access_position-1]);
        DB_WAIT_COUNT++;
        pthread_cond_wait(&cond_db[user_ptr->access_position-1], &mutex_db);
    }
    DB_LOCKS[user_ptr->access_position-1] = user_ptr->id;
    pthread_mutex_unlock(&mutex_db);

    // Access required position
    printf("User %d is accessing the position %d of the database for %d second(s)\n", user_ptr->id, user_ptr->access_position, user_ptr->usage);
    sleep(user_ptr->usage);
    printf("User %d finished its execution\n", user_ptr->id);

    // Release db lock and signal others waiting in queue
    pthread_mutex_lock(&mutex_db);

    DB_LOCKS[user_ptr->access_position-1] = 0;
    pthread_cond_signal(&cond_db[user_ptr->access_position-1]);
    CURRENT_GROUP_COMPLETED++;
    //Update Current Group
    change_current_group();

    pthread_mutex_unlock(&mutex_db);

    return NULL;
}

int main() {
    int NUM_OF_USERS = 0;
    // To store input data
    vector <User> user_queue;
    User temp;

    // Condition variables for each record
    cond_db = new pthread_cond_t[NUM_OF_RECORDS];

    cin >> CURRENT_GROUP;

    // Input Data
    while (cin >> temp.group >> temp.access_position >> temp.arrival_time >> temp.usage) {
        temp.id = NUM_OF_USERS + 1;
        user_queue.push_back(temp);

        GROUP_TOTAL_COUNT[temp.group-1]++;
        NUM_OF_USERS++;
    }
    pthread_t tid[NUM_OF_USERS];
    CURRENT_GROUP_TOTAL_COUNT = GROUP_TOTAL_COUNT[CURRENT_GROUP-1];
    // check current group
    change_current_group();
    // Request thread Creation
    for (int i = 0; i < NUM_OF_USERS; i++) {
        sleep(user_queue[i].arrival_time);
        if (int response_code = pthread_create(&tid[i], NULL, request_thread, (void*)&user_queue[i])) {
            cout << "ERROR: THREAD CREATION FAILED\n";
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < NUM_OF_USERS; i++) {
        pthread_join(tid[i], NULL);
    }
    printf("\nTotal Requests:\n\tGroup 1: %d\n\tGroup 2: %d\n", GROUP_TOTAL_COUNT[0], GROUP_TOTAL_COUNT[1]);
    printf("\nRequests that waited:\n\tDue to its group: %d\n\tDue to a locked position: %d\n", GROUP_WAIT_COUNT, DB_WAIT_COUNT);

    return 0;
}