#ifndef NBODY_SERIALIZEDCELL_H
#define NBODY_SERIALIZEDCELL_H
#include "common.h"
#include "Cell.h"
#include "Particle.h"
#include <vector>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>

class Particle;
class Cell;

class SerializedCell {
public:
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->cellCount;
        for(int i=0; i<this->cellCount*10; i++)
        {
            ar << this->serializedCellMatrixFloats[i];
        }
        for(int i=0; i<this->cellCount*10; i++)
        {
            ar << this->serializedCellMatrixInts[i];
        }

        delete[] this->serializedCellMatrixFloats;
        delete[] this->serializedCellMatrixInts;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar >> this->cellCount;
        this->serializedCellMatrixFloats = new float[this->cellCount*10];
        this->serializedCellMatrixInts = new int[this->cellCount*10];

        for(int i=0; i<this->cellCount*10; i++)
        {
            float aux;
            ar>>aux;
            this->serializedCellMatrixFloats[i] = aux;
        }
        for(int i=0; i<this->cellCount*10; i++)
        {
            int aux;
            ar>>aux;
            this->serializedCellMatrixInts[i] = aux;
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::split_member(ar, *this, version);
    }

    std::vector<Particle> *particleVector;
    float* serializedCellMatrixFloats=nullptr;
    int* serializedCellMatrixInts = nullptr;
    long cellCount=0;

    void sdrTraversal(std::vector<Cell*>&, Cell*);
    void serializeTree(Cell*);
    Cell* deserializeTree();

    SerializedCell(){};
    SerializedCell(const SerializedCell &);
};


#endif