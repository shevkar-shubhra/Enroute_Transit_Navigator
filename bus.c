#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include "file2.h"
#define MAX_STOPS 22757
#define INF 999999

int timeToSeconds(Time time) {
    return time.hr * 3600 + time.min * 60 + time.sec;
}

// Helper functions for MinHeap
MinHeap* createMinHeap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->nodes = (MinHeapNode*)malloc(capacity * sizeof(MinHeapNode));
    minHeap->positions = (int*)malloc(capacity * sizeof(int));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    for (int i = 0; i < capacity; ++i) {
        minHeap->positions[i] = -1;
    }
    return minHeap;
}

Graph* create_graph(int num_stops,int num_buses) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    debug_memory(graph, "Failed to allocate memory for graph");
    graph->num_buses = num_buses;
    
    graph->bus_times = (BusStartTiming*)malloc(num_buses * sizeof(BusStartTiming));
    for (int i = 0; i < num_buses; i++) {
        graph->bus_times[i].num_timings = 0;
        graph->bus_times[i].bus_timings = NULL;
    }

    graph->num_stops = num_stops;
    graph->stops = (Stop*)malloc(num_stops * sizeof(Stop));
    debug_memory(graph->stops, "Failed to allocate memory for stops");

    graph->adjLists = (AdjList*)malloc(num_stops * sizeof(AdjList));
    debug_memory(graph->adjLists, "Failed to allocate memory for adjLists");
    
    graph->stop_id_map = (int*)malloc(MAX_STOPS * sizeof(int));
    for (int i = 0; i < MAX_STOPS; i++) graph->stop_id_map[i] = -1; // Initialize mapping array
    debug_memory(graph->stop_id_map, "Failed to allocate memory for stop_id_map");


    for (int i = 0; i < num_stops; i++) {
        graph->adjLists[i].head = NULL;
       
    }
    return graph;
}

void swapNodes(MinHeapNode* a, MinHeapNode* b) {
    MinHeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void minHeapify(MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->nodes[left].total_time < minHeap->nodes[smallest].total_time)
        smallest = left;

    if (right < minHeap->size && minHeap->nodes[right].total_time < minHeap->nodes[smallest].total_time)
        smallest = right;

    if (smallest != idx) {
        minHeap->positions[minHeap->nodes[smallest].stop_id] = idx;
        minHeap->positions[minHeap->nodes[idx].stop_id] = smallest;
        swapNodes(&minHeap->nodes[smallest], &minHeap->nodes[idx]);
        minHeapify(minHeap, smallest);
    }
}

MinHeapNode extractMin(MinHeap* minHeap) {
    if (minHeap->size == 0)
        return (MinHeapNode){-1, -1, INT_MAX, -1, -1};

    MinHeapNode root = minHeap->nodes[0];
    MinHeapNode lastNode = minHeap->nodes[minHeap->size - 1];
    minHeap->nodes[0] = lastNode;

    minHeap->positions[root.stop_id] = -1;
    minHeap->positions[lastNode.stop_id] = 0;

    minHeap->size--;
    minHeapify(minHeap, 0);

    return root;
}

