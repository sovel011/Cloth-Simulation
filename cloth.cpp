#include <GL/glew.h>   //Include order can matter here
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;


//Global Variables
float timePast = 0;
float objx=0, objy=0.f, objz=0.0f;
bool DEBUG_ON = false;
bool MIDPOINT = false;
bool fullscreen = false;
const int N = 15;
const float clothHeight = 1.0f;
const float gravity = -0.05;
//float ks = 1500;
//const float kd = 20.f;
float ks = 35;
const float kd = 0.5f;
const float l0 = 0.13f;
bool drop = false;
float wind = 0.0f;
glm::vec3 sphereCenter = glm::vec3(0,0,0);

//Functions
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
void initializeCloth(float spacing);
void flattenClothMatrix(float*);
void update(float dt);
void midpointUpdate(float dt);
float dot(glm::vec3 v1, glm::vec3 v2);
glm::vec3 cross(glm::vec3 a, glm::vec3 b);
glm::vec3 normalize(glm::vec3);

//CLASS
class Point{
public:
    Point();
    Point(glm::vec3, glm::vec2);
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;
    glm::vec3 vel;
    glm::vec3 futureVel;
    glm::vec3 futurePos;
};

Point::Point(){}

Point::Point(glm::vec3 _pos, glm::vec2 _texCoord){
    pos = _pos;
    norm = glm::vec3(0.f,1.f,0.f);
    texCoord = _texCoord;
    vel = glm::vec3(0.0f,0.0f,0.0f);
    futureVel = glm::vec3(0.0f,0.0f,0.0f);
    futurePos = _pos;
}

Point cloth[N][N];

class Camera{
public:
    Camera();
    void moveBackward(double);
    void moveForward(double);
    void moveLeft(double);
    void moveRight(double);
    void rotDown(double);
    void rotUp(double);
    void rotLeft(double);
    void rotRight(double);
    glm::vec3 pos, look, up, dir;
    float theta, phi;
    bool backward, forward, left, right;
    bool rotDownB, rotUpB, rotLeftB, rotRightB;
private:
    void updateRot();
};

Camera::Camera(){
    pos = glm::vec3(2.f, 0.5f, 4.f);
    look = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    dir = look - pos;
    theta = 0; phi = 0;
    backward = false, forward = false, left = false, right = false;
    rotDownB = false, rotUpB = false, rotLeftB = false, rotRightB = false;
}

void Camera::updateRot(){
    phi = fmod(double(phi), 2 * M_PI);
    theta = fmod(double(theta), 2 * M_PI);
    glm::mat4 identity;
    glm::mat4 work;
    glm::vec4 dir4 = glm::vec4(0.0f, 0.0f, 10.0f, 0.0f);
    work = glm::rotate(identity, theta, glm::vec3(0.0f, 1.0f, 0.0f));
    dir4 = work * dir4;
    work = glm::rotate(identity, phi, glm::vec3(1.0f, 0.0f, 0.0f));
    dir4 = work * dir4;
    dir[0] = dir4[0];
    dir[1] = dir4[1];
    dir[2] = dir4[2];
    dir = glm::normalize(dir);
    look = pos + dir;
    
}

void Camera::moveBackward(double frameTime){
    pos -= 2.0f * float(frameTime) * dir;
    look = pos + dir;
}
void Camera::moveForward(double frameTime){
    pos += 2.0f * float(frameTime) * dir;
    look = pos + dir;
}
void Camera::moveLeft(double frameTime){
    pos -= 2.0f * float(frameTime) * glm::cross(dir, up);
    look = pos + dir;
}
void Camera::moveRight(double frameTime){
    pos += 2.0f * float(frameTime) * glm::cross(dir, up);
    look = pos + dir;
}

void Camera::rotDown(double frameTime){
    phi -= 0.50f * float(frameTime);
    updateRot();
}
void Camera::rotUp(double frameTime){
    phi += 0.50f * float(frameTime);
    updateRot();
}
void Camera::rotLeft(double frameTime){
    theta += 0.50f * float(frameTime);
    updateRot();
}
void Camera::rotRight(double frameTime){
    theta -= 0.50f * float(frameTime);
    updateRot();
}

Camera camera;

