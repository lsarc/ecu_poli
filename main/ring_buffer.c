#include "ring_buffer.h"
#include "esp_log.h"

const static char *TAG = "ring_buffer";

float read(ring_buffer *s)
{
    float data;
    if (s->head == s->tail)
    {
        data = 0.0;
        ESP_LOGI(TAG, "Buffer Underflow, tail = %d, head = %d", s->tail, s->head);
    }
    else
    {
        data = s->contents[s->head]; // read data 
        s->head = (s->head+1) % BUFFER_SIZE; // update head 
    }
    return data;
}
void write(float data, ring_buffer *s)
{
    if (( s->tail+1 ) % BUFFER_SIZE == s->head)
    {
        ESP_LOGI(TAG, "Buffer Overflow, tail = %d, head = %d", s->tail, s->head);
    }
    else
    {
        s->contents[s->tail] = data; // write data
        s->tail = (s->tail+1) % BUFFER_SIZE; // update tail
    }
}