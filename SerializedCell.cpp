#include "SerializedCell.h"
#include <stdio.h>
#include <vector>
#include <iostream>
#include "Cell.h"


SerializedCell::SerializedCell(const SerializedCell &obj)
{
    this->particleVector = obj.particleVector;
    this->cellCount = obj.cellCount;

    this->serializedCellMatrixFloats = new float[this->cellCount*10];
    for(int i=0; i<this->cellCount*10; i++)
    {
        this->serializedCellMatrixFloats[i] = obj.serializedCellMatrixFloats[i];
    }

    this->serializedCellMatrixInts = new int[this->cellCount*10];
    for(int i=0; i<this->cellCount*10; i++)
    {
        this->serializedCellMatrixInts[i] = obj.serializedCellMatrixInts[i];
    }
}


void SerializedCell::sdrTraversal(std::vector<Cell *>& supportVector, Cell *cell)
{
    if(!cell->children.size())
    {
        supportVector.push_back(cell);
    }
    else
    {
        for(int i=0; i<cell->children.size(); i++)
        {
            sdrTraversal(supportVector, cell->children[i]);
        }

        supportVector.push_back(cell);
    }
}


void SerializedCell::serializeTree(Cell* cell)
{
    std::cout<<"SERTREEE";
    if(this->serializedCellMatrixFloats != nullptr)
    {
        delete[] this->serializedCellMatrixFloats;
    }
    if(this->serializedCellMatrixInts != nullptr)
    {
        delete[] this->serializedCellMatrixInts;
    }

    std::vector<Cell*> supportVector;
    sdrTraversal(supportVector, cell);

    for(int i=0; i<supportVector.size(); i++)
    {
        supportVector[i]->serialID = i;
    }

    this->cellCount = supportVector.size();
    this->serializedCellMatrixFloats = new float[this->cellCount * 10];
    this->serializedCellMatrixInts = new int[this->cellCount * 10];


    for(int i=0; i<supportVector.size(); i++)
    {
        this->serializedCellMatrixFloats[i * 10 + 0] = supportVector[i]->xMin;
        this->serializedCellMatrixFloats[i * 10 + 1] = supportVector[i]->xMax;
        this->serializedCellMatrixFloats[i * 10 + 2] = supportVector[i]->yMin;
        this->serializedCellMatrixFloats[i * 10 + 3] = supportVector[i]->yMax;
        this->serializedCellMatrixFloats[i * 10 + 4] = supportVector[i]->zMin;
        this->serializedCellMatrixFloats[i * 10 + 5] = supportVector[i]->zMax;
        this->serializedCellMatrixFloats[i * 10 + 6] = supportVector[i]->xCenter;
        this->serializedCellMatrixFloats[i * 10 + 7] = supportVector[i]->yCenter;
        this->serializedCellMatrixFloats[i * 10 + 8] = supportVector[i]->zCenter;
        this->serializedCellMatrixFloats[i * 10 + 9] = supportVector[i]->totalMass;
    }


    for(int i=0; i<supportVector.size(); i++)
    {
        this->serializedCellMatrixInts[i*10 + 0] = supportVector[i]->particleCount;

        if(cell->particle)
        {
            for(int j=0; j<this->particleVector->size(); j++)
            {
                if(&(*this->particleVector)[j] == cell->particle)
                {
                    // Index of the particle in the particle vector.
                    this->serializedCellMatrixInts[i*10 + 1] = j;
                }
            }
        }
        else
        {
            this->serializedCellMatrixInts[i*10 + 1] = -1;
        }

        for(int j=0; j<8; j++)
        {
            if(j < cell->children.size())
            {
                this->serializedCellMatrixInts[i*10 + 2 + j] = cell->children[j]->serialID;
            }
            else
            {
                this->serializedCellMatrixInts[i*10 + 2 + j] = -1;
            }
            //std::cout<<this->serializedCellMatrixInts[i*10+2+j]<<'\n';
        }
    }
}


Cell* SerializedCell::deserializeTree()
{
    std::vector<Cell*> deserializedCells;


    for(int i=0; i<this->cellCount; i++)
    {
        Cell* newCell = new Cell();
        deserializedCells.push_back(newCell);

        newCell->xMin = serializedCellMatrixFloats[i*10 + 0];
        newCell->xMax = serializedCellMatrixFloats[i*10 + 1];
        newCell->yMin = serializedCellMatrixFloats[i*10 + 2];
        newCell->yMax = serializedCellMatrixFloats[i*10 + 3];
        newCell->zMin = serializedCellMatrixFloats[i*10 + 4];
        newCell->zMax = serializedCellMatrixFloats[i*10 + 5];
        newCell->xCenter = serializedCellMatrixFloats[i*10 + 6];
        newCell->yCenter = serializedCellMatrixFloats[i*10 + 7];
        newCell->zCenter = serializedCellMatrixFloats[i*10 + 8];
        newCell->totalMass = serializedCellMatrixFloats[i*10 + 9];

        newCell->particleCount = serializedCellMatrixInts[i*10 + 0];
        newCell->particle = &(*this->particleVector)[serializedCellMatrixInts[i*10 + 1]];
    }


    for(int i=0; i<deserializedCells.size(); i++)
    {
        for(int j=0; j<8; j++)
        {
            if(serializedCellMatrixInts[i*10 + 2 +j] != -1)
            {
                deserializedCells[i]->children.push_back(deserializedCells[serializedCellMatrixInts[i*10 + 2 +j]]);
            }
        }
    }

    return deserializedCells.back();
}