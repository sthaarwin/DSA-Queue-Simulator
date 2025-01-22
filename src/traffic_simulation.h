#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include <SDL.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define LANE_WIDTH 80 // Updated for four lanes
#define MAX_VEHICLES 200
#define INTERSECTION_X (WINDOW_WIDTH / 2)
#define INTERSECTION_Y (WINDOW_HEIGHT / 2)

typedef enum
{
    DIRECTION_NORTH,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST
} Direction;

typedef enum
{
    REGULAR_CAR,
    AMBULANCE,
    POLICE_CAR,
    FIRE_TRUCK
} VehicleType;

typedef enum
{
    RED,
    GREEN
} TrafficLightState;

#define TRAFFIC_LIGHT_WIDTH 30
#define TRAFFIC_LIGHT_HEIGHT 60
#define STOP_LINE_WIDTH 5

typedef struct
{
    SDL_Rect rect;
    VehicleType type;
    Direction direction;
    float speed;
    float x;
    float y;
    bool active;
} Vehicle;

typedef struct
{
    TrafficLightState state;
    int timer;
    SDL_Rect position;
    Direction direction;
} TrafficLight;

typedef struct
{
    int vehiclesPassed;
    int totalVehicles;
    float vehiclesPerMinute;
    Uint32 startTime;
} Statistics;

// Function declarations
void initializeTrafficLights(TrafficLight *lights);
void updateTrafficLights(TrafficLight *lights);
Vehicle *createVehicle(Direction direction);
void updateVehicle(Vehicle *vehicle, Vehicle *vehicles, TrafficLight *lights);
void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats);
void renderRoads(SDL_Renderer *renderer);

#endif
