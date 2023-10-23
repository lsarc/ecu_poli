#include "ring_buffer.h"


const int SAMPLING_SIZE = 50; // pelo menos 20 amostras

void fillAbsoluteVectors(float plantVector[SAMPLING_SIZE], float estimatorVector[SAMPLING_SIZE]);

float estimator(ring_buffer *Plant, ring_buffer *Estimator, float plantVector[SAMPLING_SIZE], float estimatorVector[SAMPLING_SIZE], float a);

void updateSamples(ring_buffer *Plant, ring_buffer *Estimator, float newPlant, float newEstimator);
