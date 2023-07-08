#ifndef SCANNER_FUNC_H
#define SCANNER_FUNC_H

#define MAX_DISTANCE_LIDAR_MM 1000
#define PI 3.141592654
#define NUM_OF_MAX_Z_MOVEMENTS 17 // needs to by calculated before change
#define MAX_Z_DISTANCE_MM	125
#define NUM_OF_POINTS_PER_SCAN 80

float *calc_3D_pos(float distance_from_lidar_mm, float r_oz_degree, float h_mm);
int move_xy(void);
int move_z(void);
int move_z_down(void);
#endif //SCANNER_FUNC_H
