#include "ring_buffer.h"
#include "esp_log.h"

const static char *TAG = "ring_buffer";

float readBuffer(ring_buffer *s)
{
    float data;
    if (s->head == s->tail)
    {
        data = 0.0;
        ESP_LOGI(TAG, "Buffer Underflow");
    }
    else
    {
        data = s->contents[s->head]; // read data 
        s->head = (s->head+1) % BUFFER_SIZE; // update head 
    }
    return data;
}
void writeBuffer(float data, ring_buffer *s)
{
    if (( s->tail+1 ) % BUFFER_SIZE == s->head)
    {
    	ESP_LOGI(TAG, "Buffer Overflow");
    }
    else
    {
        s->contents[s->tail] = data; // write data
        s->tail = (s->tail+1) % BUFFER_SIZE; // update tail
    }
}

void initializeBuffer(ring_buffer *s)
{
	s->tail = 0;
	s->head = 0;

	for (int i = 0; i < SAMPLING_SIZE; i++)
	{
		writeBuffer(0.0, s);
	}
}

float newerValue(ring_buffer *s)
{
	int tail = s->tail;
	int i;
	if (tail == 0){
		i = SAMPLING_SIZE;
	}
	else{
		i = tail - 1;
	}
	return s->contents[i];
}