int main(int argc, char *argv[]){
    
    //INTITIALIZATION
    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)
    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	

	
	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
	
	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
	
	//GLEW loads new OpenGL functions
	glewExperimental = GL_TRUE; //Use the new way of testing which methods are supported
	glewInit();
	
	//Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context
	
    //INIT CLOTH MATRIX
    initializeCloth(l0);
    int clothDataSize = 48*(N-1)*(N-1);
    float clothData[clothDataSize];
    
	//MODELS
    ifstream modelFile;
    modelFile.open("models/sphere.txt");
    int numLines = 0;
    modelFile >> numLines;
    float* modelData = new float[numLines];
    for (int i = 0; i < numLines; i++){
        modelFile >> modelData[i];
    }
    int numTris = numLines/8;
    modelFile.close();
    
    int totalNumTris = numTris;
    
    //TEXTURES
    SDL_Surface* surface = SDL_LoadBMP("red.bmp");
    if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint wtex;
    glGenTextures(1, &wtex);
    glBindTexture(GL_TEXTURE_2D, wtex);
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    
	surface = SDL_LoadBMP("cloth.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    SDL_FreeSurface(surface);
    
	
	
	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[2];
	glGenBuffers(2, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    
	int shaderProgram = InitShader("vertexTex.glsl", "fragmentTex.glsl");
	glUseProgram(shaderProgram); //Set the active shader (only one can be used at a time)
	glEnable(GL_DEPTH_TEST);
    
    float newTime, frameTime = 0.0f;
	
	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	while (true){
      if (SDL_PollEvent(&windowEvent)){
        if (windowEvent.type == SDL_QUIT) break;
        //List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
        //Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) 
          break; //Exit event loop
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){ //If "f" is pressed
          //fullscreen = !fullscreen;
          //SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen
        }
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_SPACE){ //If "f" is pressed
            drop = true;
        }
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_LEFT){ //If "f" is pressed
            wind -= .5;
        }
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_RIGHT){ //If "f" is pressed
            wind += .5;
        }
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_UP){ //If "f" is pressed
            ks += .5;
        }
        if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_DOWN){ //If "f" is pressed
            ks -= .5;
        }
        if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_i){ //If "f" is pressed
            sphereCenter[2] -= 7*frameTime;
        }
        if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_j){ //If "f" is pressed
            sphereCenter[0] -= 7*frameTime;
        }
        if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_k){ //If "f" is pressed
            sphereCenter[2] += 7*frameTime;
        }
        if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_l){ //If "f" is pressed
            sphereCenter[0] += 7*frameTime;
        }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_s){
              camera.backward = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_w){
              camera.forward = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_a){
              camera.left = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_d){
              camera.right = true;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_s){
              camera.backward = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_w){
              camera.forward = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_a){
              camera.left = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_d){
              camera.right = false;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_g){
              camera.rotDownB = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_t){
              camera.rotUpB = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_f){
              camera.rotLeftB = true;
          }
          if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_h){
              camera.rotRightB = true;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_g){
              camera.rotDownB = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_t){
              camera.rotUpB = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){
              camera.rotLeftB = false;
          }
          if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_h){
              camera.rotRightB = false;
          }
      }
        
        if(camera.backward){
            camera.moveBackward(frameTime);
        }
        else if(camera.forward){
            camera.moveForward(frameTime);
        }
        if(camera.left){
            camera.moveLeft(frameTime);
        }
        else if(camera.right){
            camera.moveRight(frameTime);
        }
        if(camera.rotDownB){
            camera.rotDown(frameTime);
        }
        else if(camera.rotUpB){
            camera.rotUp(frameTime);
        }
        if(camera.rotLeftB){
            camera.rotLeft(frameTime);
        }
        else if(camera.rotRightB){
            camera.rotRight(frameTime);
        }
      
     newTime = SDL_GetTicks() / 1000.0;
     frameTime = newTime - timePast;
     timePast = newTime;
     double fps = 1.0 / frameTime;
     char window_title[30];
     sprintf(window_title, "Cloth Sim - %.1f",fps);
     SDL_SetWindowTitle(window,window_title);
     glUseProgram(shaderProgram);
        
     //frameTime = .005;
     if (MIDPOINT){
        midpointUpdate(frameTime);
     }
     else{
      update(frameTime);
     }
     flattenClothMatrix(clothData);

     
     //BIND BUFFERS AND DEFINE DATA
     glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
     glBufferData(GL_ARRAY_BUFFER, clothDataSize*sizeof(float), clothData, GL_STATIC_DRAW);
     
     //Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, normalized?, stride, offset
	  //Binds to VBO current GL_ARRAY_BUFFER 
	glEnableVertexAttribArray(posAttrib);

	//GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);
	
	GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
                       8*sizeof(float), (void*)(6*sizeof(float)));

      
      
      
      GLint uniModel = glGetUniformLocation(shaderProgram, "model");
      GLint uniView = glGetUniformLocation(shaderProgram, "view");
      GLint uniProj = glGetUniformLocation(shaderProgram, "proj");

      glm::mat4 view = glm::lookAt(camera.pos, camera.look, camera.up); //Up
        
        // Clear the screen to default color
        glClearColor(1.f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        
        
//        glm::mat4 view = glm::lookAt(
//      	glm::vec3(2.f, 0.5f, 4.f),  //Cam Position
//      	glm::vec3(0.0f, 0.0f, 0.0f),  //Look at point
//      	glm::vec3(0.0f, 1.0f, 0.0f)); //Up
      
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
      
      
        glm::mat4 proj = glm::perspective(3.14f/4, 800.0f / 600.0f, 1.0f, 100.0f); //FOV, aspect, near, far
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
      
      	glm::mat4 model;
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
      
        //DRAW CLOTH
        glBindTexture(GL_TEXTURE_2D, tex);
        glPointSize(5);
        glDrawArrays(GL_TRIANGLES, 0, clothDataSize/8); //(Primitives, Which VBO, Number of vertices)
        
        //DRAW SPHERE
        model = glm::translate(model, sphereCenter);
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, wtex);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, totalNumTris*8*sizeof(float), modelData, GL_STATIC_DRAW);
        
        //Tell OpenGL how to set fragment shader input
        posAttrib = glGetAttribLocation(shaderProgram, "position");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
        //Attribute, vals/attrib., type, normalized?, stride, offset
        //Binds to VBO current GL_ARRAY_BUFFER
        glEnableVertexAttribArray(posAttrib);
        
        normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
        glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
        glEnableVertexAttribArray(normAttrib);
        
        texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
                              8*sizeof(float), (void*)(3*sizeof(float)));
        
        glDrawArrays(GL_TRIANGLES, 0, totalNumTris);
        
      
        SDL_GL_SwapWindow(window); //Double buffering
	}
	
	glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	//Clean Up
	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

