#ifndef HEADER_FILE
#define HEADER_FILE

#define BUFFER_SIZE 51

typedef struct ring_buffer
{
    float contents[BUFFER_SIZE];
    int head;
    int tail;
}
ring_buffer;

float read(ring_buffer *s);

void write(float data, ring_buffer *s);

#endif