
#include <stdio.h>
#include "esp_system.h"
#include "cJSON.h"

/*
 *  the incoming json has this form
 *  {
 *   "datapoints": 10,
 *   "type": "scatter",
 *   "offset": 100
 *  }
 *
 *  This function expects the json in this exact form.
 *  If you need to respond to another json object, you need to alter this function.
 *
 */
/*
 * data: the incoming json
 * returns the response payload to be sent
 */
char* process_incoming_websocket_frame_payload(char *data) {
	printf("Data: %s\n", data);
	cJSON *root = cJSON_Parse(data);
	if (root != NULL) {
		printf("root not null\n");
	} else {
		printf("root is null\n");
		return NULL;
	}

	cJSON *datapoints = cJSON_GetObjectItemCaseSensitive(root, "datapoints");
	if (cJSON_IsNumber(datapoints)) {
		printf("datapoints:%d\n", datapoints->valueint);
	}

	cJSON *offset = cJSON_GetObjectItemCaseSensitive(root, "offset");
	if (cJSON_IsNumber(offset)) {
		printf("offset:%d\n", offset->valueint);
	}

	cJSON *plot_type = cJSON_GetObjectItemCaseSensitive(root, "type");
	if (plot_type) {
		printf("plot_type:%s\n", plot_type->valuestring);
	}


	// Generate some dummy data for X
	int number_of_datapoints = datapoints->valueint;
	uint16_t x_axis[number_of_datapoints];
	for (int i = 1; i <= number_of_datapoints; i++) {
		x_axis[i - 1] = i;
	}

	// Generate some dummy data for Y
	uint16_t y_axis[number_of_datapoints];
	for (int i = 0; i < number_of_datapoints; i++) {
		y_axis[i] = 110 + i;
	}

	//cJSON *x = cJSON_CreateIntArray(x_axis, number_of_datapoints);
	//cJSON *y = cJSON_CreateIntArray(y_axis, number_of_datapoints);
	cJSON *x = cJSON_CreateArray();
	for (int i = 0; i < number_of_datapoints; i++) {
		cJSON *x_num = cJSON_CreateNumber((double) x_axis[i]);
		cJSON_AddItemToArray(x, x_num);
	}
	cJSON *y = cJSON_CreateArray();
	for (int i = 0; i < number_of_datapoints; i++) {
		cJSON *y_num = cJSON_CreateNumber((double) y_axis[i]);
		cJSON_AddItemToArray(y, y_num);
	}




	cJSON_AddItemToObject(root, "x", x);
	cJSON_AddItemToObject(root, "y", y);

	//printf("%s\n", cJSON_Print(root));
	char *json = cJSON_Print(root);
	//printf("JSON:%s\n JSON Length:%d\n", json, strlen(json));
	cJSON_Delete(root);
	return json;
}
