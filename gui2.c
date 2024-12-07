#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "file2.h"

// Function to load stops from a file into a list store
GtkListStore* load_stops_from_file(const char *filename) {
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING); // Create a list store with one string column
    FILE *file = fopen(filename, "r");
    if (!file) {
        g_printerr("Error opening file: %s\n", filename);
        return store;
    }

    char line[256];
    GtkTreeIter iter;
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline character
        line[strcspn(line, "\n")] = 0;

        // Add stop name to the list store
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, line, -1);
    }

    fclose(file);
    return store;
}

// Set up autocomplete for a GtkEntry
void setup_autocomplete(GtkEntry *entry, GtkListStore *store) {
    GtkEntryCompletion *completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
    gtk_entry_completion_set_text_column(completion, 0); // Use the first column for suggestions
    gtk_entry_set_completion(entry, completion);
}

// Callback function for the "Find Route" button
void on_find_route_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *start_entry = g_object_get_data(G_OBJECT(button), "start_entry");
    GtkWidget *end_entry = g_object_get_data(G_OBJECT(button), "end_entry");
    GtkWidget *mode_combo = g_object_get_data(G_OBJECT(button), "mode_combo");
    GtkWidget *path_combo = g_object_get_data(G_OBJECT(button), "path_combo");
    GtkWidget *output_text_view = g_object_get_data(G_OBJECT(button), "output_text_view");

    // Ensure the mode_combo and path_type_combo are valid GtkComboBoxText widgets
    if (!GTK_IS_COMBO_BOX_TEXT(mode_combo)) {
        g_printerr("Error: mode_combo is not a GtkComboBoxText\n");
        return;
    }
    if (!GTK_IS_COMBO_BOX_TEXT(path_combo)) {
        g_printerr("Error: path_combo is not a GtkComboBoxText\n");
        return;
    }

    // Get user input from the GTK+ widgets (text entries, combo boxes, etc.)
    const char *start = gtk_entry_get_text(GTK_ENTRY(start_entry)); // Start stop from user
    const char *end = gtk_entry_get_text(GTK_ENTRY(end_entry)); // End stop from user

    // Get selected text from combo boxes
    const char *mode = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(mode_combo)); // Mode (Bus/Metro)
    const char *path = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(path_combo)); // Path type (Shortest Path/Minimum Interchange)

    if (!mode || !path) {
        g_printerr("Error: No selection made in combo boxes\n");
        return;
    }

    // Call the main_logic function and get the result
    char *result = main_logic(start, end, mode, path);

    // Display the result in a GTK+ TextView or other appropriate widget
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text_view));
    gtk_text_buffer_set_text(buffer, result, -1);

    // Free the allocated memory for result (if it was dynamically allocated)
    free(result);
}

// Callback function for the "Reset" button
void on_reset_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *start_entry = g_object_get_data(G_OBJECT(button), "start_entry");
    GtkWidget *end_entry = g_object_get_data(G_OBJECT(button), "end_entry");
    GtkWidget *mode_combo = g_object_get_data(G_OBJECT(button), "mode_combo");
    GtkWidget *path_combo = g_object_get_data(G_OBJECT(button), "path_combo");
    GtkWidget *output_text_view = g_object_get_data(G_OBJECT(button), "output_text_view");

    // Reset the text entries
    gtk_entry_set_text(GTK_ENTRY(start_entry), "");
    gtk_entry_set_text(GTK_ENTRY(end_entry), "");

    // Reset the combo boxes to their first item (you can adjust as needed)
    gtk_combo_box_set_active(GTK_COMBO_BOX(path_combo), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(mode_combo), 0);

    // Clear the output text view
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *grid;
    GtkWidget *start_label, *end_label, *path_label, *mode_label;
    GtkWidget *start_entry, *end_entry;
    GtkWidget *find_route_button, *reset_button;
    GtkWidget *path_combo, *mode_combo;
    GtkWidget *output_text_view;
    GtkTextBuffer *buffer;
    GtkCssProvider *css_provider;
    GtkStyleContext *context;
    GtkListStore *stop_store;

    gtk_init(&argc, &argv);

    // Load stops from a file
    const char *filename = "new_stops.txt"; // Replace with your file name
    stop_store = load_stops_from_file(filename);

    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Delhi Transportation System");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create vertical box container
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create grid container for input fields
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 10);

    // Create labels and input fields
    start_label = gtk_label_new("Starting Location:");
    end_label = gtk_label_new("Destination:");
    path_label = gtk_label_new("Path Type:");
    mode_label = gtk_label_new("Mode of Transport:");

    start_entry = gtk_entry_new();
    end_entry = gtk_entry_new();

    // Set up autocomplete for start and end entries
    setup_autocomplete(GTK_ENTRY(start_entry), stop_store);
    setup_autocomplete(GTK_ENTRY(end_entry), stop_store);

    // Create dropdown menu for path type
    path_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(path_combo), "Shortest Path");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(path_combo), "Minimum Interchange");

    // Create dropdown menu for mode type
    mode_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mode_combo), "Bus");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mode_combo), "Metro");

    // Create output text view
    output_text_view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_text_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_text_view), FALSE);

    // Create buttons
    find_route_button = gtk_button_new_with_label("Find Route");
    reset_button = gtk_button_new_with_label("Reset");

    // Arrange widgets in grid
    gtk_grid_attach(GTK_GRID(grid), start_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), start_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), end_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), end_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), path_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), path_combo, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mode_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mode_combo, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), find_route_button, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), reset_button, 1, 4, 1, 1);

    gtk_box_pack_start(GTK_BOX(vbox), output_text_view, TRUE, TRUE, 10);

    // Connect signals
    g_object_set_data(G_OBJECT(find_route_button), "start_entry", start_entry);
    g_object_set_data(G_OBJECT(find_route_button), "end_entry", end_entry);
    g_object_set_data(G_OBJECT(find_route_button), "output_text_view", output_text_view);
    g_object_set_data(G_OBJECT(find_route_button), "path_combo", path_combo); // Fix: Use correct key
    g_object_set_data(G_OBJECT(find_route_button), "mode_combo", mode_combo);
    g_signal_connect(find_route_button, "clicked", G_CALLBACK(on_find_route_clicked), NULL);
    
     g_object_set_data(G_OBJECT(reset_button), "start_entry", start_entry);
    g_object_set_data(G_OBJECT(reset_button), "end_entry", end_entry);
    g_object_set_data(G_OBJECT(reset_button), "mode_combo", mode_combo);
    g_object_set_data(G_OBJECT(reset_button), "path_combo", path_combo);
    g_object_set_data(G_OBJECT(reset_button), "output_text_view", output_text_view);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_button_clicked), NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start GTK main loop
    gtk_main();

    // Free the list store
    g_object_unref(stop_store);
    
    return 0;
}

