#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#define MAX_STOPS 22757
#define INF 999999

typedef struct {
    int hr;
    int min;
    int sec;
} Time;


typedef struct {
    int bus_id;
    Time *bus_timings;
    int num_timings;
} BusStartTiming;

typedef struct AdjStop {
    int stop_id;
    int* bus_ids;
    int bus_count;
    Time time;
    float fare;
    struct AdjStop* next;
} AdjStop;


typedef struct AdjList {
    AdjStop* head;     
} AdjList;

typedef struct Stop {
    int stop_id;
    float stop_lat;
    float stop_lon;
    char stop_name[50];
} Stop;

typedef struct Graph {
    int num_stops;
    Stop* stops;
    AdjList* adjLists;
    int* stop_id_map;
    BusStartTiming *bus_times;
    int num_buses;

} Graph;

typedef struct {
    int stop_id;               // The stop ID
    int bus_id;                // The bus ID for the current segment
    int total_time;            // The total time taken for the current segment
    int bus_transfer_count;    // The number of bus transfers
    int previous_stop_id;      // The previous stop ID
} MinHeapNode;


typedef struct {
    MinHeapNode* nodes;
    int* positions;
    int size;
    int capacity;
} MinHeap;

int timeToSeconds(Time time);
MinHeap* createMinHeap(int capacity);
void swapNodes(MinHeapNode* a, MinHeapNode* b);
void minHeapify(MinHeap* minHeap, int idx);
MinHeapNode extractMin(MinHeap* minHeap);
void decreaseKey(MinHeap* minHeap, int stop_id, int total_time, int prev_stop_id, int bus_id);
void insertMinHeap(MinHeap* minHeap, MinHeapNode node);
char* dijkstraShortestTime(Graph* graph, int src_stop_id, int dest_stop_id);
char* dijkstraFewestTransfers(Graph* graph, int src_stop_id, int dest_stop_id, int max_transfers);
void debug_memory(void* ptr, const char* msg);
Graph* create_graph(int num_stops,int num_buses);
void addBusTiming(Graph *graph, int bus_id, Time timing);
Time calculate_time_diff(char* time1, char* time2);
void add_bus_id(AdjStop* adj_stop, int bus_id);
void add_edge(Graph* graph, int from_stop_id, int to_stop_id, Time travel_time, float fare, int bus_id);
void load_stops(const char* filename, Graph* graph);
void loadData(Graph *graph, const char *filename);
void load_stop_times(const char* filename, Graph* graph);
void load_fares(const char* filename, Graph* graph);
void display_graph(Graph* graph);
void printBusTimings(Graph *graph);
void free_graph(Graph* graph);
int find_stop_id(const char *stop_name, const char *filename);
char* main_logic(const char *start, const char *end, const char *mode, const char *path_type);

