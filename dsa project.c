#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TIME 144 // 144 slots (10 minutes each in 24 hours)

typedef struct Flight {
    char flightID[10];
    char type[10]; // "landing" / "takeoff" / "emergency"
    int priority;  // emergency=3, landing=2, takeoff=1
    int time;      // time in minutes from start of day
    struct Flight *next;
} Flight;

typedef struct FlightQueue {
    Flight *front;
    Flight *rear;
} FlightQueue;

typedef struct FlightHistory {
    Flight *flight;
    struct FlightHistory *next;
} FlightHistory;

typedef struct Runway {
    int runwayID;
    Flight *currentFlight;
} Runway;

// Queue functions
FlightQueue* createQueue() {
    FlightQueue *q = (FlightQueue*)malloc(sizeof(FlightQueue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(FlightQueue *q, Flight *f) {
    f->next = NULL;
    if (!q->rear) { q->front = q->rear = f; }
    else { q->rear->next = f; q->rear = f; }
}

Flight* dequeue(FlightQueue *q) {
    if (!q->front) return NULL;
    Flight *temp = q->front;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    temp->next = NULL;
    return temp;
}

int isEmptyQueue(FlightQueue *q) {
    return q->front == NULL;
}

// Stack functions for processed flights
FlightHistory* pushHistory(FlightHistory *top, Flight *f) {
    FlightHistory *node = (FlightHistory*)malloc(sizeof(FlightHistory));
    node->flight = f;
    node->next = top;
    return node;
}

void displayHistory(FlightHistory *top) {
    if(!top) { printf("No processed flights.\n"); return; }
    FlightHistory *curr = top;
    printf("\nProcessed Flights:\n");
    while(curr) {
        printf("FlightID: %s, Type: %s, Priority: %d, Time: %d\n",
               curr->flight->flightID, curr->flight->type, curr->flight->priority, curr->flight->time);
        curr = curr->next;
    }
}

// Create flight
Flight* createFlight() {
    Flight *f = (Flight*)malloc(sizeof(Flight));
    printf("Enter Flight ID: "); scanf("%s", f->flightID);
    printf("Enter type (landing/takeoff/emergency): "); scanf("%s", f->type);
    if (strcmp(f->type,"emergency")==0) f->priority=3;
    else if(strcmp(f->type,"landing")==0) f->priority=2;
    else f->priority=1;
    printf("Enter time (in minutes from start of day 0-1439): "); scanf("%d", &f->time);
    f->next=NULL;
    return f;
}

// Display pending queue
void displayQueue(FlightQueue *q) {
    if(isEmptyQueue(q)) { printf("No pending flights.\n"); return; }
    Flight *curr = q->front;
    printf("\nPending Flights:\n");
    while(curr) {
        printf("FlightID: %s, Type: %s, Priority: %d, Time: %d\n",
               curr->flightID, curr->type, curr->priority, curr->time);
        curr = curr->next;
    }
}

// Assign flights to runways (priority scheduling) and update graph
void assignFlights(FlightQueue *q, Runway runways[], int numRunways, FlightHistory **history, char graph[][MAX_TIME][10]) {
    int i;
    FlightQueue *tempQueue = createQueue();
    while(!isEmptyQueue(q)) {
        // Find highest priority flight
        Flight *prev=NULL, *curr=q->front, *highest=q->front;
        Flight *prevHighest=NULL;
        while(curr) {
            if(curr->priority > highest->priority ||
               (curr->priority==highest->priority && curr->time < highest->time)) {
                highest=curr;
                prevHighest=prev;
            }
            prev=curr;
            curr=curr->next;
        }
        // Remove highest from queue
        if(prevHighest) prevHighest->next=highest->next;
        else q->front=highest->next;
        if(highest==q->rear) q->rear=prevHighest;
        highest->next=NULL;

        // Assign to first available runway
        int assigned=0;
        for(i=0;i<numRunways;i++) {
            if(!runways[i].currentFlight) {
                runways[i].currentFlight=highest;
                printf("Assigned Flight %s to Runway %d at time %d\n", highest->flightID, runways[i].runwayID, highest->time);
                *history = pushHistory(*history, highest);

                // Update graph
                int startSlot = highest->time / 10;
                int durationSlots = 3; // assume 30 min per flight
                for(int t=startSlot; t<startSlot+durationSlots && t<MAX_TIME; t++)
                    strcpy(graph[i][t], highest->flightID);

                assigned=1;
                break;
            }
        }
        if(!assigned) { // all runways busy, put back into tempQueue
            enqueue(tempQueue, highest);
        }
    }
    while(!isEmptyQueue(tempQueue)) enqueue(q,dequeue(tempQueue));
    free(tempQueue);
}

// Display runway usage graph
void displayRunwayGraph(Runway runways[], int numRunways, char graph[][MAX_TIME][10]) {
    printf("\nRunway Usage Graph (10-min intervals):\n");
    for(int i=0;i<numRunways;i++) {
        printf("Runway %d: ", runways[i].runwayID);
        for(int t=0;t<MAX_TIME;t++) {
            if(strlen(graph[i][t])>0)
                printf("|%s", graph[i][t]);
            else
                printf("|    ");
        }
        printf("|\n");
    }
}

int main() {
    FlightQueue *pending = createQueue();
    FlightHistory *history = NULL;
    int numRunways=2; // Example: 2 runways
    Runway runways[2]={{1,NULL},{2,NULL}};
    char runwayGraph[2][MAX_TIME][10]; // text-based runway graph
    int choice;

    // Initialize graph
    for(int i=0;i<numRunways;i++)
        for(int j=0;j<MAX_TIME;j++) runwayGraph[i][j][0]='\0';

    do {
        printf("\nAirport Runway Management System\n");
        printf("1. Add Flight\n2. Display Pending Flights\n3. Assign Flights to Runways\n4. Display Processed Flights\n5. Display Runway Usage Graph\n6. Exit\nChoice: ");
        scanf("%d",&choice);
        switch(choice) {
            case 1: {
                Flight *f = createFlight();
                enqueue(pending,f);
                break;
            }
            case 2: displayQueue(pending); break;
            case 3: assignFlights(pending, runways, numRunways, &history, runwayGraph); break;
            case 4: displayHistory(history); break;
            case 5: displayRunwayGraph(runways, numRunways, runwayGraph); break;
            case 6: printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while(choice!=6);

    return 0;
}