void initializeCloth(float spacing){
    
    float clothWidth = spacing*(N-1);
    float currX = -clothWidth/2.0;
    float initZ = -clothWidth/2.0;
    float currZ;
    for (int i = 0; i < N; i++){
        currZ = initZ;
        for (int j = 0; j < N; j++){
            glm::vec3 p = glm::vec3(currX,clothHeight,currZ);
            glm::vec2 t = glm::vec2(j/(float)(N-1), i/(float)(N-1));
            cloth[i][j] = Point(p,t);
            currZ += spacing;
        }
        currX += spacing;
    }
}

void printCloth(){
        for (int i = 0; i < N; i++){
            for (int j = 0; j < N; j++){
                printf("(%.2g, %.2g, %.2g) ",cloth[i][j].vel[0],cloth[i][j].vel[1],cloth[i][j].vel[2]);
            }
            printf("\n");
        }
        printf("\n");
}

void flattenClothMatrix2(float* clothData){
    
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            int index = 3*(i*N+j);
            clothData[index] = cloth[i][j].pos[0];
            clothData[index+1] = cloth[i][j].pos[1];
            clothData[index+2] = cloth[i][j].pos[2];
        }
    }
//    for (int i = 0; i < 3*N*N; i+=3){
//        printf("(%f %f %f)\n",clothData[i],clothData[i+1],clothData[i+2]);
//    }
}

