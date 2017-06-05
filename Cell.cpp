#include "Cell.h"
#include "Particle.h"
#include <cmath>


Cell::Cell() {};


Cell::Cell(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax) :
        xMin(xMin), xMax(xMax), yMin(yMin), yMax(yMax), zMin(zMin), zMax(zMax)
{
    this->particle = nullptr;
    this->particleCount  = 0;
    this->totalMass = 0;
    this->xCenter = 0;
    this->yCenter = 0;
    this->zCenter = 0;
};


Cell::Cell(const Cell &obj)
{
    this->xMin = obj.xMin;
    this->xMax = obj.xMax;
    this->yMin = obj.yMin;
    this->yMax = obj.yMax;
    this->zMin = obj.zMin;
    this->zMax = obj.zMax;

    this->particle = obj.particle;
    this->particleCount = obj.particleCount;
    this->totalMass = obj.totalMass;
    this->xCenter = obj.xCenter;
    this->yCenter = obj.yCenter;
    this->zCenter = obj.zCenter;
}


void Cell::setCoordinates(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    this->xMin = xMin; this->xMax = xMax;
    this->yMin = yMin; this->yMax = yMax;
    this->zMin = zMin; this->zMax = zMax;
}


bool Cell::isInsideCell(float x, float y, float z)
{
    return this->xMin < x && this->xMax >= x
           && this->yMin < y && this->yMax >= y
           && this->zMin < z && this->zMax >= z;
}


// Insert the particle in the Cell.
// This only works if the particle's coordinates fall between the cell boundaries.
// Otherwise the operation is ignored.
void Cell::insertParticle(Particle* particle)
{
    if(!this->isInsideCell(particle->x, particle->y, particle->z))
    {
        return;
    }

    if(this->particleCount > 0)
    {
        if(this->particleCount == 1)
        {
            this->expandChildren();
            for(int i=0; i<8; i++)
            {
                this->children[i]->insertParticle(this->particle);
            }
            this->particle = nullptr;
        }

        for(int i=0; i<8; i++)
        {
            this->children[i]->insertParticle(particle);
        }
    }
    else
    {
        this->particle = particle;
    }

    this->xCenter = (this->totalMass * this->xCenter + particle->mass * particle->x) / (this->totalMass + particle->mass);
    this->yCenter = (this->totalMass * this->yCenter + particle->mass * particle->y) / (this->totalMass + particle->mass);
    this->zCenter = (this->totalMass * this->zCenter + particle->mass * particle->z) / (this->totalMass + particle->mass);
    this->totalMass = this->totalMass + particle->mass;

    this->particleCount += 1;
}


void Cell::expandChildren()
{
    float xHalf = (this->xMin + this->xMax) / 2;
    float yHalf = (this->yMin + this->yMax) / 2;
    float zHalf = (this->zMin + this->zMax) / 2;

    Cell *c1 = new Cell(this->xMin, xHalf, this->yMin, yHalf, this->zMin, zHalf);
    Cell *c2 = new Cell(xHalf, this->xMax, this->yMin, yHalf, this->zMin, zHalf);
    Cell *c3 = new Cell(this->xMin, xHalf, yHalf, this->yMax, this->zMin, zHalf);
    Cell *c4 = new Cell(xHalf, this->xMax, yHalf, this->yMax, this->zMin, zHalf);
    Cell *c5 = new Cell(this->xMin, xHalf, this->yMin, yHalf, zHalf, this->zMax);
    Cell *c6 = new Cell(xHalf, this->xMax, this->yMin, yHalf, zHalf, this->zMax);
    Cell *c7 = new Cell(this->xMin, xHalf, yHalf, this->yMax, zHalf, this->zMax);
    Cell *c8 = new Cell(xHalf, this->xMax, yHalf, this->yMax, zHalf, this->zMax);

    this->children.push_back(c1);
    this->children.push_back(c2);
    this->children.push_back(c3);
    this->children.push_back(c4);
    this->children.push_back(c5);
    this->children.push_back(c6);
    this->children.push_back(c7);
    this->children.push_back(c8);
}


bool Cell::isFarEnoughFromParticleToUseAsCluster(Particle *particle)
{
    // Check if it's an internal node.
    if(this->children.size() > 0)
    {
        float s = this->xMax - this->xMin;

        float dx = particle->x - this->xCenter;
        float dy = particle->y - this->yCenter;
        float dz = particle->z - this->zCenter;
        float d = (float)pow(dx*dx + dy*dy + dz*dz, 0.5);

        return (s/d) < OMEGA;
    }
        // Otherwise, make sure we're not comparing the particle with itself.
    else
    {
        return particle != this->particle;
    }
}
