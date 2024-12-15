#include <stdlib.h>
#ifndef RXRAND_H
#define RXRAND_H

#define DLL_EXPORT __declspec(dllexport)

inline int DLL_EXPORT RXGetRandom(int min, int max) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = max - min;
	float r = random * diff;
	return min + r;
}

inline float DLL_EXPORT RXGetRandom(float min, float max) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = max - min;
	float r = random * diff;
	return min + r;
}


#endif