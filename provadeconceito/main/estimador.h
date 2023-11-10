#include "ring_buffer.h"

void fillAbsoluteVectors(float *plantArray, float *estimatorArray);

float estimator(ring_buffer *Plant, ring_buffer *Estimator, float *plantArray, float *estimatorArray, float a);

void updateSamples(ring_buffer *Plant, ring_buffer *Estimator, float newPlant, float newEstimator);
