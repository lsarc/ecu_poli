#include <stdio.h>
#include <stdlib.h>

#define T 1 // period
#define SAMPLING_SIZE 50 // pelo menos 20 amostras
#define BUFFER_SIZE 51

const float Ts = (float)T/(float)SAMPLING_SIZE; // sample period
const float absPlant = 60.0/T/T/T/T/T;
const float absOutput = 30.0/T/T/T/T/T;

typedef struct ring_buffer
{
    float contents[BUFFER_SIZE];
    int head;
    int tail;
}
ring_buffer;

float read(ring_buffer *s)
{
    float data;
    if (s->head == s->tail)
    {
        printf("buffer underflow\n"); // buffer underflow 
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
        printf("buffer overflow\n"); // buffer overflow 
    }
    else
    {
        s->contents[s->tail] = data; // write data
        s->tail = (s->tail+1) % BUFFER_SIZE; // update tail
    }
}

float fillAbsoluteVectors(float plantVector[SAMPLING_SIZE], float estimatorVector[SAMPLING_SIZE])
{
    float kTs;
    float kTs2;
    for (int k = 1; k < SAMPLING_SIZE-1; k++)
    {
        kTs = k*Ts; // sample weighting
        kTs2 = kTs*kTs;
        plantVector[k] = (T*T - 6*T*kTs + 6*kTs2)*absPlant*Ts;
        estimatorVector[k] = ((T - kTs)*(T - kTs)*kTs2)*absOutput*Ts;
    }
    plantVector[0] = T*T*absPlant*Ts/2;
    estimatorVector[0] = 0;
    kTs = SAMPLING_SIZE*Ts;
    kTs2 = kTs*kTs;
    plantVector[SAMPLING_SIZE-1] = (T*T - 6*T*kTs + 6*kTs2)*absPlant*Ts/2;
    estimatorVector[SAMPLING_SIZE-1] = ((T - kTs)*(T - kTs)*kTs2)*absOutput*Ts/2;
}

float estimator(ring_buffer *Plant, ring_buffer *Estimator, float plantVector[SAMPLING_SIZE], float estimatorVector[SAMPLING_SIZE], float a)
{
    float sumPlant = 0.0;
    float sumEstimator = 0.0;
    float samplePlant;
    float sampleEstimator;
    int k = Plant->head+1;
    for (int n = 1; n < SAMPLING_SIZE-1; n++) // SEPARAR DOIS CALCULOS ?
    {
        if (k > SAMPLING_SIZE - 1)
        {
            k = 0;
        }
        samplePlant = Plant->contents[k];
        sampleEstimator = Estimator->contents[k];
        sumPlant += plantVector[n]*samplePlant;
        sumEstimator += estimatorVector[n]*sampleEstimator;
        k++;  
    }
    
    return sumPlant + sumEstimator*a;
}

float updateSamples(ring_buffer *Plant, ring_buffer *Estimator, float newPlant, float newEstimator)
{
    read(Plant);
    read(Estimator);
    write(newPlant, Plant);
    write(newEstimator, Estimator);
}

void systemTEST(float u, float *y)
{
    *y += u/100;
}

int main()
{
    float setPoint = 10.0;
    float output = 0.0;

    float plantVector[SAMPLING_SIZE];
    float estimatorVector[SAMPLING_SIZE];

    fillAbsoluteVectors(plantVector, estimatorVector);

    ring_buffer bufferPlant;
    bufferPlant.head = 0;
    bufferPlant.tail = 0;

    ring_buffer bufferEstimator;
    bufferEstimator.head = 0;
    bufferEstimator.tail = 0;

    for(int i = 0; i < SAMPLING_SIZE; i++)
    {
        write(0.0, &bufferPlant);
        write(0.0, &bufferEstimator);
    }

    for(int i = 0; i < 20; i++)
    {
        updateSamples(&bufferPlant, &bufferEstimator, 0.0, 0.0);
    }
    

    printf("Estimador Calculado: %f\n", estimator(&bufferPlant, &bufferEstimator, plantVector, estimatorVector, 1.0));
 /* 
    float u;
    for(int i = 0; i < 50; i++)
    {
        u = estimator(bufferPlant.contents, bufferEstimator.contents, 5.0) + (setPoint - output)*5;
        systemTEST(u, &output);
        read(&bufferEstimator);
        read(&bufferPlant);
        write(output, &bufferPlant);
        write(u, &bufferEstimator);
        printf("%f\n", output);
    }
 */
    
    
    return 0;
}
