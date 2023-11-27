#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#define SAMPLING_SIZE 50 // pelo menos 20 amostras
#define BUFFER_SIZE 51

typedef struct ring_buffer
{
    float contents[BUFFER_SIZE];
    int head;
    int tail;
}
ring_buffer;

float readBuffer(ring_buffer *s);

void writeBuffer(float data, ring_buffer *s);

void initializeBuffer(ring_buffer *s);

float newerValue(ring_buffer *s);

#endif