void flattenClothMatrix(float* clothData){
    
    for (int i = 0; i < N-1; i++){
        for (int j = 0; j < N-1; j++){
            int index = 48*(i*(N-1)+j);
            //TRIANGLE 1
            //vert 1
            clothData[index] = cloth[i][j].pos[0];
            clothData[index+1] = cloth[i][j].pos[1];
            clothData[index+2] = cloth[i][j].pos[2];
            
            clothData[index+3] = cloth[i][j].norm[0];
            clothData[index+4] = cloth[i][j].norm[1];
            clothData[index+5] = cloth[i][j].norm[2];
            
            clothData[index+6] = cloth[i][j].texCoord[0];
            clothData[index+7] = cloth[i][j].texCoord[1];
            
            //vert 2
            clothData[index+8] = cloth[i+1][j].pos[0];
            clothData[index+9] = cloth[i+1][j].pos[1];
            clothData[index+10] = cloth[i+1][j].pos[2];
            
            clothData[index+11] = cloth[i+1][j].norm[0];
            clothData[index+12] = cloth[i+1][j].norm[1];
            clothData[index+13] = cloth[i+1][j].norm[2];
            
            clothData[index+14] = cloth[i+1][j].texCoord[0];
            clothData[index+15] = cloth[i+1][j].texCoord[1];
            
            //vert 3
            clothData[index+16] = cloth[i][j+1].pos[0];
            clothData[index+17] = cloth[i][j+1].pos[1];
            clothData[index+18] = cloth[i][j+1].pos[2];
            
            clothData[index+19] = cloth[i][j+1].norm[0];
            clothData[index+20] = cloth[i][j+1].norm[1];
            clothData[index+21] = cloth[i][j+1].norm[2];
            
            clothData[index+22] = cloth[i][j+1].texCoord[0];
            clothData[index+23] = cloth[i][j+1].texCoord[1];
            
            //TRIANGLE 2
            //vert 1
            clothData[index+24] = cloth[i+1][j].pos[0];
            clothData[index+25] = cloth[i+1][j].pos[1];
            clothData[index+26] = cloth[i+1][j].pos[2];
            
            clothData[index+27] = cloth[i+1][j].norm[0];
            clothData[index+28] = cloth[i+1][j].norm[1];
            clothData[index+29] = cloth[i+1][j].norm[2];
            
            clothData[index+30] = cloth[i+1][j].texCoord[0];
            clothData[index+31] = cloth[i+1][j].texCoord[1];
            
            //vert 2
            clothData[index+32] = cloth[i+1][j+1].pos[0];
            clothData[index+33] = cloth[i+1][j+1].pos[1];
            clothData[index+34] = cloth[i+1][j+1].pos[2];
            
            clothData[index+35] = cloth[i+1][j+1].norm[0];
            clothData[index+36] = cloth[i+1][j+1].norm[1];
            clothData[index+37] = cloth[i+1][j+1].norm[2];
            
            clothData[index+38] = cloth[i+1][j+1].texCoord[0];
            clothData[index+39] = cloth[i+1][j+1].texCoord[1];
            
            //vert 3
            clothData[index+40] = cloth[i][j+1].pos[0];
            clothData[index+41] = cloth[i][j+1].pos[1];
            clothData[index+42] = cloth[i][j+1].pos[2];
            
            clothData[index+43] = cloth[i][j+1].norm[0];
            clothData[index+44] = cloth[i][j+1].norm[1];
            clothData[index+45] = cloth[i][j+1].norm[2];
            
            clothData[index+46] = cloth[i][j+1].texCoord[0];
            clothData[index+47] = cloth[i][j+1].texCoord[1];
        }
    }
    //    for (int i = 0; i < 3*N*N; i+=3){
    //        printf("(%f %f %f)\n",clothData[i],clothData[i+1],clothData[i+2]);
    //    }
}

void update(float dt){
    //printCloth();
    //vertical
    for (int i = 0; i < N-1; i++){
        for (int j = 0; j < N; j++){
            glm::vec3 e = cloth[i+1][j].pos - cloth[i][j].pos;
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].vel);
            float v2 = dot(e,cloth[i+1][j].vel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].vel += f*e;
            cloth[i+1][j].vel -= f*e;
        }
    }
    //horizontal
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N-1; j++){
            glm::vec3 e = cloth[i][j+1].pos - cloth[i][j].pos;
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].vel);
            float v2 = dot(e,cloth[i][j+1].vel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].vel += f*e;
            cloth[i][j+1].vel -= f*e;
        }
    }
    //change pos
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            float distToOrigin = sqrt(dot(cloth[i][j].pos-sphereCenter,cloth[i][j].pos-sphereCenter));
            if (cloth[i][j].pos[1] - (-2.0) < .02f){
                continue;
            }
