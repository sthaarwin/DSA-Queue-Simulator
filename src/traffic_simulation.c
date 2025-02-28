#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"

// Global queues for lanes
Queue laneQueues[4];
int lanePriorities[4] = {0};
LanePosition laneVehicles[4][MAX_VEHICLES];
int vehiclesInLane[4] = {0};

const SDL_Color VEHICLE_COLORS[] = {
    {0, 0, 255, 255}, // REGULAR_CAR: Blue
    {255, 0, 0, 255}, // AMBULANCE: Red
    {0, 0, 128, 255}, // POLICE_CAR: Dark Blue
    {255, 69, 0, 255} // FIRE_TRUCK: Orange-Red
};

float getDistanceBetweenVehicles(Vehicle *v1, Vehicle *v2)
{
    float dx = v1->x - v2->x;
    float dy = v1->y - v2->y;
    return sqrt(dx * dx + dy * dy);
}

int getVehicleLane(Vehicle *vehicle)
{
    if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH)
    {
        return (vehicle->x < INTERSECTION_X) ? 0 : 1;
    }
    else
    {
        return (vehicle->y < INTERSECTION_Y) ? 2 : 3;
    }
}

void initializeTrafficLights(TrafficLight *lights)
{
    lights[0] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH};
    lights[1] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH};
    lights[2] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST};
    lights[3] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST};
}

void updateTrafficLights(TrafficLight *lights)
{
    static Uint32 lastStateChangeTicks = 0;
    static int currentPhase = 0;
    static bool priorityMode = false;
    static int priorityLane = -1;
    static Uint32 priorityStartTime = 0;
    Uint32 currentTicks = SDL_GetTicks();

    // Check for priority conditions (special vehicles or congestion)
    int priorityLaneCandidate = -1;
    bool hasSpecialVehicle = false;
    int maxWaitingVehicles = 0;

    // First pass: check for special vehicles in each lane
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < vehiclesInLane[i]; j++)
        {
            Vehicle *vehicle = laneVehicles[i][j].vehicle;
            if (vehicle && (vehicle->type == AMBULANCE || vehicle->type == POLICE_CAR || vehicle->type == FIRE_TRUCK))
            {
                hasSpecialVehicle = true;
                priorityLaneCandidate = i;
                // Allow emergency vehicles to pass red lights
                vehicle->canSkipLight = true;
                break;
            }
        }
        if (hasSpecialVehicle)
            break; // Once we find a special vehicle, no need to check other lanes

        // Track the lane with the most vehicles for congestion detection
        if (vehiclesInLane[i] > maxWaitingVehicles)
        {
            maxWaitingVehicles = vehiclesInLane[i];
            priorityLaneCandidate = i;
        }
    }

    // Determine if we should enter or maintain priority mode
    if (hasSpecialVehicle || (maxWaitingVehicles > 5 && !priorityMode))
    {
        priorityMode = true;
        priorityLane = priorityLaneCandidate;
        priorityStartTime = currentTicks;

        // Fix: explicitly set lights based on direction rather than using modulo
        // This ensures correct pairing of traffic lights
        if (priorityLane == 0 || priorityLane == 1)
        { // North or South lane has priority
            // Give green to North-South, red to East-West
            lights[DIRECTION_NORTH].state = GREEN;
            lights[DIRECTION_SOUTH].state = GREEN;
            lights[DIRECTION_EAST].state = RED;
            lights[DIRECTION_WEST].state = RED;
        }
        else
        { // East or West lane has priority
            // Give green to East-West, red to North-South
            lights[DIRECTION_NORTH].state = RED;
            lights[DIRECTION_SOUTH].state = RED;
            lights[DIRECTION_EAST].state = GREEN;
            lights[DIRECTION_WEST].state = GREEN;
        }

        printf("Priority mode activated at %d ms. Lane %d prioritized. Reason: %s\n",
               currentTicks, priorityLane, hasSpecialVehicle ? "Emergency Vehicle" : "Congestion");
        lastStateChangeTicks = currentTicks; // Reset the state change timer
    }
    // Exit priority mode after 10 seconds if no special vehicles remain
    else if (priorityMode && currentTicks - priorityStartTime >= 10000)
    {
        bool stillHasSpecialVehicle = false;

        // Check if special vehicles are still present in the priority lane
        for (int j = 0; j < vehiclesInLane[priorityLane]; j++)
        {
            Vehicle *vehicle = laneVehicles[priorityLane][j].vehicle;
            if (vehicle && (vehicle->type == AMBULANCE || vehicle->type == POLICE_CAR || vehicle->type == FIRE_TRUCK))
            {
                stillHasSpecialVehicle = true;
                break;
            }
        }

        if (!stillHasSpecialVehicle)
        {
            priorityMode = false;
            printf("Priority mode deactivated at %d ms. Returning to normal cycle.\n", currentTicks);
        }
        else
        {
            // Extend priority mode
            priorityStartTime = currentTicks;
        }
    }

    // Normal traffic light cycle if not in priority mode
    if (!priorityMode && currentTicks - lastStateChangeTicks >= 5000)
    {
        // Toggle between phases (0 = N/S green, E/W red; 1 = N/S red, E/W green)
        currentPhase = 1 - currentPhase;

        if (currentPhase == 0)
        { // North/South green, East/West red
            lights[DIRECTION_NORTH].state = GREEN;
            lights[DIRECTION_SOUTH].state = GREEN;
            lights[DIRECTION_EAST].state = RED;
            lights[DIRECTION_WEST].state = RED;
        }
        else
        { // North/South red, East/West green
            lights[DIRECTION_NORTH].state = RED;
            lights[DIRECTION_SOUTH].state = RED;
            lights[DIRECTION_EAST].state = GREEN;
            lights[DIRECTION_WEST].state = GREEN;
        }

        lastStateChangeTicks = currentTicks;
        printf("State changed at %d ms. Phase: %d, Reason: Normal Cycle\n", currentTicks, currentPhase);
    }

    // Reset canSkipLight flag for non-emergency vehicles
    // for (int i = 0; i < 4; i++)
    // {
    //     for (int j = 0; j < vehiclesInLane[i]; j++)
    //     {
    //         Vehicle *vehicle = laneVehicles[i][j].vehicle;
    //         if (vehicle && vehicle->type == REGULAR_CAR)
    //         {
    //             vehicle->canSkipLight = false;
    //         }
    //     }
    // }
}

