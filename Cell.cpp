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


void Cell::setCoordinates(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    this->xMin = xMin; this->xMax = xMax;
    this->yMin = yMin; this->yMax = yMax;
    this->zMin = zMin; this->zMax = zMax;
}


bool Cell::isInsideCell(float x, float y, float z)
{
    if
    (
            this->xMin < x && this->xMax >= x
            && this->yMin < y && this->yMax >= y
            && this->zMin < z && this->zMax >= z
    )
    {
        return true;
    }

    return false;
}


void Cell::insertParticle(Particle* particle)
{
    if(!this->isInsideCell(particle->x, particle->y, particle->z))
    {
        return;
    }

    float totalMassBeforeInsertion = this->totalMass;

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

    this->totalMass = this->totalMass + particle->mass;
    this->xCenter = (totalMassBeforeInsertion * this->xCenter + particle->mass * particle->x) / this->totalMass;
    this->yCenter = (totalMassBeforeInsertion * this->yCenter + particle->mass * particle->y) / this->totalMass;
    this->zCenter = (totalMassBeforeInsertion * this->zCenter + particle->mass * particle->z) / this->totalMass;

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


void Cell::insertChildren(Cell *cell, int position)
{
    if(!this->children.size())
    {
        this->expandChildren();
    }

    if(this->children[position] != nullptr && this->children.size() > position)
    {
        this->totalMass = this->totalMass - this->children[position]->totalMass;
        this->particleCount = this->particleCount - this->children[position]->particleCount;
        delete this->children[position];
        this->children[position] = nullptr;
    }

    this->children[position] = cell;
    this->totalMass = this->totalMass + cell->totalMass;

    // Re-calculate center of mass.
    float newXCenter=0, newYCenter=0, newZCenter=0;

    for(int i=0; i<this->children.size(); i++)
    {
        newXCenter = newXCenter + (this->children[i]->totalMass * this->children[i]->xCenter)/this->totalMass;
        newYCenter = newYCenter + (this->children[i]->totalMass * this->children[i]->yCenter)/this->totalMass;
        newZCenter = newZCenter + (this->children[i]->totalMass * this->children[i]->zCenter)/this->totalMass;
    }

    this->xCenter = newXCenter;
    this->yCenter = newYCenter;
    this->zCenter = newZCenter;
}


bool Cell::isFarEnoughForClustering(Particle *particle)
{
    if(this->children.size() > 0)
    {
        float s = this->xMax - this->xMin;
        float dx = particle->x - this->xCenter;
        float dy = particle->y - this->yCenter;
        float dz = particle->z - this->zCenter;
        float d = pow(dx*dx + dy*dy + dz*dz, 0.5);

        return (s/d) < OMEGA;
    }
    else
    {
        return particle != this->particle;
    }
}

