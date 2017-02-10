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


GLfloat* getVertexBufferData()
{
    GLfloat *vertexBuffer = new GLfloat[particles.size() * 3];
    int crt = 0;

    for(int i=0; i<particles.size(); i++)
    {
        vertexBuffer[crt] = (GLfloat)particles[i].x;
        vertexBuffer[crt+1] = (GLfloat)particles[i].y;
        vertexBuffer[crt+2] = (GLfloat)particles[i].z;
        crt += 3;
    }

    return vertexBuffer;
}


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

    // Get a handle for the "MVP" uniform
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
    // Initialize the particle vector.
    Particle::plummerSphereDensity(particles, PARTICLE_COUNT, SOFTENING_LENGTH, G);
}


void init()
{
    initGraphics();
    initPhysics();
}


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


void simulate()
{
    mpi::communicator world;

    // First create the empty tree up to the second level (so that we have better potential for parallelism).
    Cell *root = new Cell(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE);
    root->expandChildren();
    std::vector<Cell*> secondLevelCells;


    for(int i=0; i<root->children.size(); i++)
    {
        // Second level expansion for better parallelism.
        root->children[i]->expandChildren();

        for(int j=0; j<root->children[i]->children.size(); j++)
        {
            secondLevelCells.push_back(root->children[i]->children[j]);
        }
    }

    std::vector<Cell*> cellsOfThisProcess;

    // Split the second level cells among processes.
    // We split in a round robin style because the particles tend to be grouped in a couple of regions.
    // If adjacent regions are processed by different processes we'll have better parallelism.
    for(int i=world.rank(); i < secondLevelCells.size(); i+=world.size())
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

    // Create a serialized structure to hold cellsOfThisProcess information.
    vector<SerializedCell> serializedCellsOfThisProcess;

    for(int i=0; i<cellsOfThisProcess.size(); i++)
    {
        SerializedCell serializedCell;
        serializedCellsOfThisProcess.push_back(serializedCell);
        serializedCellsOfThisProcess[i].particleVector = &particles;
        serializedCellsOfThisProcess[i].serializeTree(cellsOfThisProcess[i]);
    }


    // We no longer need the un-serialized cells.
    // Deleting root will also delete all the cells bounded to it recursively.
    delete root;
    secondLevelCells.clear();
    cellsOfThisProcess.clear();


    // Gather the tree branches on the root process.
    vector<vector<SerializedCell>> gatheredSecondLevelBranches;
    mpi::gather(world, serializedCellsOfThisProcess, gatheredSecondLevelBranches, 0);

    Cell *newRoot = new Cell(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE);

    // Rebuild the tree from branches
    if(world.rank() == 0)
    {
        // Deserialize
        vector<vector<Cell*>> secondLevelBranches(gatheredSecondLevelBranches.size(), vector<Cell*>(0));

        for(int i=0; i<gatheredSecondLevelBranches.size(); i++)
        {
            for(int j=0; j<gatheredSecondLevelBranches[i].size(); j++)
            {
                // Set the particle vector pointer which was lost during serialization / deserialization.
                gatheredSecondLevelBranches[i][j].particleVector = &particles;

                //gatheredSecondLevelBranches[i][j].deserializeTree();
                secondLevelBranches[i].push_back(gatheredSecondLevelBranches[i][j].deserializeTree());
            }
        }

        //Cell *newRoot = new Cell(COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE, COORDINATE_MIN_VALUE, COORDINATE_MAX_VALUE);
        newRoot->expandChildren();

        for(int i=0; i<newRoot->children.size(); i++)
        {
            newRoot->children[i]->expandChildren();
        }

        int crtChild=0, crtGrandchild=0;
        for(int i=0; i<secondLevelBranches.size(); i++)
        {
            for(int j=0; j<secondLevelBranches[i].size(); j++)
            {
                newRoot->children[crtChild]->insertChildren(secondLevelBranches[i][j], crtGrandchild);
                crtGrandchild++;

                if(crtGrandchild > newRoot->children[crtChild]->children.size())
                {
                    crtGrandchild = 0;
                    crtChild++;
                }
            }
        }

        // Calculate center of gravity and total mass for the root node.
        for(int i=0; i<newRoot->children.size(); i++)
        {
            newRoot->totalMass = newRoot->totalMass + newRoot->children[i]->totalMass;
        }

        for(int i=0; i<newRoot->children.size(); i++)
        {
            newRoot->xCenter = newRoot->xCenter + (newRoot->children[i]->totalMass * newRoot->children[i]->xCenter)/newRoot->totalMass;
            newRoot->yCenter = newRoot->yCenter + (newRoot->children[i]->totalMass * newRoot->children[i]->yCenter)/newRoot->totalMass;
            newRoot->zCenter = newRoot->zCenter + (newRoot->children[i]->totalMass * newRoot->children[i]->zCenter)/newRoot->totalMass;
        }
    }

    world.barrier();

    cout<<world.rank()<<"\n";

    world.barrier();

    // Broadcast the newly built tree to all the processes.
    SerializedCell serializedTree;
    serializedTree.particleVector = &particles;

    if(world.rank() == 0)
    {
        //serializedTree.serializeTree(newRoot);
    }


//    boost::mpi::broadcast(world, serializedTree, 0);
//////
//////
//////    mpi::broadcast(world, particles, 0);
//
//
//    std::cout<<"End of simulation";
}


int main(int argc, char** argv)
{
    mpi::environment env;
    mpi::communicator world;

//    srand(time(NULL));

    if(world.rank() == 0)
    {
        init();
    }

    mpi::broadcast(world, particles, 0);


    simulate();

    // Wait for the simulation to end on all instances.
//    world.barrier();
//
//    while(true)
//    {
//        // Simulate with all
//
//
//        // Render
//        if(world.rank() == 0)
//        {
//            render();
//        }
//    }
}
