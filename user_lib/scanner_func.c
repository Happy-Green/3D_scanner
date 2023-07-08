#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "scanner_func.h"

float *calc_3D_pos(float distance_from_lidar_mm, float r_oz_degree, float h_mm)
{

    float *pos = malloc(sizeof(float) * 3);

    if (distance_from_lidar_mm > MAX_DISTANCE_LIDAR_MM)
    {
        pos = NULL;
        return pos;
    }

    float y_temp = 0.0;
    float x_temp = MAX_DISTANCE_LIDAR_MM / 2 - distance_from_lidar_mm;
    float z_temp = h_mm;

    float r_oz_radians = r_oz_degree * PI / 180;
    float x = cos(r_oz_radians) * x_temp - sin(r_oz_radians) * y_temp;
    float y = sin(r_oz_radians) * x_temp + cos(r_oz_radians) * y_temp;
    float z = z_temp;

    pos[0] = x;
    pos[1] = y;
    pos[2] = z;

    return pos;
}

int move_xy()
{
    char *command_slow = "echo SLOW > /proc/servo_pwm_xy";
    char *command_stop = "echo STOP > /proc/servo_pwm_xy";
    system(command_slow);
    usleep(7.875 * 1000000);
    system(command_stop);
    return 0;
}

int move_z()
{
    char *command_slow = "echo SLOW > /proc/servo_pwm_z";
    char *command_stop = "echo STOP > /proc/servo_pwm_z";
    system(command_slow);
    usleep(1 * 1000000);
    system(command_stop);
    return 0;
}

int move_z_down()
{
    char *command_fast = "echo FAST > /proc/servo_pwm_z";
    char *command_stop = "echo STOP > /proc/servo_pwm_z";
    system(command_fast);
    usleep(11 * 1000000);
    system(command_stop);
    return 0;
}