void decreaseKey(MinHeap* minHeap, int stop_id, int total_time, int prev_stop_id, int bus_id) {
    int i = minHeap->positions[stop_id];
    minHeap->nodes[i].total_time = total_time;
    minHeap->nodes[i].previous_stop_id = prev_stop_id;
    minHeap->nodes[i].bus_id = bus_id;

    while (i && minHeap->nodes[i].total_time < minHeap->nodes[(i - 1) / 2].total_time) {
        minHeap->positions[minHeap->nodes[i].stop_id] = (i - 1) / 2;
        minHeap->positions[minHeap->nodes[(i - 1) / 2].stop_id] = i;
        swapNodes(&minHeap->nodes[i], &minHeap->nodes[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void insertMinHeap(MinHeap* minHeap, MinHeapNode node) {
    // Ensure that the stop_id is within bounds, resize positions if necessary
    if (node.stop_id >= minHeap->capacity) {
        minHeap->capacity = node.stop_id + 1;
        minHeap->positions = realloc(minHeap->positions, minHeap->capacity * sizeof(int));
        // Check if realloc failed
        if (minHeap->positions == NULL) {
            printf("Memory allocation failed\n");
            exit(1);
        }
    }

    minHeap->nodes[minHeap->size] = node;
    minHeap->positions[node.stop_id] = minHeap->size;
    minHeap->size++;
    decreaseKey(minHeap, node.stop_id, node.total_time, node.previous_stop_id, node.bus_id);
}

char* dijkstraShortestTime(Graph* graph, int src_stop_id, int dest_stop_id) {
    const int TRANSFER_PENALTY_SECONDS = 5 * 60; // Transfer penalty (5 minutes)

    int num_stops = graph->num_stops;
    int* times = (int*)malloc(num_stops * sizeof(int));
    int* bus_transfer_counts = (int*)malloc(num_stops * sizeof(int));
    int* prev_stop_ids = (int*)malloc(num_stops * sizeof(int));
    int* prev_bus_ids = (int*)malloc(num_stops * sizeof(int));

    for (int i = 0; i < num_stops; ++i) {
        times[i] = INT_MAX;
        bus_transfer_counts[i] = INT_MAX;
        prev_stop_ids[i] = -1;
        prev_bus_ids[i] = -1;
    }

    int src_index = graph->stop_id_map[src_stop_id];
    int dest_index = graph->stop_id_map[dest_stop_id];
    times[src_index] = 0;
    bus_transfer_counts[src_index] = 0;

    // Direct route check (if a direct path exists)
    AdjStop* directAdj = graph->adjLists[src_index].head;
    while (directAdj != NULL) {
        if (directAdj->stop_id == dest_stop_id) {
            for (int i = 0; i < directAdj->bus_count; i++) {
                int travel_time = timeToSeconds(directAdj->time);
                char *result = (char*)malloc(256 * sizeof(char));
                snprintf(result, 256, "Direct route from %d to %d with bus ID %d\nTotal time taken: %02d:%02d:%02d (Direct route)\n", 
                         src_stop_id, dest_stop_id, directAdj->bus_ids[i], 
                         travel_time / 3600, (travel_time % 3600) / 60, travel_time % 60);
                return result;
            }
        }
        directAdj = directAdj->next;
    }

    // Proceed with Dijkstra's algorithm if no direct route is found
    MinHeap* minHeap = createMinHeap(num_stops);
    insertMinHeap(minHeap, (MinHeapNode){src_stop_id, -1, 0, 0, -1});

    while (minHeap->size > 0) {
        MinHeapNode minNode = extractMin(minHeap);
        int current_stop_id = minNode.stop_id;
        int current_time = minNode.total_time;
        int current_bus_transfer_count = minNode.bus_transfer_count;

        if (current_stop_id == dest_stop_id) {
            break;
        }

        int u_index = graph->stop_id_map[current_stop_id];
        AdjStop* adj = graph->adjLists[u_index].head;

        while (adj != NULL) {
            int v_index = graph->stop_id_map[adj->stop_id];
            for (int i = 0; i < adj->bus_count; i++) {
                int travel_time = timeToSeconds(adj->time); // Convert time to seconds
                int total_time = current_time + travel_time;
                int bus_id = adj->bus_ids[i];

                // Check for bus transfer
                int total_bus_transfer_count = current_bus_transfer_count;
                if (prev_bus_ids[u_index] != bus_id) {
                    total_bus_transfer_count++;
                    total_time += TRANSFER_PENALTY_SECONDS;  // Add transfer penalty
                }

                // Update condition: prioritize minimizing time and bus transfers
                if (total_time < times[v_index] || 
                    (total_time == times[v_index] && total_bus_transfer_count < bus_transfer_counts[v_index])) {
                    times[v_index] = total_time;
                    bus_transfer_counts[v_index] = total_bus_transfer_count;
                    prev_stop_ids[v_index] = current_stop_id;
                    prev_bus_ids[v_index] = bus_id;

                    insertMinHeap(minHeap, (MinHeapNode){adj->stop_id, bus_id, total_time, total_bus_transfer_count, current_stop_id});
                }
            }
            adj = adj->next;
        }
    }

    // Construct result string
    char *result = (char*)malloc(1024 * sizeof(char));  // Adjust size as needed
    if (times[dest_index] != INT_MAX) {
        snprintf(result, 1024, "Shortest path from %d to %d:\n", src_stop_id, dest_stop_id);

        int path[1000], path_index = 0, current_stop = dest_stop_id;

        while (current_stop != src_stop_id) {
            path[path_index++] = current_stop;
            current_stop = prev_stop_ids[graph->stop_id_map[current_stop]];
        }
        path[path_index++] = src_stop_id;

        int previous_bus_id = -1, start_stop = -1, accumulated_time = 0;
        int transfer_penalty_applied = 0;  // Track if penalty is added
        int k = 0;
        for (int i = path_index - 1; i > 0; i--) {
            int stop = path[i];
            int next_stop = path[i - 1];
            int bus_id = prev_bus_ids[graph->stop_id_map[next_stop]];
            int travel_time = times[graph->stop_id_map[next_stop]] - times[graph->stop_id_map[stop]];

            // Check for transfer and apply penalty if needed
            if (bus_id != previous_bus_id) {
                if (previous_bus_id != -1) {
                    // Subtract 5 minutes penalty before printing
                    int adjusted_time = accumulated_time - (transfer_penalty_applied ? 5 * 60 : 0);
                    char segment[256];
                    snprintf(segment, 256, "%d to %d via Bus ID %d, travel time: %02d:%02d:%02d", 
                             start_stop, stop, previous_bus_id, adjusted_time / 3600, 
                             (adjusted_time % 3600) / 60, adjusted_time % 60);
                    strcat(result, segment);  // Append segment to result string

                    if (transfer_penalty_applied && k != 0) {
                        strcat(result, " + 5 min (as there is a transfer)\n");
                    } else {
                        strcat(result, "\n");
                        k++;
                    }
                }

                start_stop = stop;
                previous_bus_id = bus_id;
                accumulated_time = travel_time;  // Reset accumulated time for this segment
                transfer_penalty_applied = 0;  // Reset the penalty flag
            } else {
                accumulated_time += travel_time;  // Accumulate time without the penalty
            }

            // If the next stop uses a different bus, apply the transfer penalty
            if (prev_bus_ids[graph->stop_id_map[stop]] != bus_id) {
                transfer_penalty_applied = 1;  // Mark that a transfer penalty is applied
            }
        }

        // Final output for the last leg of the journey
        int adjusted_time = accumulated_time - (transfer_penalty_applied ? 5 * 60 : 0);  // Adjust the last segment
        char last_leg[256];
        snprintf(last_leg, 256, "%d to %d via Bus ID %d, travel time: %02d:%02d:%02d", 
                 start_stop, dest_stop_id, previous_bus_id, adjusted_time / 3600, 
                 (adjusted_time % 3600) / 60, adjusted_time % 60);
        strcat(result, last_leg);

        if (transfer_penalty_applied) {
            strcat(result, " + 5 min (as there is a transfer)\n");
        } else {
            strcat(result, "\n");
        }

        // Subtract 5 minutes penalty (300 seconds) from the final total time
        int final_time = times[dest_index] - 300;  // Remove one penalty (as we add it manually)
        char total_time_str[100];
        snprintf(total_time_str, 100, "Total time: %02d:%02d:%02d\n", final_time / 3600, 
                 (final_time % 3600) / 60, final_time % 60);
        strcat(result, total_time_str);
    } else {
        snprintf(result, 1024, "No path found from %d to %d.\n", src_stop_id, dest_stop_id);
    }

    free(times);
    free(bus_transfer_counts);
    free(prev_stop_ids);
    free(prev_bus_ids);
    free(minHeap->positions);
    free(minHeap->nodes);
    free(minHeap);

    return result;
}

char* dijkstraFewestTransfers(Graph* graph, int src_stop_id, int dest_stop_id, int max_transfers) {
    // Validate inputs
    if (!graph || graph->num_stops <= 0) {
        return strdup("Invalid graph or empty input.\n");
    }
    if (src_stop_id == dest_stop_id) {
        char* result = (char*)malloc(64);
        sprintf(result, "Source and destination are the same: %d\n", src_stop_id);
        return result;
    }

    int num_stops = graph->num_stops;
    int* times = (int*)malloc(num_stops * sizeof(int));
    int* prev_stop_ids = (int*)malloc(num_stops * sizeof(int));
    int* prev_bus_ids = (int*)malloc(num_stops * sizeof(int));
    int* bus_transfers = (int*)malloc(num_stops * sizeof(int));

    // Initialize arrays
    for (int i = 0; i < num_stops; ++i) {
        times[i] = INT_MAX;
        prev_stop_ids[i] = -1;
        prev_bus_ids[i] = -1;
        bus_transfers[i] = INT_MAX;
    }

    int src_index = graph->stop_id_map[src_stop_id];
    int dest_index = graph->stop_id_map[dest_stop_id];
    times[src_index] = 0;
    bus_transfers[src_index] = 0;

    MinHeap* minHeap = createMinHeap(num_stops);
    insertMinHeap(minHeap, (MinHeapNode){src_stop_id, -1, 0, 0, -1});

    char* result = (char*)malloc(1024 * sizeof(char));
    result[0] = '\0';
    size_t result_size = 1024;

    while (minHeap->size > 0) {
        MinHeapNode minNode = extractMin(minHeap);
        int current_stop_id = minNode.stop_id;
        int current_time = minNode.total_time;
        int current_transfer_count = minNode.bus_transfer_count;

        if (current_stop_id == dest_stop_id) {
            break; // Destination reached
        }

        int u_index = graph->stop_id_map[current_stop_id];
        AdjStop* adj = graph->adjLists[u_index].head;

        while (adj != NULL) {
            int v_index = graph->stop_id_map[adj->stop_id];
            int travel_time = adj->time.hr * 3600 + adj->time.min * 60 + adj->time.sec;

            // Validate travel time
            if (travel_time < 0) {
                adj = adj->next;
                continue;
            }

            for (int i = 0; i < adj->bus_count; i++) {
                int bus_id = adj->bus_ids[i];
                int new_transfer_count = current_transfer_count;

                // Update transfer count
                if (prev_bus_ids[u_index] != bus_id && prev_bus_ids[u_index] != -1) {
                    new_transfer_count++;
                }

                // Process edge if within max transfer limit
                if (new_transfer_count <= max_transfers) {
                    int total_time = current_time + travel_time;

                    // Update if new path is better
                    if (new_transfer_count < bus_transfers[v_index] ||
                        (new_transfer_count == bus_transfers[v_index] && total_time < times[v_index])) {

                        times[v_index] = total_time;
                        bus_transfers[v_index] = new_transfer_count;
                        prev_stop_ids[v_index] = current_stop_id;
                        prev_bus_ids[v_index] = bus_id;

                        insertMinHeap(minHeap, (MinHeapNode){adj->stop_id, bus_id, total_time, new_transfer_count, current_stop_id});
                    }
                }
            }
            adj = adj->next;
        }
    }

    // Reconstruct the path if it exists
    if (times[dest_index] != INT_MAX) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Fewest transfers path from %d to %d:\n", src_stop_id, dest_stop_id);
        strcat(result, buffer);

        int path[1000];
        int path_index = 0;
        int current_stop = dest_stop_id;

        // Reconstruct path
        while (current_stop != src_stop_id) {
            path[path_index++] = current_stop;
            current_stop = prev_stop_ids[graph->stop_id_map[current_stop]];
        }
        path[path_index++] = src_stop_id;

        // Print path details
        int previous_bus_id = -1;
        int start_stop = -1;
        int accumulated_time = 0;

        for (int i = path_index - 1; i > 0; i--) {
            int stop = path[i];
            int next_stop = path[i - 1];
            int bus_id = prev_bus_ids[graph->stop_id_map[next_stop]];

            int travel_time = times[graph->stop_id_map[next_stop]] - times[graph->stop_id_map[stop]];
            if (bus_id != previous_bus_id) {
                if (previous_bus_id != -1) {
                    int hours = accumulated_time / 3600;
                    int minutes = (accumulated_time % 3600) / 60;
                    int seconds = accumulated_time % 60;
                    snprintf(buffer, sizeof(buffer), "%d to %d through bus ID %d, travel time: %02d:%02d:%02d\n",
                             start_stop, stop, previous_bus_id, hours, minutes, seconds);
                    strcat(result, buffer);
                }
                start_stop = stop;
                previous_bus_id = bus_id;
                accumulated_time = 0;
            }
            accumulated_time += travel_time;
        }

        int hours = accumulated_time / 3600;
        int minutes = (accumulated_time % 3600) / 60;
        int seconds = accumulated_time % 60;
        snprintf(buffer, sizeof(buffer), "%d to %d through bus ID %d, travel time: %02d:%02d:%02d\n",
                 start_stop, dest_stop_id, previous_bus_id, hours, minutes, seconds);
        strcat(result, buffer);

        int total_hours = times[dest_index] / 3600;
        int total_minutes = (times[dest_index] % 3600) / 60;
        int total_seconds = times[dest_index] % 60;
        snprintf(buffer, sizeof(buffer), "Total time taken: %02d:%02d:%02d\n", total_hours, total_minutes, total_seconds);
        strcat(result, buffer);
    } else {
        strcat(result, "No path found.\n");
    }

    free(times);
    free(prev_stop_ids);
    free(prev_bus_ids);
    free(bus_transfers);
    free(minHeap->positions);
    free(minHeap->nodes);
    free(minHeap);

    return result;
}


void debug_memory(void* ptr, const char* msg) {
    if (!ptr) {
        fprintf(stderr, "Memory Error: %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

Time parseTime(char *trip_id) {
    Time t;
    sscanf(trip_id, "%*[^_]_%d_%d", &t.hr, &t.min);
    t.sec = 0;  // Defaulting to 0, as seconds are not provided
    return t;
}

void addBusTiming(Graph *graph, int bus_id, Time timing) {
    for (int i = 0; i < graph->num_buses; i++) {
        if (graph->bus_times[i].bus_id == bus_id || graph->bus_times[i].num_timings == 0) {
            // If bus_id matches or entry is empty (new bus)
            if (graph->bus_times[i].num_timings == 0) {
                graph->bus_times[i].bus_id = bus_id;
            }
            // Expand timings array for the bus
            int index = graph->bus_times[i].num_timings++;
            graph->bus_times[i].bus_timings = (Time*)realloc(graph->bus_times[i].bus_timings, graph->bus_times[i].num_timings * sizeof(Time));
            graph->bus_times[i].bus_timings[index] = timing;
            return;
        }
    }
}

Time calculate_time_diff(char* time1, char* time2) {
    int h1, m1, s1, h2, m2, s2;
    sscanf(time1, "%d:%d:%d", &h1, &m1, &s1);
    sscanf(time2, "%d:%d:%d", &h2, &m2, &s2);

    int total_seconds1 = h1 * 3600 + m1 * 60 + s1;
    int total_seconds2 = h2 * 3600 + m2 * 60 + s2;
    int diff = abs(total_seconds2 - total_seconds1);

    Time time_diff;
    time_diff.hr = diff / 3600;
    time_diff.min = (diff % 3600) / 60;
    time_diff.sec = diff % 60;

    return time_diff;
}

AdjStop* create_adj_stop(int stop_id, int bus_id,Time time,float fare) {
    AdjStop* new_adj = (AdjStop*)malloc(sizeof(AdjStop));
    debug_memory(new_adj, "Failed to allocate memory for adjacency stop");

    new_adj->stop_id = stop_id;
    new_adj->time= time;
    new_adj->fare=fare;
    new_adj->bus_ids = (int*)malloc(sizeof(int));  // Initialize array for one bus ID
    debug_memory(new_adj->bus_ids, "Failed to allocate memory for bus_ids");
    
    new_adj->bus_ids[0] = bus_id;
    new_adj->bus_count = 1;
    new_adj->next = NULL;
    
    return new_adj;
}

void add_bus_id(AdjStop* adj_stop, int bus_id) {
	AdjStop *temp = adj_stop;
	int bus_id_checker = 1;
	for(int i = 0; i < adj_stop->bus_count; i++) {
		if((bus_id) == temp->bus_ids[i]) {
			bus_id_checker = 0;
			break;
		}
	}

	if(bus_id_checker) {
    	adj_stop->bus_ids = realloc(adj_stop->bus_ids, (adj_stop->bus_count + 1) * sizeof(int));
    	debug_memory(adj_stop->bus_ids, "Failed to reallocate memory for bus_ids");
    
    	adj_stop->bus_ids[adj_stop->bus_count] = bus_id;
    	adj_stop->bus_count++;
	}
}

void add_edge(Graph* graph, int from_stop_id, int to_stop_id, Time travel_time, float fare, int bus_id) {
    int from_index = graph->stop_id_map[from_stop_id];
    int to_index = graph->stop_id_map[to_stop_id];

    if (from_index == -1 || to_index == -1) {
        fprintf(stderr, "Invalid stop_id: %d or %d\n", from_stop_id, to_stop_id);
        return;
    }

    AdjStop* adj = graph->adjLists[from_index].head;
    AdjStop* found_adj = NULL;

    while (adj) {
        if (adj->stop_id == to_stop_id) {
            found_adj = adj;
            break;
        }
        adj = adj->next;
    }

    if (found_adj) {
        add_bus_id(found_adj, bus_id);
    } else {

        AdjStop* new_adj = create_adj_stop(to_stop_id, bus_id, travel_time,fare);
AdjStop* temp = graph->adjLists[from_index].head;

// If the list is empty, set the head to the new adjacent stop
if (temp == NULL) {
    graph->adjLists[from_index].head = new_adj;
} else {
    // Traverse to the last stop in the list
    while (temp->next != NULL) {
        temp = temp->next;
    }
    // Add the new stop at the end
    temp->next = new_adj;
}


        
    }
}



void load_stops(const char* filename, Graph* graph) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open stops file");
        exit(EXIT_FAILURE);
    }

    char line[110];
    fgets(line, sizeof(line), file); // Read header line

    int i = 0;
    int line_number = 1; // Start counting from the first data line

    while (fgets(line, sizeof(line), file) && i < graph->num_stops) {
        line_number++; // Increment line number for each line read
        int stop_id;
        float stop_lat, stop_lon;
        char stop_name[50];

        // Parsing the line
        if (sscanf(line, "%*[^,],%d,%f,%f,%49[^,]", &stop_id, &stop_lat, &stop_lon, stop_name) < 4) {
            fprintf(stderr, "Error parsing stop data on line %d: %s\n", line_number, line);
            continue; // Skip to the next line
        }

        // Store the stop information in the graph
        graph->stops[i].stop_id = stop_id;
        graph->stops[i].stop_lat = stop_lat;
        graph->stops[i].stop_lon = stop_lon;
        strcpy(graph->stops[i].stop_name, stop_name);

        graph->stop_id_map[stop_id] = i; 
        i++;
    }
    fclose(file);
}

void loadData(Graph *graph, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file1");
        return;
    }

    char line[256];
    fgets(line, sizeof(line), file);  // Skip header line

    while (fgets(line, sizeof(line), file)) {
        int bus_id;
        char trip_id[20];

        // Parse bus_id and trip_id from the line
        sscanf(line, "%d,%*d,%[^,]", &bus_id, trip_id);

        // Parse the time from trip_id and add it to the graph
        Time timing = parseTime(trip_id);
        addBusTiming(graph, bus_id, timing);
    }

    fclose(file);
}

void load_stop_times(const char* filename, Graph* graph) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open stop times file");
        exit(EXIT_FAILURE);
    }

    char line[100], prev_time[10], curr_time[10],bus_id[10];
    int prev_stop_id = -1, curr_stop_id, curr_seq_no;

    fgets(line, sizeof(line), file); // Ignore header

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        int parsedItems = sscanf(line, "%[^_]_%*[^,],%[^,],%*[^,],%d,%d", bus_id, curr_time, &curr_stop_id, &curr_seq_no);

        if (parsedItems < 4) {
            fprintf(stderr, "Error parsing stop time data: %s\n", line);
            continue;
        }
if (curr_seq_no){
        Time travel_time = calculate_time_diff(prev_time, curr_time);
                add_edge(graph, prev_stop_id, curr_stop_id, travel_time, -1,atoi(bus_id)); }

        

        prev_stop_id = curr_stop_id;
        strcpy(prev_time, curr_time);
    }

    fclose(file);
}



