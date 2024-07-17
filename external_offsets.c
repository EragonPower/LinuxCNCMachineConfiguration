#include <stdio.h>
#include <stdlib.h>
#include <rtapi.h>
#include <rtapi_app.h>
#include <hal.h>

#define MAX_POINTS 200

// Structure to store matrix points
typedef struct {
    float x;
    float y;
    float z_offset;
} Point;

Point points[MAX_POINTS];
int num_points = 0;

// HAL pins
static float *x_pos;
static float *y_pos;
static float *z_offset_counts;
static int *z_offset_enable;
static float *z_offset_scale;

// Global variable for component ID
static int comp_id;

// Function to load matrix from file
void load_matrix(const char *filename) {
	rtapi_print_msg(RTAPI_MSG_ERR, "Loading matrix\n");
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        rtapi_print_msg(RTAPI_MSG_ERR, "Error opening matrix file\n");
        return;
    }
    while (fscanf(file, "%f, %f, %f", &points[num_points].x, &points[num_points].y, &points[num_points].z_offset) == 3) { 
        num_points++;
        if (num_points >= MAX_POINTS) {
			rtapi_print_msg(RTAPI_MSG_ERR, "Finished reading matrix\n");
            break;
        }
    }
    rtapi_print_msg(RTAPI_MSG_ERR, "Unloading matrix\n");
    fclose(file);
}

// Function to interpolate z offset
float interpolate_z_offset(float x, float y) {
    for (int i = 0; i < num_points; i++) {
        if (points[i].x == x && points[i].y == y) {
            return points[i].z_offset;
        }
    }
    return 0.0; // Default offset if not found
}

// Function to update offsets
void update_offsets(void) {
    if (*z_offset_enable) {
        float x = *x_pos;
        float y = *y_pos;
        float z_offset = -interpolate_z_offset(x, y);
        *z_offset_counts = z_offset * (*z_offset_scale);
    } else {
        *z_offset_counts = 0.0;
    }
}

// Main function for the application
int rtapi_app_main(void) {
    // Initialize HAL
    comp_id = hal_init("external_offsets");

    // Load matrix
    load_matrix("/home/linuxcnc/Desktop/CNC/offset_matrix.csv");

    // Register HAL pins
    hal_pin_float_new("external_offsets.x-pos", HAL_IN, &x_pos, comp_id);
    hal_pin_float_new("external_offsets.y-pos", HAL_IN, &y_pos, comp_id);
    hal_pin_float_new("external_offsets.z-offset-counts", HAL_OUT, &z_offset_counts, comp_id);
    hal_pin_bit_new("external_offsets.z-offset-enable", HAL_IN, &z_offset_enable, comp_id);
    hal_pin_float_new("external_offsets.z-offset-scale", HAL_IN, &z_offset_scale, comp_id);

    // Export update_offsets function to HAL
    hal_export_funct("external_offsets.update", update_offsets, NULL, 1, 0, comp_id);

    return 0;
}

// Function called on application exit
void rtapi_app_exit(void) {
    // Clean up if needed
}