Vehicle *createVehicle(Direction direction)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;

    // Set vehicle type with probabilities
    int typeRoll = rand() % 100;
    if (typeRoll < 5)
    {
        vehicle->type = AMBULANCE;
    }
    else if (typeRoll < 10)
    {
        vehicle->type = POLICE_CAR;
    }
    else if (typeRoll < 15)
    {
        vehicle->type = FIRE_TRUCK;
    }
    else
    {
        vehicle->type = REGULAR_CAR;
    }

    vehicle->active = true;
    vehicle->canSkipLight = false; // Initialize canSkipLight to false
    // Set speed based on vehicle type
    switch (vehicle->type)
    {
    case AMBULANCE:
    case POLICE_CAR:
        vehicle->speed = 4.0f;
        break;
    case FIRE_TRUCK:
        vehicle->speed = 3.5f;
        break;
    default:
        vehicle->speed = 2.0f;
    }

    vehicle->state = STATE_MOVING;
    vehicle->turnAngle = 0.0f;
    vehicle->turnProgress = 0.0f;

    // 30% chance to turn
    int turnChance = rand() % 100;
    if (turnChance < 30)
    {
        vehicle->turnDirection = (turnChance < 15) ? TURN_LEFT : TURN_RIGHT;
    }
    else
    {
        vehicle->turnDirection = TURN_NONE;
    }

    // Set dimensions based on direction
    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH)
    {
        vehicle->rect.w = 20; // width
        vehicle->rect.h = 30; // height
    }
    else
    {
        vehicle->rect.w = 30; // width
        vehicle->rect.h = 20; // height
    }

    // Fixed spawn positions for each direction
    switch (direction)
    {
    case DIRECTION_NORTH: // Spawns at bottom, moves up
        if (vehicle->turnDirection == TURN_RIGHT)
        {
            vehicle->canSkipLight = true;
            vehicle->x = INTERSECTION_X - LANE_WIDTH / 2 - 30;
        }
        else
        {
            vehicle->x = INTERSECTION_X - LANE_WIDTH / 2 + 10;
        }
        vehicle->y = WINDOW_HEIGHT - vehicle->rect.h;
        break;

    case DIRECTION_SOUTH: // Spawns at top, moves down
        if (vehicle->turnDirection == TURN_RIGHT)
        {
            vehicle->canSkipLight = true;
            vehicle->x = INTERSECTION_X + 40;
        }
        else
        {
            vehicle->x = INTERSECTION_X + 10;
        }
        vehicle->y = 0;
        break;

    case DIRECTION_EAST: // Spawns at left, moves right
        if (vehicle->turnDirection == TURN_RIGHT)
        {
            vehicle->canSkipLight = true;
            vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2 - 40 + 10;
        }
        else
        {
            vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2 + 10;
        }
        vehicle->x = 0;
        break;

    case DIRECTION_WEST: // Spawns at right, moves left
        vehicle->x = WINDOW_WIDTH - vehicle->rect.w;
        if (vehicle->turnDirection == TURN_RIGHT)
        {
            vehicle->canSkipLight = true;
            vehicle->y = INTERSECTION_Y + 40;
        }
        else
        {
            vehicle->y = INTERSECTION_Y;
        }
        vehicle->isInRightLane = (vehicle->y > INTERSECTION_Y);
        break;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    return vehicle;
}

