#include <stdio.h>
#include <stdlib.h>

#define T 1 // period
#define SAMPLING_SIZE 50 // pelo menos 20 amostras
#define BUFFER_SIZE (SAMPLING_SIZE+1)

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

float readBuffer(ring_buffer *s); // PERCORRE O BUFFER DE ACORDO COM O HEAD E O TAIL

float plantOutput_abs(float y, float kTs)
{
    return (T*T - 6*T*kTs + 6*kTs*kTs)*y;  // MUDAR PARA PREENCHER O VETOR DINAMICAMENTE NO INICIO
}

float estimatorOutput_abs(float u, float kTs)
{
    return (T - kTs)*(T - kTs)*kTs*kTs*u;
}

float estimator(float *u, float *y, float a)
{
    float Ts = (float)T/(float)SAMPLING_SIZE; // sample period
    float kTs;  // sample weighting
    float sum_y = 0.0;
    float sum_u = 0.0;
    for(int k = 1; k < SAMPLING_SIZE-1; k++)
    {
        kTs = k*Ts;
        sum_y += plantOutput_abs(y[k], kTs);
        sum_u += estimatorOutput_abs(u[k], kTs);
    }
    
    sum_y *= Ts;
    sum_u *= Ts;
    sum_y += (plantOutput_abs(y[SAMPLING_SIZE-1], T) + plantOutput_abs(y[0], 0))*Ts/2;
    sum_u += (estimatorOutput_abs(u[SAMPLING_SIZE-1], T) + estimatorOutput_abs(u[0], 0))*Ts/2;
    
    return (60.0*sum_y + 30.0*sum_u*a)/T/T/T/T/T;
}

float updateEstimator(); // ATUALIZA ESTIMADOR UTILIZANDO NOVA AMOSTRA, EXCLUINDO MAIS VELHA

void systemTEST(float u, float *y)
{
    *y += u/100;
}

int main()
{
    float setPoint = 10.0;
    float output = 0.0;
    float vecPlant[SAMPLING_SIZE] = {};
    float vecEstimator[SAMPLING_SIZE] = {};

    for(int i = 0; i < SAMPLING_SIZE; i++)
    {
        vecPlant[i] = 0.0;
        vecEstimator[i] = 0.0;
    } 
    printf("%f\n", estimator(vecPlant, vecEstimator, 1.0));

    /* ring_buffer bufferPlant;
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
    } */

    
    
    return 0;
}
