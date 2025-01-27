#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "traffic_simulation.h"

#define LANE_FILES_COUNT 4
const char *laneFiles[LANE_FILES_COUNT] = {
    "bin/lanea.txt",
    "bin/laneb.txt",
    "bin/lanec.txt",
    "bin/laned.txt"};

void createFileIfNotExists(const char *filename)
{
    FILE *file = fopen(filename, "a"); // Open for appending, creates file if it does not exist.
    if (file)
    {
        fclose(file); // Close the file immediately.
    }
    else
    {
        perror("Failed to create lane file");
    }
}

void loadVehiclesFromFile(int laneIndex)
{
    createFileIfNotExists(laneFiles[laneIndex]); // Ensure the file exists
    FILE *file = fopen(laneFiles[laneIndex], "r");
    if (!file)
    {
        perror("Failed to open lane file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        Vehicle vehicle;
        sscanf(line, "%d,%d,%f", &vehicle.type, &vehicle.direction, &vehicle.speed);
        vehicle.active = true;                    // Set vehicle as active
        vehicle.state = STATE_MOVING;             // Set initial state
        vehicle.turnDirection = TURN_NONE;        // Default to no turn
        vehicle.turnAngle = 0.0f;                 // Initialize turn angle
        vehicle.turnProgress = 0.0f;              // Initialize turn progress
        vehicle.isInRightLane = true;             // Default to right lane
        vehicle.rect.w = 20;                      // Default width
        vehicle.rect.h = 30;                      // Default height
        vehicle.x = 0;                            // Default x position
        vehicle.y = 0;                            // Default y position
        enqueue(&laneQueues[laneIndex], vehicle); // Add to queue
    }

    fclose(file);

    // Clear file contents after reading
    file = fopen(laneFiles[laneIndex], "w");
    fclose(file);
}

void initializeSDL(SDL_Window **window, SDL_Renderer **renderer)
{
    SDL_Init(SDL_INIT_VIDEO);
    *window = SDL_CreateWindow("Traffic Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(*renderer, 255, 255, 255, 255); // Set background color to white
}

void cleanupSDL(SDL_Window *window, SDL_Renderer *renderer)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void handleEvents(bool *running)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            *running = false;
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    bool running = true;

    srand(time(NULL));

    initializeSDL(&window, &renderer);

    // Initialize vehicles
    Vehicle vehicles[MAX_VEHICLES] = {0};
    int vehicleCount = 0;

    // Initialize traffic lights
    TrafficLight lights[4];
    initializeTrafficLights(lights);

    // Initialize statistics
    Statistics stats = {
        .vehiclesPassed = 0,
        .totalVehicles = 0,
        .vehiclesPerMinute = 0,
        .startTime = SDL_GetTicks()};

    // Initialize queues
    for (int i = 0; i < 4; i++)
    {
        initQueue(&laneQueues[i]);
    }

    Uint32 lastVehicleSpawn = 0;

    // Ensure lane files exist
    for (int i = 0; i < LANE_FILES_COUNT; i++)
    {
        createFileIfNotExists(laneFiles[i]);
    }

    while (running)
    {
        handleEvents(&running);

        // Spawn new vehicles
        Uint32 currentTime = SDL_GetTicks();
       
            for (int i = 0; i < LANE_FILES_COUNT; i++) {
                loadVehiclesFromFile(i);
            }

        // Update vehicles
        for (int i = 0; i < MAX_VEHICLES; i++)
        {
            if (vehicles[i].active)
            {
                updateVehicle(&vehicles[i], lights);

                // Check if vehicle has passed through intersection
                if (!vehicles[i].active)
                {
                    stats.vehiclesPassed++;
                    vehicleCount--;
                }
            }
        }

        // Update traffic lights
        updateTrafficLights(lights);

        // Update statistics
        float minutes = (currentTime - stats.startTime) / 60000.0f;
        if (minutes > 0)
        {
            stats.vehiclesPerMinute = stats.vehiclesPassed / minutes;
        }

        renderSimulation(renderer, vehicles, lights, &stats);

        SDL_Delay(16); // Cap at ~60 FPS
    }

    cleanupSDL(window, renderer);

    return 0;
}