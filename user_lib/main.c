/**
 *
 * App 3D scanner using VL53L1X and AR360HB3 360 degree servo multi-device drivers for SWIS course (WEITI PW).
 * Based on https://www.st.com/en/embedded-software/stsw-img013.html VL53L1X ULD
 *
 * @authors Albert Bogdanovic, Konrad Kacper Domian (WEITI PW)
 * @copyright (C) 2023 by Albert Bogdanovic, Konrad Domian
 * License GPL v2
 * konrad.domian.stud<at>pw.edu.pl | konrad.kacper.domian@gmail.com
 * albert.bogdanovic.stud<at>pw.edu.pl | albertbog@gmail.com
 */

/* Includes ------------------------------------------------------------------*/
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "VL53L1X_api.h"
#include "VL53L1X_calibration.h"
#include "vl53l1_platform.h"
#include "scanner_func.h"
/* Defines ------------------------------------------------------------------*/

static uint8_t stop_app = 0;
static uint8_t num_of_move_z = 0;
static uint8_t num_of_angle_points = 0;

uint16_t Dev;
pthread_mutex_t mut_lock;

static void *servo_handler(void *arg)
{
	pthread_mutex_lock(&mut_lock);
	move_xy();
	pthread_mutex_unlock(&mut_lock);

	for (int i = 0; i < NUM_OF_MAX_Z_MOVEMENTS; i++)
	{
		pthread_mutex_lock(&mut_lock);
		move_z();
		pthread_mutex_unlock(&mut_lock);
		num_of_move_z++;
		num_of_angle_points = 0;
		move_xy();
	}
	pthread_mutex_lock(&mut_lock);
	move_z_down();
	pthread_mutex_unlock(&mut_lock);
	stop_app = 1;

	return 0;
}

/* main  --------------------------------------------------------------------*/

int main(int argc, char **argv)
{
	int status;
	uint8_t byteData, sensorState = 0;
	uint16_t wordData;
	VL53L1X_Result_t Results;
	uint8_t first_range = 1;
	pthread_t servo_thread;
	FILE *fp;

	float distance_from_lidar_mm, r_oz_degree, h;
	float *p;

	VL53L1X_UltraLite_Linux_I2C_Init(0, 0, 0); // overwritten VL53L1 API function to initialize 3 sensors
	printf("Init VL53L1X\n");

	for (int ToFSensor = 0; ToFSensor < NUM_OF_DEVICES; ToFSensor++)
	{
		switch (ToFSensor)
		{
		case CENTER_DEVICE:
			Dev = CENTER_DEVICE;
			break;
		case LEFT_DEVICE:
			Dev = LEFT_DEVICE;
			break;
		case RIGHT_DEVICE:
			Dev = RIGHT_DEVICE;
			break;
		}
		status = VL53L1_RdByte(Dev, 0x010F, &byteData);
		printf("VL53L1X Model_ID: %X\n", byteData);
		status += VL53L1_RdByte(Dev, 0x0110, &byteData);
		printf("VL53L1X Module_Type: %X\n", byteData);
		status += VL53L1_RdWord(Dev, 0x010F, &wordData);
		printf("VL53L1X: %X\n", wordData);
		while (sensorState == 0)
		{
			status += VL53L1X_BootState(Dev, &sensorState);
			VL53L1_WaitMs(Dev, 2);
		}
		printf("Chip booted\n");

		status = VL53L1X_SensorInit(Dev);
		/* status += VL53L1X_SetInterruptPolarity(Dev, 0); */
		status += VL53L1X_SetDistanceMode(Dev, 1); /* 1=short, 2=long */
		status += VL53L1X_SetTimingBudgetInMs(Dev, 100);
		status += VL53L1X_SetInterMeasurementInMs(Dev, 100);
		if (status)
		{
			printf("Vl53L1X_%d failed, err = %d\n", Dev, status);
			return -1;
		}
	}

	status += pthread_mutex_init(&mut_lock, NULL);
	status += pthread_create(&servo_thread, NULL, servo_handler, NULL);
	if (status)
	{
		printf("error creating thread\n");
	}

	fp = fopen("3d_data.txt", "w");
	/* read and display data loop */
	while (1)
	{
		if (stop_app)
			break;
		for (int ToFSensor = 0; ToFSensor < NUM_OF_DEVICES; ToFSensor++)
		{
			switch (ToFSensor)
			{
			case CENTER_DEVICE:
				Dev = CENTER_DEVICE;
				break;
			case LEFT_DEVICE:
				Dev = LEFT_DEVICE;
				break;
			case RIGHT_DEVICE:
				Dev = CENTER_DEVICE;
				break;
			}
			pthread_mutex_lock(&mut_lock);

#if defined(POLLING)
			uint8_t dataReady = 0;

			while (dataReady == 0)
			{
				status = VL53L1X_CheckForDataReady(Dev, &dataReady);
				usleep(1);
			}
#else
			status += VL53L1X_StartRanging(Dev);
			status = VL53L1X_UltraLite_WaitForInterrupt(Dev, ST_TOF_IOCTL_WFI);
			if (status)
			{
				printf("ST_TOF_IOCTL_WFI failed, err = %d\n", status);
				return -1;
			}
#endif
			/* Get the data the new way */
			status += VL53L1X_GetResult(Dev, &Results);
			if (status != 0)
			{
				printf("error while getting data\n");
			}

			// printf("Status = %2d, dist = %5d, Ambient = %2d, Signal = %5d, #ofSpads = %5d\n",
			// 	Results.Status, Results.Distance, Results.Ambient,
			// 						Results.SigPerSPAD, Results.NumSPADs);

			printf("VL53L1X_%d Status = %2d, dist = %5d\n", Dev, Results.Status, Results.Distance);

			distance_from_lidar_mm = Results.Distance;
			r_oz_degree = num_of_angle_points * 360 / NUM_OF_POINTS_PER_SCAN;
			num_of_angle_points++;
			h = num_of_move_z * MAX_Z_DISTANCE_MM / NUM_OF_MAX_Z_MOVEMENTS;

			p = calc_3D_pos(distance_from_lidar_mm, r_oz_degree, h);
			if (p != NULL)
			{
				fprintf(fp, "%f, %f, %f\n", p[0], p[1], p[2]);
				printf("X: %f, Y: %f, Z: %f\n", p[0], p[1], p[2]);
			}
			free(p);

			/* trigger next ranging */
			status += VL53L1X_ClearInterrupt(Dev);
			if (first_range)
			{
				/* very first measurement shall be ignored
				 * thus requires twice call
				 */
				status += VL53L1X_ClearInterrupt(Dev);
				first_range = 0;
			}
			pthread_mutex_unlock(&mut_lock);
		}
	}
	fclose(fp);
	return 0;
}
