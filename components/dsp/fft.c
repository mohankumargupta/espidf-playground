#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/err.h"

#define ISGN_FORWARD 1
#define ISGN_REVERSE 0

#define NMAX 8192
#define NMAXSQRT 64


// Declare Ooura FFT library for real DFT
void rdft(int, int, double *, int *, double *);

void fft(void *pvParameters) {
	const int n = 8;
	//const int n = 2;
	int ip[NMAXSQRT + 2];
	double a[NMAX + 1], w[NMAX * 5 / 4];

	for (int i = 0; i < 16; i++) {
		ip[i] = 0;
	}

	/*
	 for (int i=0; i<8; i++) {
	 a[i] = i;
	 }
	 */

	/*
	 a[0] = 1;
	 a[1] = 1;
	 a[2] = 0;
	 a[3] = 0;
	 */

	/*
	 for (int i=0; i<4; i++) {
	 a[i] = 1;
	 }

	 for (int i=4; i<8; i++) {
	 a[i] = -1;
	 }
	 */

	a[0] = 1;
	for (int i = 1; i < 8; i++) {
		a[i] = 0;
	}


    rdft(n, ISGN_FORWARD, a, ip, w);
	for (int i = 0; i < n; i++) {
		printf("a:%f,ip:%d,w:%f\n", a[i], ip[i], w[i]);

	}
	printf("--INTEPRETATION--\n");
	double dc = a[0];
	double center = a[1];
	printf("dc:%f\n", dc);
	for (int i = 2; i < n / 2 + 1; i = i + 2) {
		double mag = sqrt(a[i] * a[i] + a[i + 1] * a[i + 1]);
		printf("%f. %fi mag:%f\n", a[i], a[i + 1], mag);
	}

	for (int i = n / 2 + 2; i < 8; i = i + 2) {
		double mag = sqrt(a[i] * a[i] + a[i + 1] * a[i + 1]);
		printf("%f. %fi mag:%f\n", a[i], a[i + 1], mag);
	}

	printf("center FFT bin:%f\n", center);


	vTaskDelete(NULL);
}
