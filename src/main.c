#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "traffic_simulation.h"

void initializeSDL(SDL_Window **window, SDL_Renderer **renderer)
{
    SDL_Init(SDL_INIT_VIDEO);
    *window = SDL_CreateWindow("Traffic Simulation",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               WINDOW_WIDTH, WINDOW_HEIGHT,
                               SDL_WINDOW_SHOWN);
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
        .startTime = SDL_GetTicks()
    };

    Uint32 lastVehicleSpawn = 0;

    while (running)
    {
        handleEvents(&running);

        // Spawn new vehicles
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastVehicleSpawn > 1000) // Spawn every second
        {
            if (vehicleCount < MAX_VEHICLES)
            {
                Direction direction = (Direction)(rand() % 4);
                Vehicle *newVehicle = createVehicle(direction);

                // Find empty slot in vehicles array
                for (int i = 0; i < MAX_VEHICLES; i++)
                {
                    if (!vehicles[i].active)
                    {
                        vehicles[i] = *newVehicle;
                        free(newVehicle);
                        vehicleCount++;
                        stats.totalVehicles++;
                        break;
                    }
                }

                lastVehicleSpawn = currentTime;
            }
        }

        // Update traffic lights
        updateTrafficLights(lights);

        // Update vehicles
        for (int i = 0; i < MAX_VEHICLES; i++)
        {
            if (vehicles[i].active)
            {
                updateVehicle(&vehicles[i], vehicles, lights);

                // Check if vehicle passed through intersection
                if (!vehicles[i].active)
                {
                    stats.vehiclesPassed++;
                    vehicleCount--;
                }
            }
        }

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