void updateVehicle(Vehicle *vehicle, TrafficLight *lights)
{
    if (!vehicle->active)
        return;

    float stopLine = 0;
    bool shouldStop = false;
    float stopDistance = 40.0f;
    float turnPoint = 0;
    const float MIN_VEHICLE_DISTANCE = 40.0f;
    bool hasEmergencyPriority = (vehicle->type != REGULAR_CAR);

    // Calculate stop line based on direction
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        stopLine = INTERSECTION_Y + LANE_WIDTH + 40;
        // Check for vehicles ahead in the same lane
        for (int i = 0; i < vehiclesInLane[getVehicleLane(vehicle)]; i++)
        {
            Vehicle *other = laneVehicles[getVehicleLane(vehicle)][i].vehicle;
            if (other != vehicle && other->direction == vehicle->direction)
            {
                float distance = vehicle->y - other->y;
                if (distance > 0 && distance < MIN_VEHICLE_DISTANCE && !vehicle->canSkipLight)
                {
                    shouldStop = true;
                    stopLine = other->y + other->rect.h + 5;
                    break;
                }
            }
        }

        switch (vehicle->turnDirection)
        {
        case TURN_LEFT:
            turnPoint = INTERSECTION_X - LANE_WIDTH - 40;
            break;
        case TURN_RIGHT:
            turnPoint = INTERSECTION_X + LANE_WIDTH + 40;
            break;
        }
        break;
    case DIRECTION_SOUTH:
        stopLine = INTERSECTION_Y - LANE_WIDTH - 40;
        for (int i = 0; i < vehiclesInLane[getVehicleLane(vehicle)]; i++)
        {
            Vehicle *other = laneVehicles[getVehicleLane(vehicle)][i].vehicle;
            if (other != vehicle && other->direction == vehicle->direction)
            {
                float distance = other->y - vehicle->y;
                if (distance > 0 && distance < MIN_VEHICLE_DISTANCE && !vehicle->canSkipLight)
                {
                    shouldStop = true;
                    stopLine = other->y - vehicle->rect.h - 5;
                    break;
                }
            }
        }
        switch (vehicle->turnDirection)
        {
        case TURN_LEFT:
            turnPoint = INTERSECTION_X + LANE_WIDTH + 40;
            break;
        case TURN_RIGHT:
            turnPoint = INTERSECTION_X - LANE_WIDTH - 40;
            break;
        }
        break;
    case DIRECTION_EAST:
        stopLine = INTERSECTION_X - LANE_WIDTH - 40;
        for (int i = 0; i < vehiclesInLane[getVehicleLane(vehicle)]; i++)
        {
            Vehicle *other = laneVehicles[getVehicleLane(vehicle)][i].vehicle;
            if (other != vehicle && other->direction == vehicle->direction)
            {
                float distance = other->x - vehicle->x;
                if (distance > 0 && distance < MIN_VEHICLE_DISTANCE && !vehicle->canSkipLight)
                {
                    shouldStop = true;
                    stopLine = other->x - vehicle->rect.w - 5;
                    break;
                }
            }
        }
        switch (vehicle->turnDirection)
        {
        case TURN_LEFT:
            turnPoint = INTERSECTION_Y + LANE_WIDTH + 40;
            break;
        case TURN_RIGHT:
            turnPoint = INTERSECTION_Y - LANE_WIDTH - 40;
            break;
        }
        break;
    case DIRECTION_WEST:
        stopLine = INTERSECTION_X + LANE_WIDTH + 40;
        for (int i = 0; i < vehiclesInLane[getVehicleLane(vehicle)]; i++)
        {
            Vehicle *other = laneVehicles[getVehicleLane(vehicle)][i].vehicle;
            if (other != vehicle && other->direction == vehicle->direction)
            {
                float distance = vehicle->x - other->x;
                if (distance > 0 && distance < MIN_VEHICLE_DISTANCE && !vehicle->canSkipLight)
                {
                    shouldStop = true;
                    stopLine = other->x + other->rect.w + 5;
                    break;
                }
            }
        }
        switch (vehicle->turnDirection)
        {
        case TURN_LEFT:
            turnPoint = INTERSECTION_Y - LANE_WIDTH - 40;
            break;
        case TURN_RIGHT:
            turnPoint = INTERSECTION_Y + LANE_WIDTH + 40;
            break;
        }
    }

    // Check if vehicle should stop based on traffic lights
    if (!shouldStop && !vehicle->canSkipLight)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            shouldStop = (vehicle->y > stopLine - stopDistance) &&
                         (vehicle->y < stopLine) &&
                         lights[DIRECTION_NORTH].state == RED;
            break;
        case DIRECTION_SOUTH:
            shouldStop = (vehicle->y < stopLine + stopDistance) &&
                         (vehicle->y > stopLine) &&
                         lights[DIRECTION_SOUTH].state == RED;
            break;
        case DIRECTION_EAST:
            shouldStop = (vehicle->x < stopLine + stopDistance) &&
                         (vehicle->x > stopLine) &&
                         lights[DIRECTION_EAST].state == RED;
            break;
        case DIRECTION_WEST:
            shouldStop = (vehicle->x > stopLine - stopDistance) &&
                         (vehicle->x < stopLine) &&
                         lights[DIRECTION_WEST].state == RED;
            break;
        }
    }

    // Update vehicle state based on stopping conditions
    if (shouldStop)
    {
        vehicle->state = STATE_STOPPING;
        vehicle->speed *= 0.8f; // Increased deceleration
        if (vehicle->speed < 0.1f)
        {
            vehicle->state = STATE_STOPPED;
            vehicle->speed = 0;
        }
    }

    else if (vehicle->state == STATE_STOPPED && !shouldStop)
    {
        vehicle->state = STATE_MOVING;
        // Reset speed based on vehicle type
        switch (vehicle->type)
        {
        case AMBULANCE:
        case POLICE_CAR:
            vehicle->speed = 4.0f;
            break;
        case FIRE_TRUCK:
            vehicle->speed = 3.5f;
            break;
        default:
            vehicle->speed = 2.0f;
        }
    }

    // Decrease speed as vehicle approaches turn point
    if (vehicle->state == STATE_MOVING && vehicle->turnDirection != TURN_NONE)
    {
        float distanceToTurnPoint = 0;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
        case DIRECTION_SOUTH:
            distanceToTurnPoint = fabs(vehicle->y - turnPoint);
            break;
        case DIRECTION_EAST:
        case DIRECTION_WEST:
            distanceToTurnPoint = fabs(vehicle->x - turnPoint);
            break;
        }

        if (distanceToTurnPoint < stopDistance)
        {
            vehicle->speed *= 1.0f;
            if (vehicle->speed < 0.5f)
            {
                vehicle->speed = 0.5f;
            }
        }
    }

    // Check if at turning point
    bool atTurnPoint = false;
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        atTurnPoint = vehicle->y <= INTERSECTION_Y;
        break;
    case DIRECTION_SOUTH:
        atTurnPoint = vehicle->y >= INTERSECTION_Y;
        break;
    case DIRECTION_EAST:
        atTurnPoint = vehicle->x >= INTERSECTION_X;
        break;
    case DIRECTION_WEST:
        atTurnPoint = vehicle->x <= INTERSECTION_X;
        break;
    }

    // Start turning if at turn point
    if (atTurnPoint && vehicle->turnDirection != TURN_NONE &&
        vehicle->state != STATE_TURNING && vehicle->state != STATE_STOPPED)
    {
        vehicle->state = STATE_TURNING;
        vehicle->turnAngle = 0.0f;
        vehicle->turnProgress = 0.0f;
    }

    // Movement logic
    float moveSpeed = vehicle->speed;
    if (vehicle->state == STATE_MOVING || vehicle->state == STATE_STOPPING)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->y -= moveSpeed;
            break;
        case DIRECTION_SOUTH:
            vehicle->y += moveSpeed;
            break;
        case DIRECTION_EAST:
            vehicle->x += moveSpeed;
            break;
        case DIRECTION_WEST:
            vehicle->x -= moveSpeed;
            break;
        }
    }
    else if (vehicle->state == STATE_TURNING)
    {
        // Calculate turn angle based on vehicle type
        float turnSpeed = 1.0f;

        vehicle->turnAngle += turnSpeed;
        vehicle->turnProgress = vehicle->turnAngle / 90.0f;
        if (vehicle->turnAngle >= 90.0f)
        {
            vehicle->state = STATE_MOVING;
            vehicle->turnAngle = 0.0f;
            vehicle->turnProgress = 0.0f;
            vehicle->isInRightLane = !vehicle->isInRightLane;
        }

        // Calculate new position based on turn angle
        float turnRadius = 0.5f;
        float turnCenterX = 0;
        float turnCenterY = 0;
        float turnCenter = 15;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            turnCenterX = vehicle->x + (vehicle->isInRightLane ? turnCenter : -turnCenter);
            turnCenterY = vehicle->y;
            ;
            break;
        case DIRECTION_SOUTH:
            turnCenterX = vehicle->x + (vehicle->isInRightLane ? -turnCenter : turnCenter);
            turnCenterY = vehicle->y;
            break;
        case DIRECTION_EAST:
            turnCenterX = vehicle->x;
            turnCenterY = vehicle->y + (!vehicle->isInRightLane ? turnCenter : -turnCenter);
            break;
        case DIRECTION_WEST:
            turnCenterX = vehicle->x;
            turnCenterY = vehicle->y + (!vehicle->isInRightLane ? -turnCenter : turnCenter);
            break;
        }

        float radians = vehicle->turnAngle * M_PI / 180.0f;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->x = turnCenterX + turnRadius * sin(radians);
            vehicle->y = turnCenterY - turnRadius * cos(radians);
            break;
        case DIRECTION_SOUTH:
            vehicle->x = turnCenterX - turnRadius * sin(radians);
            vehicle->y = turnCenterY + turnRadius * cos(radians);
            break;
        case DIRECTION_EAST:
            vehicle->x = turnCenterX + turnRadius * cos(radians);
            vehicle->y = turnCenterY + turnRadius * sin(radians);
            break;
        case DIRECTION_WEST:
            vehicle->x = turnCenterX - turnRadius * cos(radians);
            vehicle->y = turnCenterY - turnRadius * sin(radians);
            break;
        }
    }
    // Update rectangle position
    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    // Check if vehicle has left the screen
    if (vehicle->x < -100 || vehicle->x > WINDOW_WIDTH + 100 ||
        vehicle->y < -100 || vehicle->y > WINDOW_HEIGHT + 100)
    {
        vehicle->active = false;
    }

    // Update rectangle position
    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    // Check if vehicle has left the screen
    if (vehicle->x < -100 || vehicle->x > WINDOW_WIDTH + 100 ||
        vehicle->y < -100 || vehicle->y > WINDOW_HEIGHT + 100)
    {
        vehicle->active = false;
    }
}

