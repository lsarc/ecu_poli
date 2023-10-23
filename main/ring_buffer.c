#include "ring_buffer.h"

float read(ring_buffer *s)
{
    float data;
    if (s->head == s->tail)
    {

        data = 0.0;
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

    }
    else
    {
        s->contents[s->tail] = data; // write data
        s->tail = (s->tail+1) % BUFFER_SIZE; // update tail
    }
}