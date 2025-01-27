#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"

// Global queues for lanes
Queue laneQueues[4];         // Queues for lanes A, B, C, D
int lanePriorities[4] = {0}; // Priority levels for lanes (0 = normal, 1 = high)

const SDL_Color VEHICLE_COLORS[] = {
    {0, 0, 255, 255}, // REGULAR_CAR: Blue
    {255, 0, 0, 255}, // AMBULANCE: Red
    {0, 0, 128, 255}, // POLICE_CAR: Dark Blue
    {255, 69, 0, 255} // FIRE_TRUCK: Orange-Red
};

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
    Uint32 currentTicks = SDL_GetTicks();
    static Uint32 lastUpdateTicks = 0;

    if (currentTicks - lastUpdateTicks >= 5000)
    { // Change lights every 5 seconds
        lastUpdateTicks = currentTicks;

        // Check for high-priority lanes
        for (int i = 0; i < 4; i++)
        {
            if (laneQueues[i].size > 10)
            {
                lanePriorities[i] = 1; // Set high priority
            }
            else if (laneQueues[i].size < 5)
            {
                lanePriorities[i] = 0; // Reset to normal priority
            }
        }

        // Toggle lights based on priority
        for (int i = 0; i < 4; i++)
        {
            if (lanePriorities[i] == 1)
            {
                lights[i].state = GREEN; // Give green light to high-priority lane
            }
            else
            {
                lights[i].state = (lights[i].state == RED) ? GREEN : RED; // Toggle lights
            }
        }
    }
}