void updateLanePositions(Vehicle *vehicles)
{
    // Reset lane tracking
    for (int i = 0; i < 4; i++)
    {
        vehiclesInLane[i] = 0;
    }

    // Update lane positions for active vehicles
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (vehicles[i].active)
        {
            int lane = getVehicleLane(&vehicles[i]);
            float pos;

            // Calculate position along the lane
            switch (vehicles[i].direction)
            {
            case DIRECTION_NORTH:
                pos = vehicles[i].y;
                break;
            case DIRECTION_SOUTH:
                pos = -vehicles[i].y;
                break;
            case DIRECTION_EAST:
                pos = -vehicles[i].x;
                break;
            case DIRECTION_WEST:
                pos = vehicles[i].x;
                break;
            }

            laneVehicles[lane][vehiclesInLane[lane]].position = pos;
            laneVehicles[lane][vehiclesInLane[lane]].vehicle = &vehicles[i];
            vehiclesInLane[lane]++;
        }
    }
}

void renderRoads(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray color for roads

    // Draw the intersection
    SDL_Rect intersection = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, LANE_WIDTH * 2, LANE_WIDTH * 2};
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
    SDL_Rect northStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - STOP_LINE_WIDTH, LANE_WIDTH * 2, STOP_LINE_WIDTH};
    SDL_Rect southStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, LANE_WIDTH * 2, STOP_LINE_WIDTH};
    SDL_Rect eastStop = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, STOP_LINE_WIDTH, LANE_WIDTH * 2};
    SDL_Rect westStop = {INTERSECTION_X - LANE_WIDTH - STOP_LINE_WIDTH, INTERSECTION_Y - LANE_WIDTH, STOP_LINE_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &northStop);
    SDL_RenderFillRect(renderer, &southStop);
    SDL_RenderFillRect(renderer, &eastStop);
    SDL_RenderFillRect(renderer, &westStop);
}

