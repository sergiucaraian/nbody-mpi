#include "Particle.h"
#include "Cell.h"
#include <cmath>


Particle::Particle() {};

Particle::Particle(float x, float y, float z, float vX, float vY, float vZ, float mass) :
    x(x), y(y), z(z), vX(vX), vY(vY), vZ(vZ), mass(mass) {};


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


void Particle::plummerSphereDensity(std::vector<Particle>& particles, int n, int a, float G)
{
    assert(particles.size() == 0);

    float xMin = 99999, xMax = -999999, yMin = 999999, yMax = -999999, zMin = 999999, zMax = -999999;

    float radius, xx, yy, v, phi, theta, xNew, yNew, zNew, vXNew, vYNew, vZNew, mass;
    int nGeneratedParticles = 0;

    while(nGeneratedParticles < n)
    {
        radius = a/ std::sqrt(pow(randUniform(), -((float)2/3)) - 1);
        xx = randUniform();
        yy = randUniform() * 0.1;

        if(yy < pow(xx, 2) * pow((1 - pow(xx, 2)), 3.5))
        {
            nGeneratedParticles++;
            v = xx * sqrt(2*G*n) * pow((pow(radius,2) + pow(a,2)), (-0.25));

            phi = randUniform() * 2 * PI;
            theta = acos(randUniform()*2 - 1);

            xNew = radius * sin(theta) * cos(phi);
            yNew = radius * sin(theta) * sin(phi);
            zNew = radius * cos(theta);

            if(xNew > xMax)
            {
                xMax = xNew;
            }
            else if(xNew < xMin)
            {
                xMin = xNew;
            }
            else if(yNew > yMax)
            {
                yMax = yNew;
            }
            else if(yNew < yMin)
            {
                yMin = yNew;
            }
            else if(zNew > zMax)
            {
                zMax = zNew;
            }
            else if(zNew < zMin)
            {
                zMin = zNew;
            }

            phi = randUniform() * 2 * PI;
            theta = acos(randUniform()*2 - 1);
            vXNew = v * sin(theta) * cos(phi);
            vYNew = v * sin(theta) * sin(phi);
            vZNew = v * cos(theta);

            mass = rand()%MAX_MASS + MIN_MASS;

            Particle newParticle(xNew, yNew, zNew, vXNew, vYNew, vZNew, mass);
            particles.push_back(newParticle);
        }
    }

    for(int i=0; i<particles.size(); i++)
    {
        particles[i].scale(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, xMin, xMax, yMin, yMax, zMin, zMax);
    }
}

void Particle::scale(float scaleMin, float scaleMax, float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    this->x = ((scaleMax - scaleMin)*(this->x - xMin) / (xMax - xMin)) + scaleMin;
    this->y = ((scaleMax - scaleMin)*(this->y - yMin) / (yMax - yMin)) + scaleMin;
    this->z = ((scaleMax - scaleMin)*(this->z - zMin) / (zMax - zMin)) + scaleMin;
}


void Particle::forcePush(Cell* cell, float timeDelta)
{
    float dX = this->x - cell->xCenter;
    float dY = this->y - cell->yCenter;
    float dZ = this->z - cell->zCenter;

    float r = pow(dX * dX + dY * dY + dZ * dZ , 0.5);

    float fX = ((G * this->mass * cell->totalMass)/pow(r,2)) * (dX/r);
    float fY = ((G * this->mass * cell->totalMass)/pow(r,2)) * (dY/r);
    float fZ = ((G * this->mass * cell->totalMass)/pow(r,2)) * (dZ/r);

    this->vX = this->vX + fX * timeDelta/this->mass;
    this->vY = this->vY + fY * timeDelta/this->mass;
    this->vZ = this->vZ + fZ * timeDelta/this->mass;
}


void Particle::updatePosition(float timeDelta)
{
    this->x = this->x + this->vX * timeDelta;
    this->y = this->y + this->vY * timeDelta;
    this->z = this->z + this->vZ * timeDelta;
}