Vehicle *createVehicle(Direction direction)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;

    // Adjust vehicle type probability
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
    // Adjust speed based on vehicle type
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

    // Random lane selection (truly random between left and right lanes)
    bool rightLane = (rand() % 2) == 0;

    // Randomly decide if vehicle will turn (30% chance)
    int turnChance = rand() % 100;
    if (turnChance < 30)
    {
        vehicle->turnDirection = (turnChance < 15) ? TURN_LEFT : TURN_RIGHT;
        // Force lane selection based on turn direction
        rightLane = (vehicle->turnDirection == TURN_RIGHT);
    }
    else
    {
        vehicle->turnDirection = TURN_NONE;
    }

    // Spawn position based on direction and lane
    switch (direction)
    {
    case DIRECTION_NORTH:
        vehicle->x = INTERSECTION_X + (rightLane ? LANE_WIDTH / 3 : -LANE_WIDTH / 3);
        vehicle->y = WINDOW_HEIGHT - 20;
        vehicle->rect.w = 20;
        vehicle->rect.h = 30;
        break;
    case DIRECTION_SOUTH:
        vehicle->x = INTERSECTION_X + (rightLane ? -LANE_WIDTH / 3 : LANE_WIDTH / 3);
        vehicle->y = 20;
        vehicle->rect.w = 20;
        vehicle->rect.h = 30;
        break;
    case DIRECTION_EAST:
        vehicle->x = 20;
        vehicle->y = INTERSECTION_Y + (rightLane ? LANE_WIDTH / 3 : -LANE_WIDTH / 3);
        vehicle->rect.w = 30;
        vehicle->rect.h = 20;
        break;
    case DIRECTION_WEST:
        vehicle->x = WINDOW_WIDTH - 20;
        vehicle->y = INTERSECTION_Y + (rightLane ? -LANE_WIDTH / 3 : LANE_WIDTH / 3);
        vehicle->rect.w = 30;
        vehicle->rect.h = 20;
        break;
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;
    vehicle->isInRightLane = rightLane;
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
    bool hasEmergencyPriority = (vehicle->type != REGULAR_CAR);

    // Calculate stop line and intersection points
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        stopLine = INTERSECTION_Y + LANE_WIDTH;
        turnPoint = INTERSECTION_Y + LANE_WIDTH / 4;
        break;
    case DIRECTION_SOUTH:
        stopLine = INTERSECTION_Y - LANE_WIDTH;
        turnPoint = INTERSECTION_Y - LANE_WIDTH / 4;
        break;
    case DIRECTION_EAST:
        stopLine = INTERSECTION_X - LANE_WIDTH;
        turnPoint = INTERSECTION_X - LANE_WIDTH / 4;
        break;
    case DIRECTION_WEST:
        stopLine = INTERSECTION_X + LANE_WIDTH;
        turnPoint = INTERSECTION_X + LANE_WIDTH / 4;
        break;
    }

    // Check if vehicle should stop based on traffic lights
    if (!hasEmergencyPriority)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            shouldStop = vehicle->y > stopLine &&
                         vehicle->y < stopLine + stopDistance &&
                         lights[DIRECTION_NORTH].state == RED;
            break;
        case DIRECTION_SOUTH:
            shouldStop = vehicle->y < stopLine &&
                         vehicle->y > stopLine - stopDistance &&
                         lights[DIRECTION_SOUTH].state == RED;
            break;
        case DIRECTION_EAST:
            shouldStop = vehicle->x < stopLine &&
                         vehicle->x > stopLine - stopDistance &&
                         lights[DIRECTION_EAST].state == RED;
            break;
        case DIRECTION_WEST:
            shouldStop = vehicle->x > stopLine &&
                         vehicle->x < stopLine + stopDistance &&
                         lights[DIRECTION_WEST].state == RED;
            break;
        }
    }

    // Update vehicle state based on stopping conditions
    if (shouldStop)
    {
        vehicle->state = STATE_STOPPING;
        vehicle->speed *= 0.9f;
        if (vehicle->speed < 0.1f)
        {
            vehicle->state = STATE_STOPPED;
            vehicle->speed = 0;
        }
    }
    else if (vehicle->state == STATE_STOPPED)
    {
        vehicle->state = STATE_MOVING;
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

    // Check if at turning point
    bool atTurnPoint = false;
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        atTurnPoint = vehicle->y <= turnPoint;
        break;
    case DIRECTION_SOUTH:
        atTurnPoint = vehicle->y >= turnPoint;
        break;
    case DIRECTION_EAST:
        atTurnPoint = vehicle->x >= turnPoint;
        break;
    case DIRECTION_WEST:
        atTurnPoint = vehicle->x <= turnPoint;
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
        float turnSpeed = moveSpeed * 0.7f;
        vehicle->turnProgress += turnSpeed * 0.05f;

        // Calculate turn trajectory
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            if (vehicle->turnDirection == TURN_LEFT)
            {
                float turnRadius = LANE_WIDTH * 1.5f;
                float centerX = INTERSECTION_X - LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y - LANE_WIDTH / 2;
                vehicle->x = centerX - turnRadius * cosf(vehicle->turnProgress * 3.14159f / 2);
                vehicle->y = centerY - turnRadius * sinf(vehicle->turnProgress * 3.14159f / 2);
            }
            else
            {
                float turnRadius = LANE_WIDTH * 0.5f;
                float centerX = INTERSECTION_X + LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y - LANE_WIDTH / 2;
                vehicle->x = centerX - turnRadius * cosf((1 - vehicle->turnProgress) * 3.14159f / 2);
                vehicle->y = centerY + turnRadius * sinf((1 - vehicle->turnProgress) * 3.14159f / 2);
            }
            break;

        case DIRECTION_SOUTH:
            if (vehicle->turnDirection == TURN_LEFT)
            {
                float turnRadius = LANE_WIDTH * 1.5f;
                float centerX = INTERSECTION_X - LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y + LANE_WIDTH / 2;
                vehicle->x = centerX + turnRadius * cosf(vehicle->turnProgress * 3.14159f / 2);
                vehicle->y = centerY + turnRadius * sinf(vehicle->turnProgress * 3.14159f / 2);
            }
            else
            {
                float turnRadius = LANE_WIDTH * 0.5f;
                float centerX = INTERSECTION_X + LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y + LANE_WIDTH / 2;
                vehicle->x = centerX + turnRadius * cosf((1 - vehicle->turnProgress) * 3.14159f / 2);
                vehicle->y = centerY - turnRadius * sinf((1 - vehicle->turnProgress) * 3.14159f / 2);
            }
            break;

        case DIRECTION_EAST:
            if (vehicle->turnDirection == TURN_LEFT)
            {
                float turnRadius = LANE_WIDTH * 1.5f;
                float centerX = INTERSECTION_X + LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y - LANE_WIDTH / 2;
                vehicle->x = centerX + turnRadius * sinf(vehicle->turnProgress * 3.14159f / 2);
                vehicle->y = centerY + turnRadius * cosf(vehicle->turnProgress * 3.14159f / 2);
            }
            else
            {
                float turnRadius = LANE_WIDTH * 0.5f;
                float centerX = INTERSECTION_X + LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y + LANE_WIDTH / 2;
                vehicle->x = centerX - turnRadius * sinf((1 - vehicle->turnProgress) * 3.14159f / 2);
                vehicle->y = centerY + turnRadius * cosf((1 - vehicle->turnProgress) * 3.14159f / 2);
            }
            break;

        case DIRECTION_WEST:
            if (vehicle->turnDirection == TURN_LEFT)
            {
                float turnRadius = LANE_WIDTH * 1.5f;
                float centerX = INTERSECTION_X - LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y + LANE_WIDTH / 2;
                vehicle->x = centerX - turnRadius * sinf(vehicle->turnProgress * 3.14159f / 2);
                vehicle->y = centerY - turnRadius * cosf(vehicle->turnProgress * 3.14159f / 2);
            }
            else
            {
                float turnRadius = LANE_WIDTH * 0.5f;
                float centerX = INTERSECTION_X - LANE_WIDTH / 2;
                float centerY = INTERSECTION_Y - LANE_WIDTH / 2;
                vehicle->x = centerX + turnRadius * sinf((1 - vehicle->turnProgress) * 3.14159f / 2);
                vehicle->y = centerY - turnRadius * cosf((1 - vehicle->turnProgress) * 3.14159f / 2);
            }
            break;
        }

        // Complete the turn
        if (vehicle->turnProgress >= 1.0f)
        {
            vehicle->state = STATE_MOVING;
            vehicle->turnDirection = TURN_NONE;
            // Align vehicle to the lane after turning
            switch (vehicle->direction)
            {
            case DIRECTION_NORTH:
                vehicle->direction = (vehicle->turnDirection == TURN_LEFT) ? DIRECTION_WEST : DIRECTION_EAST;
                break;
            case DIRECTION_SOUTH:
                vehicle->direction = (vehicle->turnDirection == TURN_LEFT) ? DIRECTION_EAST : DIRECTION_WEST;
                break;
            case DIRECTION_EAST:
                vehicle->direction = (vehicle->turnDirection == TURN_LEFT) ? DIRECTION_NORTH : DIRECTION_SOUTH;
                break;
            case DIRECTION_WEST:
                vehicle->direction = (vehicle->turnDirection == TURN_LEFT) ? DIRECTION_SOUTH : DIRECTION_NORTH;
                break;
            }
            // Align vehicle to the lane after turning
            switch (vehicle->direction)
            {
            case DIRECTION_NORTH:
            case DIRECTION_SOUTH:
                vehicle->x = INTERSECTION_X + (vehicle->isInRightLane ? LANE_WIDTH / 3 : -LANE_WIDTH / 3);
                break;
            case DIRECTION_EAST:
            case DIRECTION_WEST:
                vehicle->y = INTERSECTION_Y + (vehicle->isInRightLane ? LANE_WIDTH / 3 : -LANE_WIDTH / 3);
                break;
            }
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