void renderQueues(SDL_Renderer *renderer)
{
    for (int i = 0; i < 4; i++)
    {
        int x = 10 + i * 200; // Adjust position for each lane
        int y = 10;
        Node *current = laneQueues[i].front;
        while (current != NULL)
        {
            SDL_Rect vehicleRect = {x, y, 30, 30};
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for vehicles
            SDL_RenderFillRect(renderer, &vehicleRect);
            y += 40; // Move down for the next vehicle
            current = current->next;
        }
    }
}

void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats)
{
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Brighter background color
    SDL_RenderClear(renderer);

    // Render roads
    renderRoads(renderer);

    // Render traffic lights
    for (int i = 0; i < 4; i++)
    {
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255); // Dark gray for housing
        SDL_RenderFillRect(renderer, &lights[i].position);
        SDL_SetRenderDrawColor(renderer, (lights[i].state == RED) ? 255 : 0, (lights[i].state == GREEN) ? 255 : 0, 0, 255);
        SDL_RenderFillRect(renderer, &lights[i].position);
    }

    // Render vehicles
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (vehicles[i].active)
        {
            SDL_SetRenderDrawColor(renderer, VEHICLE_COLORS[vehicles[i].type].r, VEHICLE_COLORS[vehicles[i].type].g, VEHICLE_COLORS[vehicles[i].type].b, VEHICLE_COLORS[vehicles[i].type].a);
            SDL_RenderFillRect(renderer, &vehicles[i].rect);
        }
    }

    // Render queues
    renderQueues(renderer);

    SDL_RenderPresent(renderer);
}

// Queue functions
void initQueue(Queue *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue *q, Vehicle vehicle)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->vehicle = vehicle;
    newNode->next = NULL;
    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

Vehicle dequeue(Queue *q)
{
    if (q->front == NULL)
    {
        Vehicle emptyVehicle = {0};
        return emptyVehicle;
    }
    Node *temp = q->front;
    Vehicle vehicle = temp->vehicle;
    q->front = q->front->next;
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    free(temp);
    q->size--;
    return vehicle;
}

int isQueueEmpty(Queue *q)
{
    return q->front == NULL;
}