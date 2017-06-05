#include "Particle.h"
#include "Cell.h"
#include <cmath>


Particle::Particle() {};

Particle::Particle(float x, float y, float z, float vX, float vY, float vZ, float mass) :
    x(x), y(y), z(z), vX(vX), vY(vY), vZ(vZ), mass(mass) {};

Particle::Particle(const Particle &obj)
{
    this->x = obj.x;
    this->y = obj.y;
    this->z = obj.z;
    this->vX = obj.vX;
    this->vY = obj.vY;
    this->vZ = obj.vZ;
    this->mass = obj.mass;
}

void Particle::setCoordinates(float x, float y, float z)
{
    this->x = x; this->y = y; this->z = z;
}


void Particle::setSpeed(float vX, float vY, float vZ)
{
    this->vX = vX; this->vY = vY; this->vZ = vZ;
}


void Particle::setMass(float mass)
{
    this->mass = mass;
}

// Creates n particles inside the particles vector with a plummer sphere density.
// a is the softening length.
// G is the gravitational constant.
void Particle::plummerSphereDensity(std::vector<Particle>& particles, int n, int a, float G)
{
    assert(particles.size() == 0);

    float xMin = 99999, xMax = -999999, yMin = 999999, yMax = -999999, zMin = 999999, zMax = -999999;

    float radius, xx, yy, v, phi, theta, xNew, yNew, zNew, vXNew, vYNew, vZNew, mass;
    int nGeneratedParticles = 0;

    while(nGeneratedParticles < n)
    {
        radius = a / std::sqrt((float)pow(randUniform(), -((float)2/3)) - 1);
        xx = randUniform();
        yy = (float)(randUniform() * 0.1);

        if(yy < pow(xx, 2) * pow((1 - pow(xx, 2)), 3.5))
        {
            nGeneratedParticles++;
            v = xx * sqrt(2*G*n) * pow((pow(radius,2) + pow(a,2)), (-0.25));

            phi = randUniform() * 2 * PI;
            theta = (float)acos(randUniform()*2 - 1);

            xNew = radius * (float)sin(theta) * (float)cos(phi);
            yNew = radius * (float)sin(theta) * (float)sin(phi);
            zNew = radius * (float)cos(theta);

            if(xNew > xMax)
            {
                xMax = xNew;
            }
            if(xNew < xMin)
            {
                xMin = xNew;
            }
            if(yNew > yMax)
            {
                yMax = yNew;
            }
            if(yNew < yMin)
            {
                yMin = yNew;
            }
            if(zNew > zMax)
            {
                zMax = zNew;
            }
            if(zNew < zMin)
            {
                zMin = zNew;
            }

            phi = randUniform() * 2 * PI;
            theta = (float)acos(randUniform()*2 - 1);
            vXNew = v * (float)sin(theta) * (float)cos(phi);
            vYNew = v * (float)sin(theta) * (float)sin(phi);
            vZNew = v * (float)cos(theta);

            mass = MIN_MASS + rand()% (MAX_MASS - MIN_MASS + 1);

            Particle newParticle(xNew, yNew, zNew, vXNew, vYNew, vZNew, mass);
            particles.push_back(newParticle);
        }
    }

    for(int i=0; i<particles.size(); i++)
    {
        particles[i].scale(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, xMin, xMax, yMin, yMax, zMin, zMax);
    }
}

// Scales down the particle's coordinates, making it fit between scalMin and scaleMax.
void Particle::scale(float scaleMin, float scaleMax, float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    // Add some padding
    scaleMin += 0.01;
    scaleMax -= 0.01;

    this->x = ((scaleMax - scaleMin) * (this->x - xMin))/(xMax - xMin) + scaleMin;
    this->y = ((scaleMax - scaleMin) * (this->y - yMin))/(yMax - yMin) + scaleMin;
    this->z = ((scaleMax - scaleMin) * (this->z - zMin))/(zMax - zMin) + scaleMin;
}

// Changes the velocity of the particle after the interaction with the cell.
void Particle::forcePush(Cell* cell, float timeDelta)
{


// OLD FORMULA
//    float dX = this->x - cell->xCenter;
//    float dY = this->y - cell->yCenter;
//    float dZ = this->z - cell->zCenter;
//
//    float r = (float)pow(dX * dX + dY * dY + dZ * dZ , 0.5);
//
//    float fX = ((G * this->mass * cell->totalMass)/(float)pow(r,2)) * (dX/r);
//    float fY = ((G * this->mass * cell->totalMass)/(float)pow(r,2)) * (dY/r);
//    float fZ = ((G * this->mass * cell->totalMass)/(float)pow(r,2)) * (dZ/r);
//
//    this->vX = this->vX + fX * timeDelta/this->mass;
//    this->vY = this->vY + fY * timeDelta/this->mass;
//    this->vZ = this->vZ + fZ * timeDelta/this->mass;
}

// Updates the particle position
void Particle::updatePosition(float timeDelta)
{
    this->x += this->vX * timeDelta;
    this->y += this->vY * timeDelta;
    this->z += this->vZ * timeDelta;
}
