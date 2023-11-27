#include "ring_buffer.h"

#ifndef ESTIMADOR_H_
#define ESTIMADOR_H_


void fillAbsoluteVectors(float *plantArray, float *estimatorArray);

float estimator(ring_buffer *Plant, ring_buffer *Estimator, float *plantArray, float *estimatorArray, float a);

void updateSamples(ring_buffer *Plant, ring_buffer *Estimator, float newPlant, float newEstimator);

#endif
