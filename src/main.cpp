#include <iostream>
#include <math.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

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
shared_ptr<Shape> shape;
shared_ptr<Shape> cube;
shared_ptr<Shape> car;

int g_width, g_height;
float phi = 0.0;
float theta = 0.0;
static const float D80 = 1.39626;
int turn = 0;
int accelerate = 0;
float moveSpeed = 0.08;
float turnSpeed = 0.025;
vec3 playerLoc = vec3(0, 0, 0);
vec3 forwardVec = vec3(0, 0, -1);

bool trackForward = true;
double oldX, oldY;
bool hideCursor = true;

bool animateUp = true;
float armRotate = 0.0;

#define NUM_OBJECTS 50
vec3 startingLocs[NUM_OBJECTS];
int materials[NUM_OBJECTS];
float rotations[NUM_OBJECTS];

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
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

static float getRandFloat(float max) {
    return (float) ((-max) + static_cast <float> (rand() / (static_cast <float> (RAND_MAX / (max * 2)))));
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

    for (int i = 0; i < NUM_OBJECTS; i++) {
        startingLocs[i] = vec3(getRandFloat(40.0), 0.0, getRandFloat(40.0));
        materials[i] = rand() % 5;
        rotations[i] = getRandFloat(7.0);
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

	//Use the matrix stack for Lab 6
    float aspect = width/(float)height;

    // Create the matrix stacks - please leave these alone for now
    auto P = make_shared<MatrixStack>();
    auto M = make_shared<MatrixStack>();
    auto V = make_shared<MatrixStack>();
    vec3 up = vec3(0.0, 1.0, 0.0);

    vec3 rot = vec3(cos(phi) * cos(theta), sin(phi), cos(phi) * cos((3.14/2) - theta));
    if (accelerate != 0) {
        vec3 left = cross(up, forwardVec);
        forwardVec -= accelerate * turnSpeed * turn * left;
        forwardVec = normalize(forwardVec);
    }
    playerLoc += (moveSpeed * accelerate) * forwardVec;
    playerLoc.y = 0.0;

    if (trackForward) {
        V->lookAt((playerLoc - 5.0f * forwardVec) + vec3(0, 2, 0), playerLoc, up);
    } else {
        V->lookAt(playerLoc - 5.0f * rot, playerLoc, up);
    }

    // Apply perspective projection.
    P->pushMatrix();
    P->perspective(45.0f, aspect, 0.01f, 100.0f);

    prog->bind();
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
    glUniform3f(prog->getUniform("lightPos"), 6.0, 2.0, 2.0);

    // car
    M->pushMatrix();
        M->loadIdentity();
        M->translate(playerLoc);
        M->translate(vec3(0, -0.34, 0));
        float angle = atan2(forwardVec.x * -1, dot(forwardVec, vec3(0, 0, -1)));
        M->rotate(angle, vec3(0, 1, 0)); // rotate to actual position from forward
        M->rotate(1.57, vec3(0, 1, 0)); // rotate by default to face forward
        M->scale(vec3(2, 2, 2));
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.93, 0.83, 0.14);
        glUniform3f(prog->getUniform("MatDif"), 0.9, 0.83, 0.4);
        glUniform3f(prog->getUniform("MatSpec"), 0.9, 0.83, 0.4);
        glUniform1f(prog->getUniform("shine"), 24.0);
        car->draw(prog);
    M->popMatrix();

    // grass plane
    M->pushMatrix();
        M->loadIdentity();
        M->translate(vec3(0.0, -1.0, 0.0));
        M->scale(vec3(50.0, 0.05, 50.0));
	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
        glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.83, 0.14);
        glUniform3f(prog->getUniform("MatDif"), 0.3, 0.83, 0.4);
        glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.83, 0.4);
        glUniform1f(prog->getUniform("shine"), 4.0);
	  	cube->draw(prog);
    M->popMatrix();

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
