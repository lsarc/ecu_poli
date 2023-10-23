#include "ring_buffer.h"
#include "estimador.h"

const float T = 1.0; // period
const float Ts = T/SAMPLING_SIZE; // sample period
const float absPlant = 60.0/T/T/T/T/T;
const float absOutput = 30.0/T/T/T/T/T;

float read(ring_buffer *s)
{
    float data;
    if (s->head == s->tail)
    {
        // buffer underflow 
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
        // buffer overflow 
    }
    else
    {
        s->contents[s->tail] = data; // write data
        s->tail = (s->tail+1) % BUFFER_SIZE; // update tail
    }
}

void fillAbsoluteVectors(float plantVector[SAMPLING_SIZE], float estimatorVector[SAMPLING_SIZE])
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

void updateSamples(ring_buffer *Plant, ring_buffer *Estimator, float newPlant, float newEstimator)
{
    read(Plant);
    read(Estimator);
    write(newPlant, Plant);
    write(newEstimator, Estimator);
}
