#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h> // Include for Sleep function
#include "traffic_simulation.h"

#define LANE_FILES_COUNT 4
const char *laneFiles[LANE_FILES_COUNT] = {
    "bin/lanea.txt",
    "bin/laneb.txt",
    "bin/lanec.txt",
    "bin/laned.txt"
};

void createFileIfNotExists(const char *filename) {
    FILE *file = fopen(filename, "a"); // Open for appending, creates file if it does not exist.
    if (file) {
        fclose(file); // Close the file immediately.
    } else {
        perror("Failed to create lane file");
    }
}

void clearFileContents(const char *filename) {
    FILE *file = fopen(filename, "w"); // Open for writing, clears the file contents
    if (file) {
        fclose(file); // Close the file immediately
    } else {
        perror("Failed to clear lane file");
    }
}

void generateVehicleData(Direction direction) {
    createFileIfNotExists(laneFiles[direction]); // Ensure the file exists
    FILE *file = fopen(laneFiles[direction], "a");
    if (!file) {
        perror("Failed to open lane file");
        return;
    }

    Vehicle vehicle;
    vehicle.type = (VehicleType)(rand() % 4); // Random vehicle type
    vehicle.direction = direction;
    vehicle.speed = (vehicle.type == AMBULANCE || vehicle.type == POLICE_CAR) ? 4.0f : 2.0f;
    fprintf(file, "%d,%d,%f\n", vehicle.type, vehicle.direction, vehicle.speed);

    fclose(file);
}

int SDL_main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // Ensure lane files exist and clear their contents
    for (int i = 0; i < LANE_FILES_COUNT; i++) {
        createFileIfNotExists(laneFiles[i]);
        clearFileContents(laneFiles[i]);
    }

    while (1) {
        for (int i = 0; i < LANE_FILES_COUNT; i++) {
            generateVehicleData((Direction)i);
        }
        Sleep(2000); // Generate a vehicle every 2 seconds (2000 milliseconds)
    }

    return 0;
}