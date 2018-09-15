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
bool fullscreen = false;
int N = 5;

//Functions
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
float* generateCloth();

int main(int argc, char *argv[]){
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
	
	
	//Load Sphere
	ifstream modelFile;
	modelFile.open("models/sphere.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* modelData = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> modelData[i];
	}
	printf("%d\n",numLines);
	int numTris = numLines/8;
	modelFile.close();

	int totalNumTris = numTris;
    
    //Load Cloth
    float* clothData = generateCloth();
    int numClothTris = N*N;
	
	SDL_Surface* surface = SDL_LoadBMP("ball.bmp");
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
          fullscreen = !fullscreen;
          SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
        }
      }
      
      
      
      timePast = SDL_GetTicks()/1000.f;
      
     glUseProgram(shaderProgram); 
     
     glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
     glBufferData(GL_ARRAY_BUFFER, totalNumTris*8*sizeof(float), modelData, GL_STATIC_DRAW);
     
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
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
                       8*sizeof(float), (void*)(3*sizeof(float)));

      
      
      
      GLint uniModel = glGetUniformLocation(shaderProgram, "model");
      GLint uniView = glGetUniformLocation(shaderProgram, "view");
      GLint uniProj = glGetUniformLocation(shaderProgram, "proj");

        
        // Clear the screen to default color
        glClearColor(.2f, 0.4f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        
        
        glm::mat4 view = glm::lookAt(
      	glm::vec3(0.f, 0.5f, 4.f),  //Cam Position
      	glm::vec3(0.0f, 0.50f, 0.0f),  //Look at point
      	glm::vec3(0.0f, 1.0f, 0.0f)); //Up
      
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
      
      
        glm::mat4 proj = glm::perspective(3.14f/4, 800.0f / 600.0f, 1.0f, 100.0f); //FOV, aspect, near, far
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
      
      	glm::mat4 model;
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
      
        //SJG: Here we draw only the first object (start at 0, draw numTris1 triangles)  
        glDrawArrays(GL_TRIANGLES, 0, numTris); //(Primitives, Which VBO, Number of vertices)
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, numClothTris*8*sizeof(float), clothData, GL_STATIC_DRAW);
        
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
        
        glDrawArrays(GL_TRIANGLES, 0, numClothTris);
      
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


float* generateCloth(){
    //how many points the cloth is made of
    float* clothData = new float[(N*N)*48];
    float clothHeight = 1.5f;
    float clothWidth = 3.f;
    float currX = 0.f - (clothWidth/2.0);
    float currZ = -1*currX;
    float stepSize = clothWidth / N;
    
    for (int i=0; i<(N-1); i++){
        
        for (int j=0; j<N; j++){
            int k = (N*i)+j;
            clothData[k*48] = currX;
            clothData[k*48+1] = clothHeight;
            clothData[k*48+2] = currZ;
            
            clothData[k*48+3] = 0.0f;
            clothData[k*48+4] = 1.0f;
            clothData[k*48+5] = 0.0f;
            
            clothData[k*48+6] = j/(N-1);
            clothData[k*48+7] = i/(N-1);
            //
            clothData[k*48*8] = currX+stepSize;
            clothData[k*48+9] = clothHeight;
            clothData[k*48+10] = currZ;
            
            clothData[k*48+11] = 0.0f;
            clothData[k*48+12] = 1.0f;
            clothData[k*48+13] = 0.0f;
            
            clothData[k*48+14] = j/(N-1);
            clothData[k*48+15] = (i+1)/(N-1);
            //
            clothData[k*48*16] = currX;
            clothData[k*48+17] = clothHeight;
            clothData[k*48+18] = currZ-stepSize;
            
            clothData[k*48+19] = 0.0f;
            clothData[k*48+20] = 1.0f;
            clothData[k*48+21] = 0.0f;
            
            clothData[k*48+22] = (j+1)/(N-1);
            clothData[k*48+23] = (i)/(N-1);
            //
            clothData[k*48*24] = currX+stepSize;
            clothData[k*48+25] = clothHeight;
            clothData[k*48+26] = currZ;
            
            clothData[k*48+27] = 0.0f;
            clothData[k*48+28] = 1.0f;
            clothData[k*48+29] = 0.0f;
            
            clothData[k*48+30] = (j)/(N-1);
            clothData[k*48+31] = (i+1)/(N-1);
            //
            clothData[k*48*32] = currX+stepSize;
            clothData[k*48+33] = clothHeight;
            clothData[k*48+34] = currZ-stepSize;
            
            clothData[k*48+35] = 0.0f;
            clothData[k*48+36] = 1.0f;
            clothData[k*48+37] = 0.0f;
            
            clothData[k*48+38] = (j+1)/(N-1);
            clothData[k*48+39] = (i+1)/(N-1);
            //
            clothData[k*48*40] = currX;
            clothData[k*48+41] = clothHeight;
            clothData[k*48+42] = currZ-stepSize;
            
            clothData[k*48+43] = 0.0f;
            clothData[k*48+44] = 1.0f;
            clothData[k*48+45] = 0.0f;
            
            clothData[k*48+46] = (j+1)/(N-1);
            clothData[k*48+47] = (i+1)/(N-1);
            
            currZ -= stepSize;
        }
        currX += stepSize;
    }
    return clothData;
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