void load_fares(const char* filename, Graph* graph) {

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open fares file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    fgets(line, sizeof(line), file); 

    while (fgets(line, sizeof(line), file)) {
        char stop1[10], stop2[10];
        float price;
        
        if (sscanf(line, "%*[^_]_%*[^_]_%[^_]_%[^,],%f", stop1, stop2, &price) < 3) {
            fprintf(stderr, "Error parsing fare data: %s\n", line);
            continue;
        }

        int from_stop = graph->stop_id_map[atoi(stop1)];     
        int to_stop = graph->stop_id_map[atoi(stop2)];
        int to_id=atoi(stop2);

        if (from_stop < 0 || from_stop >= graph->num_stops || to_stop < 0 || to_stop >= graph->num_stops) {
            fprintf(stderr, "Warning: Invalid stop ID in fares file (%d -> %d)\n", from_stop, to_stop);
            continue;
        }

        AdjStop* adj = graph->adjLists[from_stop].head;
                
        int index = 0;
        while (adj) {
        
            if (adj->stop_id == to_id) {
                adj->fare=price;
                } 
            
            adj = adj->next;
            index++;
        }
    }

    fclose(file);
}




void display_graph(Graph* graph) {
    for (int i = 0; i < graph->num_stops; i++) {
        printf("Stop ID: %d, Name: %s\n", graph->stops[i].stop_id, graph->stops[i].stop_name);
        AdjStop* adj = graph->adjLists[i].head;


        // Assuming MAX_FARES is the maximum size of fares
        while (adj) {
            
            
                
                printf("    -> Stop ID: %d, Bus IDs: ", adj->stop_id);
                for (int k = 0; k < adj->bus_count; k++) {
                    printf("%d ", adj->bus_ids[k]);
                }
                printf(", Time: %02d:%02d:%02d, Fare: %.2f\n",
                    adj->time.hr,
                    adj->time.min,
                    adj->time.sec,
                    adj->fare);
            
            adj = adj->next;
            
        }
    }
}


