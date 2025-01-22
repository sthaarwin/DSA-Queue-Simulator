#include <stdio.h>
#include <stdlib.h>
#include "traffic_simulation.h"

void initializeTrafficLights(TrafficLight *lights)
{
    // Initialize traffic lights with better positioned stop lights
    lights[0] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH/2 - TRAFFIC_LIGHT_WIDTH/2, 
                    INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT,
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH
    };
    
    lights[1] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH/2 - TRAFFIC_LIGHT_WIDTH/2,
                    INTERSECTION_Y + LANE_WIDTH,
                    TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH
    };
    
    lights[2] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH,
                    INTERSECTION_Y - LANE_WIDTH/2 - TRAFFIC_LIGHT_WIDTH/2,
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST
    };
    
    lights[3] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT,
                    INTERSECTION_Y + LANE_WIDTH/2 - TRAFFIC_LIGHT_WIDTH/2,
                    TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST
    };
}

void updateTrafficLights(TrafficLight *lights)
{
    Uint32 currentTicks = SDL_GetTicks();
    static Uint32 lastUpdateTicks = 0;

    if (currentTicks - lastUpdateTicks >= 5000) // Change lights every 5 seconds
    {
        lastUpdateTicks = currentTicks;

        // Toggle between RED and GREEN
        for (int i = 0; i < 4; i++)
        {
            lights[i].state = (lights[i].state == RED) ? GREEN : RED;
        }
    }
}

Vehicle *createVehicle(Direction direction)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;
    vehicle->type = (rand() % 100 < 10) ? (VehicleType)(rand() % 4) : REGULAR_CAR;
    vehicle->active = true;
    vehicle->speed = (vehicle->type == REGULAR_CAR) ? 2.0f : 3.0f;

    int laneOffset = (rand() % 4 - 2) * (LANE_WIDTH / 4); // Adjusted for four lanes

    switch (direction)
    {
    case DIRECTION_NORTH:
        vehicle->x = INTERSECTION_X + laneOffset;
        vehicle->y = WINDOW_HEIGHT + 50;
        break;
    case DIRECTION_SOUTH:
        vehicle->x = INTERSECTION_X - laneOffset;
        vehicle->y = -50;
        break;
    case DIRECTION_EAST:
        vehicle->x = -50;
        vehicle->y = INTERSECTION_Y + laneOffset;
        break;
    case DIRECTION_WEST:
        vehicle->x = WINDOW_WIDTH + 50;
        vehicle->y = INTERSECTION_Y - laneOffset;
        break;
    }

    vehicle->rect = (SDL_Rect){.x = (int)vehicle->x, .y = (int)vehicle->y, .w = 30, .h = 30};
    return vehicle;
}

void updateVehicle(Vehicle *vehicle, Vehicle *vehicles, TrafficLight *lights)
{
    if (!vehicle->active)
        return;

    TrafficLight *relevantLight = NULL;
    float stopLine = 0;
    bool canMove = true;

    for (int i = 0; i < 4; i++)
    {
        if (lights[i].direction == vehicle->direction)
        {
            relevantLight = &lights[i];
            switch (vehicle->direction)
            {
            case DIRECTION_NORTH:
                stopLine = INTERSECTION_Y - 60;
                break;
            case DIRECTION_SOUTH:
                stopLine = INTERSECTION_Y + 60;
                break;
            case DIRECTION_EAST:
                stopLine = INTERSECTION_X - 60;
                break;
            case DIRECTION_WEST:
                stopLine = INTERSECTION_X + 60;
                break;
            }
        }
    }

    // Collision detection
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (&vehicles[i] != vehicle && vehicles[i].active && vehicles[i].direction == vehicle->direction)
        {
            float distance = 0;
            switch (vehicle->direction)
            {
            case DIRECTION_NORTH:
                distance = vehicles[i].y - vehicle->y;
                if (distance > 0 && distance < 50)
                    canMove = false;
                break;
            case DIRECTION_SOUTH:
                distance = vehicle->y - vehicles[i].y;
                if (distance > 0 && distance < 50)
                    canMove = false;
                break;
            case DIRECTION_EAST:
                distance = vehicles[i].x - vehicle->x;
                if (distance > 0 && distance < 50)
                    canMove = false;
                break;
            case DIRECTION_WEST:
                distance = vehicle->x - vehicles[i].x;
                if (distance > 0 && distance < 50)
                    canMove = false;
                break;
            }
        }
    }

    // Stop at red light
    if (relevantLight && relevantLight->state == RED)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            if (vehicle->y > stopLine && vehicle->y < INTERSECTION_Y + 100)
                canMove = false;
            break;
        case DIRECTION_SOUTH:
            if (vehicle->y < stopLine && vehicle->y > INTERSECTION_Y - 100)
                canMove = false;
            break;
        case DIRECTION_EAST:
            if (vehicle->x < stopLine && vehicle->x > INTERSECTION_X - 100)
                canMove = false;
            break;
        case DIRECTION_WEST:
            if (vehicle->x > stopLine && vehicle->x < INTERSECTION_X + 100)
                canMove = false;
            break;
        }
    }

    if (canMove || vehicle->type != REGULAR_CAR) // Special vehicles can move regardless of red light
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->y -= vehicle->speed;
            break;
        case DIRECTION_SOUTH:
            vehicle->y += vehicle->speed;
            break;
        case DIRECTION_EAST:
            vehicle->x += vehicle->speed;
            break;
        case DIRECTION_WEST:
            vehicle->x -= vehicle->speed;
            break;
        }
    }

    // Check if vehicle has left the screen
    if (vehicle->x < -50 || vehicle->x > WINDOW_WIDTH + 50 ||
        vehicle->y < -50 || vehicle->y > WINDOW_HEIGHT + 50)
    {
        vehicle->active = false;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;
}

