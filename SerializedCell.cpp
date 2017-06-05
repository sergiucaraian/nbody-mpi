#include "SerializedCell.h"
#include <stdio.h>
#include <vector>
#include <iostream>
#include <map>
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


void SerializedCell::sdrTraversal(std::vector<Cell *>& cells, Cell *cell)
{
    for(int i=0; i < cell->children.size(); i++)
    {
        this->sdrTraversal(cells, cell->children[i]);
    }

    cells.push_back(cell);
}


void SerializedCell::serializeTree(Cell* cell)
{
    if(this->serializedCellMatrixFloats != nullptr)
    {
        delete[] this->serializedCellMatrixFloats;
    }
    if(this->serializedCellMatrixInts != nullptr)
    {
        delete[] this->serializedCellMatrixInts;
    }

    std::vector<Cell*> cells;
    sdrTraversal(cells, cell);

    std::map<Cell*, int> cellAddressToSerializedIndex;

    for(int i=0; i<cells.size(); i++)
    {
        cellAddressToSerializedIndex[cells[i]] = i;
    }

    this->cellCount = cells.size();
    this->serializedCellMatrixFloats = new float[this->cellCount * 10];
    this->serializedCellMatrixInts = new int[this->cellCount * 10];

    for(int i=0; i<cells.size(); i++)
    {
        this->serializedCellMatrixFloats[i * 10 + 0] = cells[i]->xMin;
        this->serializedCellMatrixFloats[i * 10 + 1] = cells[i]->xMax;
        this->serializedCellMatrixFloats[i * 10 + 2] = cells[i]->yMin;
        this->serializedCellMatrixFloats[i * 10 + 3] = cells[i]->yMax;
        this->serializedCellMatrixFloats[i * 10 + 4] = cells[i]->zMin;
        this->serializedCellMatrixFloats[i * 10 + 5] = cells[i]->zMax;
        this->serializedCellMatrixFloats[i * 10 + 6] = cells[i]->xCenter;
        this->serializedCellMatrixFloats[i * 10 + 7] = cells[i]->yCenter;
        this->serializedCellMatrixFloats[i * 10 + 8] = cells[i]->zCenter;
        this->serializedCellMatrixFloats[i * 10 + 9] = cells[i]->totalMass;
    }

    for(int i=0; i<cells.size(); i++)
    {
        this->serializedCellMatrixInts[i*10 + 0] = cells[i]->particleCount;

        if(cells[i]->particle)
        {
            for(int j=0; j<this->particleVector->size(); j++)
            {
                if(&(*this->particleVector)[j] == cells[i]->particle)
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
            if(j < cells[i]->children.size())
            {
                this->serializedCellMatrixInts[i*10 + 2 + j] = cellAddressToSerializedIndex[cells[i]->children[j]];
            }
            else
            {
                this->serializedCellMatrixInts[i*10 + 2 + j] = -1;
            }
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

        if(serializedCellMatrixInts[i*10 + 1] == -1)
        {
            newCell->particle = nullptr;
        }
        else
        {
            newCell->particle = &(*this->particleVector)[serializedCellMatrixInts[i*10 + 1]];
        }
    }

    for(int i=0; i<deserializedCells.size(); i++)
    {
        for(int j=0; j<8; j++)
        {
            // Assumption: the cell either has the maximum children count or none.
            if(serializedCellMatrixInts[i*10 + 2 + j] != -1)
            {
                deserializedCells[i]->children.push_back(deserializedCells[serializedCellMatrixInts[i*10 + 2 + j]]);
            }
        }
    }

    return deserializedCells[deserializedCells.size()-1];
}
