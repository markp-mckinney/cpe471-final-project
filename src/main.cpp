#include <iostream>
#include <math.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "Texture.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

/* to use glee */
#define GLEE_OVERWRITE_GL_FUNCTIONS
#include "glee.hpp"

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;
shared_ptr<Program> grassProg;
shared_ptr<Shape> shape;
shared_ptr<Shape> cube;
shared_ptr<Shape> car;

Texture grassTexture;
GLint h_grassTexture;
GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;

int g_width, g_height;
float phi = 0.0;
float theta = 0.0;
static const float D80 = 1.39626;
int turn = 0;
int accelerate = 0;
float moveSpeed = 0.1;
float turnSpeed = 0.025;
vec3 playerLoc = vec3(0, 0, 0);
vec3 forwardVec = vec3(0, 0, -1);

bool trackForward = true;
double oldX, oldY;
bool hideCursor = true;

bool animateUp = true;
float armRotate = 0.0;

bool initialCam = true;
vec3 flyCam = vec3(0, 100, 250);
float flySpeed = 0.8;
float camDistance = 5.0;

#define NUM_OBJECTS 50
vec3 startingLocs[NUM_OBJECTS];
int materials[NUM_OBJECTS];
float rotations[NUM_OBJECTS];

vector<vec3> buildingLocs[8];
vector<vec3> buildingScales[8];

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// Axis Aligned Bounding Box collision detection
// Only need to check x/z plane since the car height is static
static bool checkCollision(vec3 carPos, vec3 objPos, vec3 objScale) {
    if (fabs(carPos.x - objPos.x) < 1.6 + objScale.x){// / 1.25) {
        if (fabs(carPos.z - objPos.z) < 1.6 + objScale.z){// / 1.25) {
            return true;
        }
    }
    return false;
}

static bool checkCollisions(vec3 carPos) {
    for (int i = 0; i < 8; i++) {
        int j = 0;
        while (j < buildingLocs[i].size()) {
            if (checkCollision(carPos, buildingLocs[i][j], buildingScales[i][j])) {
                return true;
            }
            j++;
        }
    }
    for (int i = 0; i < NUM_OBJECTS; i++) {
        if (i < NUM_OBJECTS / 2) {
            if (checkCollision(carPos, startingLocs[i], vec3(0.6, 0, 0.8))) {
                return true;
            }
        } else {
            if (checkCollision(carPos, startingLocs[i], vec3(0.5, 0, 0.5))) {
                return true;
            }
        }
    }
    return false;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case GLFW_KEY_A:
                turn = -1;
                break;
            case GLFW_KEY_D:
                turn = 1;
                break;
            case GLFW_KEY_W:
                accelerate = 1;
                break;
            case GLFW_KEY_S:
                accelerate = -1;
                break;
            case GLFW_KEY_T:
                trackForward = !trackForward;
                if (!trackForward) {
                    theta = -1.57491;
                    phi = -0.329875;
                }
                break;
            case GLFW_KEY_C:
                hideCursor = !hideCursor;
                if (hideCursor) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                break;
            case GLFW_KEY_LEFT_SHIFT:
                moveSpeed *= 3;
                break;
            case GLFW_KEY_P:
                if (initialCam) {
                    initialCam = false;
                } else {
                    initialCam = true;
                    flyCam = vec3(0, 100, 250);
                }
                break;
            case GLFW_KEY_TAB:
                if (camDistance == 5.0f) {
                    camDistance = 270.0;
                } else {
                    camDistance = 5.0;
                }
                break;
            case GLFW_KEY_E:
                camDistance += 5.0;
                break;
            case GLFW_KEY_Q:
                if (camDistance > 5.0) {
                    camDistance -= 5.0;
                }
                break;
            default:
                break;
        }
	} else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case GLFW_KEY_A:
                if (turn == -1) {
                    turn = 0;
                }
                break;
            case GLFW_KEY_D:
                if (turn == 1) {
                    turn = 0;
                }
                break;
            case GLFW_KEY_W:
                if (accelerate == 1) {
                    accelerate = 0;
                }
                break;
            case GLFW_KEY_S:
                if (accelerate == -1) {
                    accelerate = 0;
                }
                break;
            case GLFW_KEY_LEFT_SHIFT:
                moveSpeed /= 3;
                break;
            default:
                break;
        }
	}
}

static void mouse_callback(GLFWwindow *window, int button, int action, int mods)
{
   double posX, posY;
   if (action == GLFW_PRESS) {
      glfwGetCursorPos(window, &posX, &posY);
      cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
	}
}