void renderRoads(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray color for roads

    // Draw the intersection first
    SDL_Rect intersection = {
        INTERSECTION_X - LANE_WIDTH,
        INTERSECTION_Y - LANE_WIDTH,
        LANE_WIDTH * 2,
        LANE_WIDTH * 2
    };
    SDL_RenderFillRect(renderer, &intersection);

    // Draw main roads
    SDL_Rect verticalRoad1 = {INTERSECTION_X - LANE_WIDTH, 0, LANE_WIDTH * 2, INTERSECTION_Y - LANE_WIDTH};
    SDL_Rect verticalRoad2 = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, LANE_WIDTH * 2, WINDOW_HEIGHT - INTERSECTION_Y - LANE_WIDTH};
    SDL_Rect horizontalRoad1 = {0, INTERSECTION_Y - LANE_WIDTH, INTERSECTION_X - LANE_WIDTH, LANE_WIDTH * 2};
    SDL_Rect horizontalRoad2 = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, WINDOW_WIDTH - INTERSECTION_X - LANE_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &verticalRoad1);
    SDL_RenderFillRect(renderer, &verticalRoad2);
    SDL_RenderFillRect(renderer, &horizontalRoad1);
    SDL_RenderFillRect(renderer, &horizontalRoad2);

    // Draw lane dividers
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Vertical lane dividers (skip intersection)
    for (int i = 0; i < WINDOW_HEIGHT; i += 40)
    {
        if (i < INTERSECTION_Y - LANE_WIDTH || i > INTERSECTION_Y + LANE_WIDTH)
        {
            SDL_Rect laneDivider1 = {INTERSECTION_X - LANE_WIDTH / 2 - 1, i, 2, 20};
            SDL_Rect laneDivider2 = {INTERSECTION_X + LANE_WIDTH / 2 - 1, i, 2, 20};
            SDL_RenderFillRect(renderer, &laneDivider1);
            SDL_RenderFillRect(renderer, &laneDivider2);
        }
    }

    // Horizontal lane dividers (skip intersection)
    for (int i = 0; i < WINDOW_WIDTH; i += 40)
    {
        if (i < INTERSECTION_X - LANE_WIDTH || i > INTERSECTION_X + LANE_WIDTH)
        {
            SDL_Rect laneDivider1 = {i, INTERSECTION_Y - LANE_WIDTH / 2 - 1, 20, 2};
            SDL_Rect laneDivider2 = {i, INTERSECTION_Y + LANE_WIDTH / 2 - 1, 20, 2};
            SDL_RenderFillRect(renderer, &laneDivider1);
            SDL_RenderFillRect(renderer, &laneDivider2);
        }
    }

    // Add stop lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // North stop line
    SDL_Rect northStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - STOP_LINE_WIDTH,
                         LANE_WIDTH * 2, STOP_LINE_WIDTH};
    SDL_RenderFillRect(renderer, &northStop);
    
    // South stop line
    SDL_Rect southStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH,
                         LANE_WIDTH * 2, STOP_LINE_WIDTH};
    SDL_RenderFillRect(renderer, &southStop);
    
    // East stop line
    SDL_Rect eastStop = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH,
                        STOP_LINE_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &eastStop);
    
    // West stop line
    SDL_Rect westStop = {INTERSECTION_X - LANE_WIDTH - STOP_LINE_WIDTH, INTERSECTION_Y - LANE_WIDTH,
                        STOP_LINE_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &westStop);
}

void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats)
{
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Brighter background color
    SDL_RenderClear(renderer);

    // Render roads
    renderRoads(renderer);

    // Draw traffic lights with more visible design
    for (int i = 0; i < 4; i++)
    {
        // Draw traffic light housing (dark gray)
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderFillRect(renderer, &lights[i].position);
        
        // Draw traffic light color
        switch (lights[i].state)
        {
        case RED:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case GREEN:
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            break;
        }
        SDL_RenderFillRect(renderer, &lights[i].position);
    }

    // Draw vehicles
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (vehicles[i].active)
        {
            switch (vehicles[i].type)
            {
            case REGULAR_CAR:
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                break;
            case AMBULANCE:
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                break;
            case POLICE_CAR:
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                break;
            case FIRE_TRUCK:
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                break;
            }
            SDL_RenderFillRect(renderer, &vehicles[i].rect);
        }
    }

    // Render statistics (optional)
    // Add stats rendering if needed with SDL_ttf

    SDL_RenderPresent(renderer);
}
