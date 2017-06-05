#ifndef NBODY_CELL_H
#define NBODY_CELL_H

#include "Particle.h"
#include <vector>
#include <iostream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class Particle;


class Cell {
public:

    float xMin, xMax, yMin, yMax, zMin, zMax, xCenter, yCenter, zCenter, totalMass;
    std::vector<Cell*> children;
    Particle* particle;
    int particleCount;

    void setCoordinates(float, float, float, float, float, float);
    bool isInsideCell(float, float, float);
    void insertParticle(Particle*);
    void expandChildren();
    void insertChildren(Cell*, int);
    bool isFarEnoughFromParticleToUseAsCluster(Particle*);


    Cell();
    Cell(float, float, float, float, float, float);
    Cell(const Cell &);

    ~Cell(){
        for(int i=0; i<this->children.size(); i++)
        {
            if(this->children[i] != NULL and this->children[i] != nullptr && this->children[i])
                delete this->children[i];
        }
        this->children.clear();
    };
};


#endif
