#ifndef NBODY_PARTICLE_H
#define NBODY_PARTICLE_H

#include "common.h"
#include <vector>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class Cell;


class Particle {
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & x; ar & y; ar & z;
        ar & vX; ar & vY; ar & vZ;
        ar & mass;
    }
public:
    float x, y, z;
    float vX, vY, vZ;
    float mass;

    void setCoordinates(float, float, float);
    void setMass(float);
    void setSpeed(float, float, float);
    void scale(float, float, float, float, float, float, float, float);
    void forcePush(Cell*, float);
    void updatePosition(float);

    Particle();
    Particle(float, float, float, float, float, float, float);

    static void plummerSphereDensity(std::vector<Particle>&, int, int, float);
};


#endif