void printBusTimings(Graph *graph) {
    for (int i = 0; i < graph->num_buses; i++) {
        if (graph->bus_times[i].num_timings > 0) {
            printf("Bus ID: %d\n", graph->bus_times[i].bus_id);
            for (int j = 0; j < graph->bus_times[i].num_timings; j++) {
                printf("  Timing: %02d:%02d\n", graph->bus_times[i].bus_timings[j].hr, graph->bus_times[i].bus_timings[j].min);
            }
        }
    }
}


void free_adj_stop(AdjStop* stop) {
    while (stop) {
        AdjStop* temp = stop;
        stop = stop->next;
        free(temp);
    }
}

void free_graph(Graph* graph) {
    for (int i = 0; i < 2; i++) {
        free_adj_stop(graph->adjLists[i].head);
        
    }
    free(graph->adjLists);
    free(graph->stops);
    free(graph);
}

int find_stop_id(const char *stop_name, const char *filename) {
	FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Unable to open file.\n");
        return -1; // Error code for file opening failure
    }

    char line[256];  // Buffer to store each line from the file
    while (fgets(line, sizeof(line), file)) {
        char stop_code[50], stop_id[10], stop_lat[20], stop_lon[20], stop_name_in_file[100], zone_id[10];
        
        // Parse the line using sscanf to extract the relevant data
        int result = sscanf(line, "%49[^,],%9[^,],%19[^,],%19[^,],%99[^,],%9[^,]", 
                             stop_code, stop_id, stop_lat, stop_lon, stop_name_in_file, zone_id);

        if (result == 6 && strcmp(stop_name, stop_name_in_file) == 0) {
            fclose(file);
            return atoi(stop_id);  // Return the stop_id as an integer
        }
    }

    fclose(file);
    return -1;  // Return -1 if stop name is not found
}

