#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
#define MAX_EVENTS_PER_CUSTOMER 200

/* Shared resource state */
int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int global_event = 0;

/* Per-customer event queue (parsed from stdin before threads start) */
typedef struct {
    char op;
    int v[NUMBER_OF_RESOURCES];
} Event;

Event customer_events[NUMBER_OF_CUSTOMERS][MAX_EVENTS_PER_CUSTOMER];
int customer_event_count[NUMBER_OF_CUSTOMERS];

/* Safety algorithm — must be called while mutex is held */
static int is_safe(void) {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        work[i] = available[i];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        finish[i] = 0;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (finish[i])
                continue;
            int ok = 1;
            for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                if (need[i][j] > work[j]) { ok = 0; break; }
            }
            if (ok) {
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                    work[j] += allocation[i][j];
                finish[i] = 1;
                changed = 1;
            }
        }
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        if (!finish[i]) return 0;
    return 1;
}

/* Must be called while mutex is held */
int request_resources(int customer_num, int request[]) {
    /* Check request <= need */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        if (request[i] > need[customer_num][i]) return 0;

    /* Check request <= available */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        if (request[i] > available[i]) return 0;

    /* Tentatively allocate */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i]              -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i]     -= request[i];
    }

    /* Safety check */
    if (!is_safe()) {
        /* Rollback */
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i]              += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i]     += request[i];
        }
        return 0;
    }
    return 1;
}

/* Must be called while mutex is held */
int release_resources(int customer_num, int release[]) {
    /* Validate release <= allocation */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        if (release[i] > allocation[customer_num][i]) return 0;

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i]              += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i]     += release[i];
    }
    return 1;
}

void *customer_thread(void *arg) {
    int cid = *(int *)arg;

    for (int i = 0; i < customer_event_count[cid]; i++) {
        Event *e = &customer_events[cid][i];

        pthread_mutex_lock(&mutex);

        int ok = (e->op == 'R')
                 ? request_resources(cid, e->v)
                 : release_resources(cid, e->v);

        /* Print log while mutex is still held */
        printf("E=%d C=%d OP=%c V=[%d,%d,%d] OK=%d AVAIL=[%d,%d,%d] ALLOC=[%d,%d,%d] NEED=[%d,%d,%d].\n",
               global_event, cid, e->op,
               e->v[0], e->v[1], e->v[2],
               ok,
               available[0], available[1], available[2],
               allocation[cid][0], allocation[cid][1], allocation[cid][2],
               need[cid][0], need[cid][1], need[cid][2]);

        global_event++;

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void) {
    /* Read available resources */
    scanf("%d %d %d", &available[0], &available[1], &available[2]);

    /* Read maximum matrix and initialise need / allocation */
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        scanf("%d %d %d", &maximum[i][0], &maximum[i][1], &maximum[i][2]);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            allocation[i][j] = 0;
            need[i][j]       = maximum[i][j];
        }
    }

    /* Parse all events from stdin before threads start */
    memset(customer_event_count, 0, sizeof(customer_event_count));
    {
        int cid, v0, v1, v2;
        char op;
        while (scanf("%d %c %d %d %d", &cid, &op, &v0, &v1, &v2) == 5) {
            int idx = customer_event_count[cid]++;
            customer_events[cid][idx].op   = op;
            customer_events[cid][idx].v[0] = v0;
            customer_events[cid][idx].v[1] = v1;
            customer_events[cid][idx].v[2] = v2;
        }
    }

    /* Launch one thread per customer */
    pthread_t threads[NUMBER_OF_CUSTOMERS];
    int ids[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, customer_thread, &ids[i]);
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&mutex);
    return 0;
}
