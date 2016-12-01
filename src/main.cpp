#include <iostream>
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

int g_width, g_height;
float phi = 0.0;
float theta = 0.0;
static const float D80 = 1.39626;
int strafe = 0;
int zoom = 0;
float moveSpeed = 0.05;
vec3 camera = vec3(0, 0, 0);

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
                strafe = -1;
                break;
            case GLFW_KEY_D:
                strafe = 1;
                break;
            case GLFW_KEY_W:
                zoom = 1;
                break;
            case GLFW_KEY_S:
                zoom = -1;
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
                if (strafe == -1) {
                    strafe = 0;
                }
                break;
            case GLFW_KEY_D:
                if (strafe == 1) {
                    strafe = 0;
                }
                break;
            case GLFW_KEY_W:
                if (zoom == 1) {
                    zoom = 0;
                }
                break;
            case GLFW_KEY_S:
                if (zoom == -1) {
                    zoom = 0;
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

static void scroll_callback(GLFWwindow *window, double dx, double dy) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    theta += (dx/width) * 3.14 * 3;
    if ((dy < 0 && phi <= D80) || (dy > 0 && phi >= -D80)) {
        phi -= (dy/height) * 3.14 * 3;
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

    srand(static_cast <unsigned> (time(0)));
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
    vec3 up;
    if (cos(phi) < 0) {
        up = vec3(0.0, -1.0, 0.0);
    } else {
        up = vec3(0.0, 1.0, 0.0);
    }

    vec3 rot = vec3(cos(phi) * cos(theta), sin(phi), cos(phi) * cos((3.14/2) - theta));
    vec3 lap = camera + rot;
    vec3 forward = normalize(lap - camera);
    vec3 left = cross(up, forward);
    camera += forward * (moveSpeed * zoom);
    camera -= left * (moveSpeed * strafe);
    camera.y = 0.0;
    lap = camera + rot;
    V->lookAt(camera, lap, up);
    //V->translate(vec3(strafe * moveSpeed, 0.0, zoom * moveSpeed));
    // Apply perspective projection.
    P->pushMatrix();
    P->perspective(45.0f, aspect, 0.01f, 100.0f);

    prog->bind();
    glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
    glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
    glUniform3f(prog->getUniform("lightPos"), 6.0, 2.0, 2.0);

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
   glfwSetScrollCallback(window, scroll_callback);
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