//            if (i == 0 && (j == 0 || j == N-1)){
//                cloth[i][j].vel = glm::vec3(0,0,0);
//            }
            if (i == 0 && !drop){
                cloth[i][j].vel = glm::vec3(0,0,0);
            }
            else if (distToOrigin <= .55){
                glm::vec3 n = -1.0f*(sphereCenter - cloth[i][j].pos);
                n = n/distToOrigin;
                glm::vec3 bounce = dot(cloth[i][j].vel,n)*n;
                cloth[i][j].vel -= (bounce);
                float bounceScale = (.55 - distToOrigin);
                bounce = bounceScale*n;
                cloth[i][j].pos += bounce;
            }
            else{
                glm::vec3 a = glm::vec3(wind*dt,gravity,0.f);
                cloth[i][j].vel += a;
                //aero force
                glm::vec3 vel;
                glm::vec3 n1;
                if (i < N-1 && j < N-1){
                    vel = (cloth[i][j].vel+cloth[i+1][j].vel+cloth[i+1][j+1].vel)/3.0f;
                    //vel = vel - glm::vec3(wind*dt,0,0);
                    n1 = cross(cloth[i+1][j].pos - cloth[i][j].pos, cloth[i+1][j+1].pos - cloth[i][j].pos);
                }
                else if (i == N-1 && j < N-1){
                    vel = (cloth[i][j].vel+cloth[i-1][j].vel+cloth[i-1][j+1].vel)/3.0f;
                   // vel = vel - glm::vec3(wind*dt,0,0);
                    n1 = cross(cloth[i-1][j+1].pos - cloth[i][j].pos,cloth[i-1][j].pos - cloth[i][j].pos);

                }
                else if (i == 0 && j == N-1){
                    vel = (cloth[i][j].vel+cloth[i+1][j].vel+cloth[i+1][j-1].vel)/3.0f;
                    //vel = vel - glm::vec3(wind*dt,0,0);
                    n1 = cross(cloth[i+1][j-1].pos - cloth[i][j].pos,cloth[i+1][j].pos - cloth[i][j].pos);

                }
                else{
                    vel = (cloth[i][j].vel+cloth[i-1][j].vel+cloth[i-1][j-1].vel)/3.0f;
                    //vel = vel - glm::vec3(wind*dt,0,0);
                    n1 = cross(cloth[i-1][j].pos - cloth[i][j].pos, cloth[i-1][j-1].pos - cloth[i][j].pos);
                }
                float va = (sqrt(dot(vel,vel))*dot(vel,n1)) / (-4.0f*sqrt(dot(n1,n1)));
                glm::vec3 aeroForce = va*n1;
                cloth[i][j].vel += aeroForce;
                cloth[i][j].pos += cloth[i][j].vel*dt;
                if (cloth[i][j].pos[1] < -2.0f){
                    cloth[i][j].pos[1] = -2.0f;
                }
            }
            //calculate normals
            if (i < (N-1)){
                glm::vec3 a = normalize(cloth[i+1][j].pos - cloth[i][j].pos);
                glm::vec3 b = normalize(cloth[i][j+1].pos - cloth[i][j].pos);
                glm::vec3 normal = cross(b,a);
                cloth[i][j].norm = normal;
            }
            else{
                glm::vec3 a = normalize(cloth[i-1][j].pos - cloth[i][j].pos);
                glm::vec3 b = normalize(cloth[i][j-1].pos - cloth[i][j].pos);
                glm::vec3 normal = cross(b,a);
                cloth[i][j].norm = normal;
            }
        }
    }
}

