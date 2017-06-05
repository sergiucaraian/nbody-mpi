#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <boost/mpi.hpp>
#include <cstdlib>
#include <queue>

#include "common.h"
#include "common/shader.hpp"
#include "Particle.h"
#include "Cell.h"
#include "SerializedCell.h"

namespace mpi = boost::mpi;
using namespace std;


// Graphics
GLFWwindow* window;
GLuint programID;
GLuint VertexArrayID;
GLuint vertexBuffer;
GLuint colorBuffer;
GLuint MatrixID;
glm::mat4 MVP;

// Physics
vector<Particle> particles;
vector<Cell> cells;


// Return a VertexBuffer for particle positions.
GLfloat* getVertexBufferData()
{
    GLfloat *vertexBuffer = new GLfloat[particles.size() * 3];

    for(int i=0, j=0; i<particles.size(); i++, j+=3)
    {
        vertexBuffer[j] = (GLfloat)particles[i].x;
        vertexBuffer[j+1] = (GLfloat)particles[i].y;
        vertexBuffer[j+2] = (GLfloat)particles[i].z;
    }

    return vertexBuffer;
}


// Initialize graphics.
void initGraphics()
{
    if(!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "N-Body Simulation", NULL, NULL);

    if(window == NULL)
    {
        fprintf( stderr, "Failed to Open GLFW window.\n" );
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental=true;
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return;
    }

    // Ensure we can capture the escape key.
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Generate and bind a VertexArray
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Compile shaders
    programID = LoadShaders("../shaders/VertexShader.vs.glsl", "../shaders/FragmentShader.fs.glsl");

    // Get a handle for the ModelViewProjection matrix.
    MatrixID = glGetUniformLocation(programID, "MVP");

    // Projection matrix : 45 degree Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);

    // Camera matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(2, 2,-2), // camera position
            glm::vec3(0,0,0), // looks at origin
            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);

    // ModelViewProjection matrix
    MVP = Projection * View * Model;

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, GL_FLOAT * 3 * PARTICLE_COUNT, NULL, GL_DYNAMIC_DRAW);
}


void initPhysics()
{
    // Initialize the particle vector with a plummer sphere density.
    Particle::plummerSphereDensity(particles, PARTICLE_COUNT, SOFTENING_LENGTH, G);
}


void init()
{
    initGraphics();
    initPhysics();
}

// Render the curent step.
void render()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader
    glUseProgram(programID);

    // Send MVP matrix
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);


    // Prepare the vertex buffer.
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
    );

    GLfloat* g_vertex_buffer_data = getVertexBufferData();

    // Update vertex data
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * PARTICLE_COUNT, g_vertex_buffer_data);

    // Draw
    glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);

    glEnableVertexAttribArray(0);


    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

    delete g_vertex_buffer_data;
}


int getNumberOfCellsInTree(Cell* cell)
{
    if(cell->children.size() == 0)
    {
        return 1;
    }
    else
    {
        int nrOfCells = 1;

        for(int i=0; i<cell->children.size(); i++)
        {
            nrOfCells += getNumberOfCellsInTree(cell->children[i]);
        }

        return nrOfCells;
    }
}

