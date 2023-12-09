#include "ring_buffer.h"
#include "estimador.h"

const float T = 5.0; // period
const float Ts = T/(SAMPLING_SIZE-1); // sample period
const float absPlant = 60.0/T/T/T/T/T;
const float absOutput = -30.0/T/T/T/T/T;

void fillAbsoluteVectors(float *plantArray, float *estimatorArray)
{
    float kTs;
    float kTs2;
    for (int k = 1; k < SAMPLING_SIZE-1; k++)
    {
        kTs = k*Ts; // sample weighting
        kTs2 = kTs*kTs;
        plantArray[k] = (T*T - 6*T*kTs + 6*kTs2)*absPlant*Ts;
        estimatorArray[k] = ((T - kTs)*(T - kTs)*kTs2)*absOutput*Ts;
    }
    plantArray[0] = T*T*absPlant*Ts/2;
    estimatorArray[0] = 0;
    kTs = SAMPLING_SIZE*Ts;
    kTs2 = kTs*kTs;
    plantArray[SAMPLING_SIZE-1] = (T*T - 6*T*kTs + 6*kTs2)*absPlant*Ts/2;
    estimatorArray[SAMPLING_SIZE-1] = ((T - kTs)*(T - kTs)*kTs2)*absOutput*Ts/2;
}

float estimator(ring_buffer *Plant, ring_buffer *Estimator, float *plantArray, float *estimatorArray, float a)
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
        sumPlant += plantArray[n]*samplePlant;
        sumEstimator += estimatorArray[n]*sampleEstimator;
        k++;  
    }
    
    return sumPlant + sumEstimator*a;
}

void updateSamples(ring_buffer *Array, float newSample)
{
    readBuffer(Array);
    writeBuffer(newSample, Array);
}
