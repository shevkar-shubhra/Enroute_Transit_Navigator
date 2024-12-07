#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include "file2.h"
#define MAX_STOPS 22757
#define INF 999999


char* main_logic(const char *start, const char *end, const char *mode, const char *path_type) {
    int num_stops = 10560;
    int num_buses = 2403;
    Graph* graph = create_graph(num_stops, num_buses);
    loadData(graph, "trips.txt");

    // Free allocated memory
    for (int i = 0; i < graph->num_buses; i++) {
        free(graph->bus_times[i].bus_timings);
    }
    free(graph->bus_times);
    
    load_stops("stops.txt", graph);
    load_stop_times("stop_times.txt", graph);
    load_fares("fare_attributes.txt", graph);

    int start_id = find_stop_id(start, "stops.txt");
    int end_id = find_stop_id(end, "stops.txt");

    // Determine the action based on mode and path type
    if (strcmp(mode, "Bus") == 0 && strcmp(path_type, "Shortest Path") == 0) {
        return dijkstraShortestTime(graph, start_id, end_id);
    } 
    
    else if (strcmp(mode, "Bus") == 0 && strcmp(path_type, "Minimum Interchange") == 0) {
        return dijkstraFewestTransfers(graph, start_id, end_id, 5);
    } 
    
    else if (strcmp(mode, "Metro") == 0 && strcmp(path_type, "Shortest Path") == 0) {
        //return find_shortest_path_metro(start, end);
    } 
    
    else if (strcmp(mode, "Metro") == 0 && strcmp(path_type, "Minimum Interchange") == 0) {
        //return find_minimum_interchange_metro(start, end);
    } 
    
    else {
        char *error_msg = malloc(1024 * sizeof(char));
        snprintf(error_msg, 1024, "Invalid selection. Please try again.");
        return error_msg;
    }
}