// Run a simulation step.
void simulate()
{
    mpi::communicator world;

    // First create the empty tree up to the second level (so that we have better potential for parallelism).
    // This way we can scale up to 64 cores.
    Cell *root = new Cell(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE);
    root->expandChildren();

    std::vector<Cell*> secondLevelCells;
    for(int i=0; i<root->children.size(); i++)
    {
        root->children[i]->expandChildren();

        for(int j=0; j<root->children[i]->children.size(); j++)
        {
            secondLevelCells.push_back(root->children[i]->children[j]);
        }
    }

    // Split the second level cells among processes.
    // We split in a round robin style because the particles tend to be grouped in a couple of regions.
    // Thus, if adjacent regions are processed by different processes we'll have better parallelism.
    std::vector<Cell*> cellsOfThisProcess;

    for(int i = world.rank(); i < secondLevelCells.size(); i+=world.size())
    {
        cellsOfThisProcess.push_back(secondLevelCells[i]);
    }

    // Add all the particles.
    // Trying to add a particle to the wrong cell of the tree is ignored, so we try to add all particles to all cells.
    for(int i=0; i<particles.size(); i++)
    {
        for(int j=0; j<cellsOfThisProcess.size(); j++)
        {
            cellsOfThisProcess[j]->insertParticle(&particles[i]);
        }
    }

    // Now it's time to assemble the partially constructed tress on the main process.
    // Create a serialized structure to hold cellsOfThisProcess information.
    vector<SerializedCell> serializedCellsOfThisProcess;

    for(int i=0; i<cellsOfThisProcess.size(); i++)
    {
        SerializedCell serializedCell;
        serializedCell.particleVector = &particles;
        serializedCell.serializeTree(cellsOfThisProcess[i]);

        serializedCellsOfThisProcess.push_back(serializedCell);
    }

    // Gather the tree branches on the main process.
    vector<vector<SerializedCell>> gatheredSecondLevelBranches;
    mpi::gather(world, serializedCellsOfThisProcess, gatheredSecondLevelBranches, 0);

    // Clear the old Cell data to clear up space for the new.
    delete root;
    secondLevelCells.clear();
    cellsOfThisProcess.clear();

    // Rebuild the tree from branches on the main process
    if(world.rank() == 0)
    {
        vector<Cell*> secondLevelBranches(64);
        for(int i=0; i<gatheredSecondLevelBranches.size(); i++)
        {
            for(int j=0; j<gatheredSecondLevelBranches[i].size(); j++)
            {
                // Set the particle vector pointer which was lost during serialization / deserialization.
                gatheredSecondLevelBranches[i][j].particleVector = &particles;

                secondLevelBranches[j * gatheredSecondLevelBranches.size() + i] = gatheredSecondLevelBranches[i][j].deserializeTree();
            }
        }

        root = new Cell(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE);
        root->expandChildren();

        for(int i=0, k=0; i<root->children.size(); i++)
        {
            for(int j=0; j<root->children.size(); j++, k++)
            {
                root->children[i]->children.push_back(secondLevelBranches[k]);
            }
        }

        // Calculate center of gravity and total mass for the root and first level nodes.
        for(int i=0; i<root->children.size(); i++)
        {
            for(int j=0; j<root->children[i]->children.size(); j++)
            {
                root->children[i]->xCenter = (root->children[i]->totalMass * root->children[i]->xCenter + root->children[i]->children[j]->totalMass * root->children[i]->children[j]->xCenter) / (root->children[i]->totalMass + root->children[i]->children[j]->totalMass);
                root->children[i]->yCenter = (root->children[i]->totalMass * root->children[i]->yCenter + root->children[i]->children[j]->totalMass * root->children[i]->children[j]->yCenter) / (root->children[i]->totalMass + root->children[i]->children[j]->totalMass);
                root->children[i]->zCenter = (root->children[i]->totalMass * root->children[i]->zCenter + root->children[i]->children[j]->totalMass * root->children[i]->children[j]->zCenter) / (root->children[i]->totalMass + root->children[i]->children[j]->totalMass);

                root->children[i]->totalMass += root->children[i]->children[j]->totalMass;
            }

            root->xCenter = (root->totalMass * root->xCenter + root->children[i]->totalMass * root->children[i]->xCenter) / (root->totalMass + root->children[i]->totalMass);
            root->yCenter = (root->totalMass * root->yCenter + root->children[i]->totalMass * root->children[i]->yCenter) / (root->totalMass + root->children[i]->totalMass);
            root->zCenter = (root->totalMass * root->zCenter + root->children[i]->totalMass * root->children[i]->zCenter) / (root->totalMass + root->children[i]->totalMass);

            root->totalMass += root->children[i]->totalMass;
        }
    }

    // Broadcast the newly built tree to all other processes.
    SerializedCell serializedRoot;

    if(world.rank() == 0)
    {
        serializedRoot.particleVector = &particles;
        serializedRoot.serializeTree(root);
    }

    mpi::broadcast(world, serializedRoot, 0);

    if(world.rank() != 0)
    {
        root = serializedRoot.deserializeTree();
    }

    vector<Cell*> cells;
    serializedRoot.sdrTraversal(cells, root);

    // Update the velocity of the particles after interacting with other particles or clusters of particles.
    for(int i = world.rank(); i < particles.size(); i+=world.size())
    {
        std::list<Cell*> cellQueue;
        cellQueue.push_front(root);

        while(!cellQueue.empty())
        {
            Cell *crtCell = cellQueue.front();
            cellQueue.pop_front();

            if(crtCell->isFarEnoughFromParticleToUseAsCluster(&particles[i]))
            {
                particles[i].forcePush(crtCell, TIMESTEP);
            }
            else
            {
                for(int j=0; j<crtCell->children.size(); j++)
                {
                    cellQueue.push_back(crtCell->children[j]);
                }
            }
        }

        // Update the particles position after it's velocity has been updated.
        particles[i].updatePosition(TIMESTEP);
    }

    // Gather the partially calculated particle vectors on all processes and assemble the final particle vector
    vector<vector<Particle>> gatheredParticleVectors;
    mpi::all_gather(world, particles, gatheredParticleVectors);

    for(int i=0; i<gatheredParticleVectors.size(); i++)
    {
        for(int j=i; j<gatheredParticleVectors[i].size(); j+=gatheredParticleVectors.size())
        {
            particles[j] = gatheredParticleVectors[i][j];
        }
    }

    // Cleanup
    delete root;
}


int main(int argc, char** argv)
{
    mpi::environment env;
    mpi::communicator world;

//    srand(123);
    srand(time(NULL));

    if(world.rank() == 0)
    {
        init();
    }

    mpi::broadcast(world, particles, 0);

    while(true)
    {
        simulate();

        // Wait for simulation to end on all instances.
        world.barrier();

        // Render on the main instance.
        if(world.rank() == 0)
        {
            render();
        }

        // Render at about 60FPS.
        nanosleep((const struct timespec[]){{0, 16666667}}, NULL);

        world.barrier();
    }

    return 0;
}