static void resize_callback(GLFWwindow *window, int width, int height) {
   g_width = width;
   g_height = height;
   glViewport(0, 0, width, height);
}

static void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float dx = oldX - x;
    float dy = oldY - y;
    oldX = x;
    oldY = y;
    theta += (dx/width) * 3.14 * 3;
    if ((dy < 0 && phi <= 0) || (dy > 0 && phi >= -D80)) {
        phi -= (dy/height) * 3.14 * 3;
    }
    if (phi > -0.02) {
        phi = -0.02;
    }
}

static float randFloat(float max) {
    return (float) (static_cast <float> (rand() / (static_cast <float> (RAND_MAX / max))));
}

static float randFloatSigned(float max) {
    float f = randFloat(max);
    return rand() % 2 ? f : -f;
}

static float randFloatMinMax(float min, float max) {
    return min + randFloat(max - min);
}

static float randFloatMinMaxSigned(float min, float max) {
    float f = randFloatMinMax(min, max);
    return rand() % 2 ? f : -f;
}

static void initGeom() {
    float g_groundSize = 400;
    float g_groundY = -1.0;

    // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
    float GrndPos[] = {
    -g_groundSize, g_groundY, -g_groundSize,
    -g_groundSize, g_groundY,  g_groundSize,
     g_groundSize, g_groundY,  g_groundSize,
     g_groundSize, g_groundY, -g_groundSize
    };

    float GrndNorm[] = {
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0,
     0, 1, 0
    };

    static GLfloat GrndTex[] = {
      0, 0, // back
      0, 100,
      100, 100,
      100, 0 };

    unsigned short idx[] = {0, 1, 2, 0, 2, 3};

    GLuint VertexArrayID;
    //generate the VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glGenBuffers(1, &GrndBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

    glGenBuffers(1, &GrndNorBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);
    
    glGenBuffers(1, &GrndTexBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

    glGenBuffers(1, &GIndxBuffObj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

static void init()
{
	GLSL::checkVersion();

	// Set background color.
	glClearColor(.12f, .34f, .56f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize mesh.
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->resize();
	shape->init();

    cube = make_shared<Shape>();
    cube->loadMesh(RESOURCE_DIR + "cube.obj");
    cube->resize();
    cube->init();

    car = make_shared<Shape>();
    car->loadMesh(RESOURCE_DIR + "car.obj"); // from http://www.turbosquid.com/FullPreview/Index.cfm/ID/1092438
    car->resize();
    car->init();

	// Initialize the GLSL program.
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RESOURCE_DIR + "phongVert.glsl", RESOURCE_DIR + "phongFrag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("M");
    prog->addUniform("V");
    prog->addUniform("MatAmb");
    prog->addUniform("MatDif");
    prog->addUniform("MatSpec");
    prog->addUniform("shine");
    prog->addUniform("lightPos");
	prog->addAttribute("vertPos");
    prog->addAttribute("vertNor");

    grassProg = make_shared<Program>();
	grassProg->setVerbose(true);
	grassProg->setShaderNames(RESOURCE_DIR + "texVert.glsl", RESOURCE_DIR + "texFragGrass.glsl");
	grassProg->init();

    grassTexture.setFilename(RESOURCE_DIR + "grass.bmp");
    grassTexture.setUnit(0);
    grassTexture.setName("grassTexture");
    grassTexture.init(GL_MIRRORED_REPEAT);

    grassProg->addUniform("P");
	grassProg->addUniform("MV");
	grassProg->addUniform("grassTexture");
    grassProg->addUniform("lightPos");
	grassProg->addAttribute("vertPos");
    grassProg->addAttribute("vertNor");
	grassProg->addAttribute("vertTex");
	grassProg->addTexture(&grassTexture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    // robot/rabbit locations
    for (int i = 0; i < NUM_OBJECTS; i++) {
        float x = randFloatSigned(200.0);
        float z;
        if (x < -120 || x > 120) {
            z = randFloatSigned(200.0);
        } else {
            z = randFloatMinMaxSigned(100, 200);
        }
        startingLocs[i] = vec3(x, 0.0, z); 
        materials[i] = rand() % 5;
        rotations[i] = randFloat(7.0);
    }

    // building locations and scales
    int x = -105;
    for (int i = 0; i < 8; i++) {
        //cout << "row " << i << endl;
        float startAt = -70.0;
        float remaining = 80.0;
        while (remaining >= 15) {
            float height = randFloatMinMax(3.0, 70.0);
            float length = randFloatMinMax(5.0, fmin(30, remaining - 5));
            remaining -= (length + 10);
            buildingScales[i].push_back(vec3(8.0, height, length));
            buildingLocs[i].push_back(vec3((float) x, height / 2, startAt + length));
            startAt += length * 2 + 10;
            //cout << "building at " << x << " " << height / 2 << " " << startAt + (length / 2) << endl;
        }
        x += 30;
    }
}

static void setMaterial(int i) {
    switch (i) {
        case 0: // chrome
            glUniform3f(prog->getUniform("MatAmb"), 0.25, 0.25, 0.25); 
            glUniform3f(prog->getUniform("MatDif"), 0.4, 0.4, 0.4);
            glUniform3f(prog->getUniform("MatSpec"), 0.774597, 0.774597, 0.774597);
            glUniform1f(prog->getUniform("shine"), 76.8);
            break;
        case 1: // black rubber
            glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.02, 0.02);
            glUniform3f(prog->getUniform("MatDif"), 0.01, 0.01, 0.01);
            glUniform3f(prog->getUniform("MatSpec"), 0.4, 0.4, 0.4);
            glUniform1f(prog->getUniform("shine"), 10.0);
            break;
        case 2: // gold
            glUniform3f(prog->getUniform("MatAmb"), 0.24725, 0.1995, 0.0745);
            glUniform3f(prog->getUniform("MatDif"), 0.75164, 0.60648, 0.22648);
            glUniform3f(prog->getUniform("MatSpec"), 0.628281, 0.555802, 0.366065);
            glUniform1f(prog->getUniform("shine"), 71.2);
            break;
        case 3: // milk chocolate
            glUniform3f(prog->getUniform("MatAmb"), 0.29, 0.17, 0.0);
            glUniform3f(prog->getUniform("MatDif"), 0.3038, 0.17048, 0.0828);
            glUniform3f(prog->getUniform("MatSpec"), 0.257, 0.1376, 0.08601);
            glUniform1f(prog->getUniform("shine"), 12.8);
            break;
        case 4: // jade
            glUniform3f(prog->getUniform("MatAmb"),0.135, 0.2225, 0.1575);
            glUniform3f(prog->getUniform("MatDif"), 0.54, 0.89, 0.63);
            glUniform3f(prog->getUniform("MatSpec"), 0.316228, 0.316228, 0.316228);
            glUniform1f(prog->getUniform("shine"), 12.8);
            break;
    }
}

static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = width/(float)height;

    auto P = make_shared<MatrixStack>();
    auto M = make_shared<MatrixStack>();
    auto V = make_shared<MatrixStack>();
    vec3 up = vec3(0.0, 1.0, 0.0);

    vec3 rot = vec3(cos(phi) * cos(theta), sin(phi), cos(phi) * cos((3.14/2) - theta));
    if (accelerate != 0) {
        bool collision = false;
        if (accelerate == 1) {
            // check collisions in the front
            collision = checkCollisions(playerLoc + 0.5f * forwardVec);
        } else {
            // check collisions in the back
            collision = checkCollisions(playerLoc - 0.5f * forwardVec);
        }
        if (!collision) {
            vec3 left = cross(up, forwardVec);
            forwardVec -= accelerate * turnSpeed * turn * left;
            forwardVec = normalize(forwardVec);
        } else {
            accelerate = 0;
        }
    }
    playerLoc += (moveSpeed * accelerate) * forwardVec;
    playerLoc.y = 0.0;

    if (initialCam) {
        // fly in the camera from far away
        V->lookAt(flyCam, playerLoc, up);
        vec3 dir = playerLoc - flyCam;
        flyCam += flySpeed * normalize(dir);
        if (length(dir) <= 5.0) {//(flyCam.z <= 5.0) {
            initialCam = false;
        }
    } else {
        if (trackForward) {
            V->lookAt((playerLoc - camDistance * forwardVec) + vec3(0, 2, 0), playerLoc, up);
        } else {
            V->lookAt(playerLoc - camDistance * rot, playerLoc, up);
        }
    }

    // Apply perspective projection.
    P->pushMatrix();
    P->perspective(45.0f, aspect, 0.01f, 400.0f);

    grassProg->bind();
    glUniformMatrix4fv(grassProg->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(grassProg->getUniform("MV"), 1, GL_FALSE, value_ptr(V->topMatrix()));
    glUniform3f(grassProg->getUniform("lightPos"), 6.0, 2.0, 2.0);

	glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	 
	glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

    grassProg->unbind();

    prog->bind();
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
    glUniform3f(prog->getUniform("lightPos"), 6.0, 2.0, 2.0);

    // grass plane
    M->pushMatrix();
        M->loadIdentity();
        M->translate(vec3(0.0, -1.0, 0.0));
        M->scale(vec3(800.0, 0.05, 800.0));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.83, 0.14);
        glUniform3f(prog->getUniform("MatDif"), 0.3, 0.83, 0.4);
        glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.83, 0.4);
        glUniform1f(prog->getUniform("shine"), 4.0);
        //cube->draw(prog);
    M->popMatrix();

    // roads
    glDisable(GL_DEPTH_TEST); // disable to draw roads on top of grass
    for (int i = -90; i <= 90; i += 30) {
        M->pushMatrix();
            M->loadIdentity();
            M->translate(vec3(vec3(i, -0.99, 0.0)));
            M->scale(vec3(5.0, 0.05, 80.0));
            glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
            glUniform3f(prog->getUniform("MatAmb"), 0.03, 0.03, 0.04);
            glUniform3f(prog->getUniform("MatDif"), 0.03, 0.03, 0.04);
            glUniform3f(prog->getUniform("MatSpec"), 0.03, 0.03, 0.04);
            glUniform1f(prog->getUniform("shine"), 4.0);
            cube->draw(prog);
        M->popMatrix();
    }

    // first side road
    M->pushMatrix();
        M->loadIdentity();
        M->translate(vec3(0.0, -0.99, 80.0));
        M->scale(vec3(95.0, 0.05, 5.0));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.03, 0.03, 0.04);
        glUniform3f(prog->getUniform("MatDif"), 0.03, 0.03, 0.04);
        glUniform3f(prog->getUniform("MatSpec"), 0.03, 0.03, 0.04);
        glUniform1f(prog->getUniform("shine"), 4.0);
        cube->draw(prog);
    M->popMatrix();

    // second side road
    M->pushMatrix();
        M->loadIdentity();
        M->translate(vec3(0.0, -0.99, -80.0));
        M->scale(vec3(95.0, 0.05, 5.0));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.03, 0.03, 0.04);
        glUniform3f(prog->getUniform("MatDif"), 0.03, 0.03, 0.04);
        glUniform3f(prog->getUniform("MatSpec"), 0.03, 0.03, 0.04);
        glUniform1f(prog->getUniform("shine"), 4.0);
	  	cube->draw(prog);
    M->popMatrix();
    glEnable(GL_DEPTH_TEST);

    // car
    M->pushMatrix();
        M->loadIdentity();
        M->translate(playerLoc);
        M->translate(vec3(0, -0.32, 0));
        float angle = atan2(forwardVec.x * -1, dot(forwardVec, vec3(0, 0, -1)));
        M->rotate(angle, vec3(0, 1, 0)); // rotate to actual position from forward
        M->rotate(1.57, vec3(0, 1, 0)); // rotate by default to face forward
        M->scale(vec3(2, 2, 2));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.93, 0.83, 0.14);
        glUniform3f(prog->getUniform("MatDif"), 0.9, 0.83, 0.4);
        glUniform3f(prog->getUniform("MatSpec"), 0.9, 0.83, 0.4);
        glUniform1f(prog->getUniform("shine"), 24.0);

        glUniform3f(prog->getUniform("MatAmb"), 0.63, 0.13, 0.14);
        glUniform3f(prog->getUniform("MatDif"), 0.3, 0.1, 0.1);
        glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.1, 0.1);
        glUniform1f(prog->getUniform("shine"), 14.0);
        car->draw(prog);
    M->popMatrix();

    // buildings
    for (int i = 0; i < 8; i++) {
        int j = 0;
        while (j < buildingLocs[i].size()) {
            M->pushMatrix();
                M->loadIdentity();
                M->translate(buildingLocs[i][j]);
                M->scale(buildingScales[i][j]);
                glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                setMaterial(4);
                cube->draw(prog);
            M->popMatrix();
            j++;
        }
    }

    // draw objects
    for (int i = 0; i < NUM_OBJECTS / 2; i++) {
        M->pushMatrix();
            M->loadIdentity();
    		M->translate(startingLocs[i]);
            M->rotate(rotations[i], vec3(0.0, 1.0, 0.0));
    	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
            setMaterial(materials[i]);
    	  	shape->draw(prog);
        M->popMatrix();
    }

    for (int i = NUM_OBJECTS / 2; i < NUM_OBJECTS; i++) {
        setMaterial(materials[i]);
		M->pushMatrix();
            M->loadIdentity();
    		M->translate(startingLocs[i]);
            M->translate(vec3(0.0, 1.0, 0.0));
            M->rotate(rotations[i], vec3(0.0, 1.0, 0.0));
    	  	M->scale(vec3(0.75, 0.75, 0.75));
    	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    	  	cube->draw(prog);

            // left upper arm
            M->pushMatrix();
                M->translate(vec3(1, 1, 0));
                // rotate from -1.2 to 0.8
                if (armRotate <= 2.0) {
                    M->rotate(armRotate - 1.2, vec3(0, 0, 1));
                } else {
                    M->rotate(0.8, vec3(0, 0, 1));
                }
                M->translate(vec3(0.75, 0, 0));

                // left forearm
                M->pushMatrix();
                    M->translate(vec3(0.75, 0, 0));
                    // rotate from 0.5 to 1.2 after armRotate is at 2.0
                    if (armRotate <= 1.8) {
                        M->rotate(0.5, vec3(0, 0, 1));
                    } else {
                        M->rotate(armRotate - 1.3, vec3(0, 0, 1));
                    }
                    if (animateUp) {
                        if (armRotate < 3.0) {
                            armRotate += 0.002;
                        } else {
                            animateUp = false;
                        }
                    } else {
                        if (armRotate > 0) {
                            armRotate -= 0.002;
                        } else {
                            animateUp = true;
                        }
                    }
                    M->translate(vec3(0.75, 0, 0));

                    // left hand
                    M->pushMatrix();
                        M->translate(vec3(0.8, 0, 0));
                        M->scale(vec3(0.3, 0.3, 0.3));
                        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                        cube->draw(prog);
                    M->popMatrix();
    
                    M->scale(vec3(0.75, 0.25, 0.25));
                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                    cube->draw(prog);
                M->popMatrix();
                
                M->scale(vec3(0.75, 0.25, 0.25));
                glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                cube->draw(prog);
            M->popMatrix();

            // right upper arm
            M->pushMatrix();
                M->translate(vec3(-1, 1, 0));
                M->rotate(1.2, vec3(0, 0, 1));
                M->translate(vec3(-0.75, 0, 0));

                // right forearm
                M->pushMatrix();
                    M->translate(vec3(-0.75, 0, 0));
                    M->rotate(-0.5, vec3(0, 0, 1));
                    M->translate(vec3(-0.75, 0, 0));

                    // left hand
                    M->pushMatrix();
                        M->translate(vec3(-0.8, 0, 0));
                        M->scale(vec3(0.3, 0.3, 0.3));
                        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                        cube->draw(prog);
                    M->popMatrix();
    
                    M->scale(vec3(0.75, 0.25, 0.25));
                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                    cube->draw(prog);
                M->popMatrix();
                
                M->scale(vec3(0.75, 0.25, 0.25));
                glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
                cube->draw(prog);
            M->popMatrix();

            // head
            M->pushMatrix();
                M->translate(vec3(0, 1.5, 0));
    	        M->rotate(3.14/4, vec3(0, 1, 0));
                M->scale(vec3(.7, .7, .7));
    	  	    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    	        cube->draw(prog);
            M->popMatrix();

            // left leg
            M->pushMatrix();
                M->translate(vec3(-0.5, -1.0, 0.0));
                M->scale(vec3(0.3, 1.75, 0.3));
    	  	    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    	        cube->draw(prog);
            M->popMatrix();
            
            // right leg
            M->pushMatrix();
                M->translate(vec3(0.5, -1.0, 0.0));
                M->scale(vec3(0.3, 1.75, 0.3));
    	  	    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    	        cube->draw(prog);
            M->popMatrix();
        M->popMatrix();
    }

	prog->unbind();

    // Pop matrix stacks.
    P->popMatrix();
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
    unsigned srandTime = static_cast <unsigned> (time(0));
    if (argc == 3) {
        srandTime = (unsigned) atoll(argv[2]);
    }
    cout << srandTime << endl;
    srand(srandTime);

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
   //request the highest possible version of OGL - important for mac
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Mark McKinney", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	//weird bootstrap of glGetError
   glGetError();
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
   cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
   //set the mouse call back
   glfwSetMouseButtonCallback(window, mouse_callback);
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   glfwSetCursorPosCallback(window, cursor_pos_callback);
   glfwGetCursorPos(window, &oldX, &oldY);
   //set the window resize call back
   glfwSetFramebufferSizeCallback(window, resize_callback);

	// Initialize scene. Note geometry initialized in init now
	init();
    initGeom();

	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