void midpointUpdate(float dt){
    //printCloth();
    //vertical
    float halfDt = dt / 2.0;
    for (int i = 0; i < N-1; i++){
        for (int j = 0; j < N; j++){
            glm::vec3 e = cloth[i+1][j].pos - cloth[i][j].pos;
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].vel);
            float v2 = dot(e,cloth[i+1][j].vel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].futureVel = cloth[i][j].vel + f*e;
            cloth[i+1][j].futureVel = cloth[i+1][j].vel - f*e;
        }
    }
    //horizontal
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N-1; j++){
            glm::vec3 e = cloth[i][j+1].pos - cloth[i][j].pos;
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].vel);
            float v2 = dot(e,cloth[i][j+1].vel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].futureVel += f*e;
            cloth[i][j+1].futureVel -= f*e;
        }
    }
    //find state at 1/2 timestep
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            if (cloth[i][j].pos[1] - (-2.0) < .02f){
                continue;
            }
            else{
                glm::vec3 a = glm::vec3(wind,gravity,0.f);
                cloth[i][j].futureVel += a;
                cloth[i][j].futurePos += cloth[i][j].futureVel*halfDt;
            }
        }
    }
    
    //vertical
    for (int i = 0; i < N-1; i++){
        for (int j = 0; j < N; j++){
            glm::vec3 e = cloth[i+1][j].futurePos - cloth[i][j].futurePos;//
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].futureVel);
            float v2 = dot(e,cloth[i+1][j].futureVel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].vel += f*e;
            cloth[i+1][j].vel -= f*e;
        }
    }
    //horizontal
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N-1; j++){
            glm::vec3 e = cloth[i][j+1].futurePos - cloth[i][j].futurePos;
            float l = sqrt(dot(e,e));
            e = e * (1.0f/l);
            float v1 = dot(e,cloth[i][j].futureVel);
            float v2 = dot(e,cloth[i][j+1].futureVel);
            float f = (-1.0f*ks*(l0-l))-(kd*(v1-v2));
            cloth[i][j].vel += f*e;
            cloth[i][j+1].vel -= f*e;
        }
    }
    //change pos
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            float distToOrigin = sqrt(dot(cloth[i][j].futurePos-sphereCenter,cloth[i][j].futurePos-sphereCenter));
            if (cloth[i][j].futurePos[1] - (-2.0) < .02f){
                continue;
            }
            //            if (i == 0 && (j == 0 || j == N-1)){
            //                cloth[i][j].vel = glm::vec3(0,0,0);
            //            }
            if (i == 0 && !drop){
                cloth[i][j].vel = glm::vec3(0,0,0);
            }
            else if (distToOrigin <= .55 && false){
                glm::vec3 n = -1.0f*(sphereCenter - cloth[i][j].futurePos);
                n = n/distToOrigin;
                glm::vec3 bounce = dot(cloth[i][j].futureVel,n)*n;
                cloth[i][j].vel -= (bounce);
                float bounceScale = (.55 - distToOrigin);
                bounce = bounceScale*n;
                cloth[i][j].pos += bounce;
            }
            else{
                glm::vec3 a = glm::vec3(wind,gravity,0.f);
                cloth[i][j].vel += a;
                cloth[i][j].pos += cloth[i][j].futureVel*dt;
                if (cloth[i][j].pos[1] < -2.0f){
                    cloth[i][j].pos[1] = -2.0f;
                }
            }
            cloth[i][j].futureVel = cloth[i][j].vel;
            cloth[i][j].futurePos = cloth[i][j].pos;
            //calculate normals
            if (i < (N-1)){
                glm::vec3 a = normalize(cloth[i+1][j].pos - cloth[i][j].pos);
                glm::vec3 b = normalize(cloth[i][j+1].pos - cloth[i][j].pos);
                glm::vec3 normal = cross(b,a);
                cloth[i][j].norm = normal;
            }
            else{
                glm::vec3 a = normalize(cloth[i-1][j].pos - cloth[i][j].pos);
                glm::vec3 b = normalize(cloth[i][j-1].pos - cloth[i][j].pos);
                glm::vec3 normal = cross(b,a);
                cloth[i][j].norm = normal;
            }
        }
    }

}

float dot(glm::vec3 v1, glm::vec3 v2){
    return (v1[0]*v2[0]) + (v1[1]*v2[1]) + (v1[2]*v2[2]);
}

glm::vec3 normalize(glm::vec3 v){
    float magnitude = sqrt(dot(v,v));
    return v*(1/magnitude);
}

glm::vec3 cross(glm::vec3 a, glm::vec3 b){
    float x = a[1]*b[2] - a[2]*b[1];
    float y = a[2]*b[0] - a[0]*b[2];
    float z = a[0]*b[1] - a[1]*b[0];
    return glm::vec3(x,y,z);
}


// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile)
{
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName)
{
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);
	glUseProgram(program);

	return program;
}



/*
What's Left:
 
 aerodynamic force?
 midpoint integration and compares?
 camera?
 more user interaction?
 
 
 
 */








