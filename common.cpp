#include "common.h"
#include <cstdlib>

const int PARTICLE_COUNT = 500;
const float TIMESTEP = 1;
const int SOFTENING_LENGTH = 10;

const float OMEGA = 0.5;
const float PI = 3.141592;
const float G = 6.67384e-11;
const int MIN_MASS = 4000;
const int MAX_MASS = 6000;

const float COORDINATE_MIN_VALUE = -1.4;
const float COORDINATE_MAX_VALUE = 1.4;

const float WINDOW_WIDTH = 800;
const float WINDOW_HEIGHT = 600;

// Generates a random float using an uniform distribution.
float randUniform()
{
    return (float)rand() / (float)RAND_MAX ;
}
