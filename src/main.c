#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
// TODO: Get Sleep function in other Operating Systems.

// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL Libraries.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h> // OpenGL Platform/Window/Input Abstraction Layer
#include <glad/glad.h> // OpenGL Bindings

#include <cglm/cglm.h> // OpenGL Maths for C

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb/stb_image.h>

// Textures
#include "textures/T_NUMBERS.h"
#include "textures/T_VIEW2D.h"
#include "textures/T_00.h"
#include "textures/T_01.h"
#include "textures/T_02.h"
#include "textures/T_03.h"
#include "textures/T_04.h"
#include "textures/T_05.h"
#include "textures/T_06.h"
#include "textures/T_07.h"
#include "textures/T_08.h"
#include "textures/T_09.h"
#include "textures/T_10.h"
#include "textures/T_11.h"
#include "textures/T_12.h"
#include "textures/T_13.h"
#include "textures/T_14.h"
#include "textures/T_15.h"
#include "textures/T_16.h"
#include "textures/T_17.h"
#include "textures/T_18.h"
#include "textures/T_19.h"
int numText = 19;			//number of textures

// Macros
#define BACKGROUND (RGB){ 0x00, 0x3C, 0x82 }
#define YELLOW (RGB){ 0xff, 0xff, 0x00 }
#define GREEN (RGB){ 0x00, 0xff, 0x00 }
#define CYAN (RGB){ 0x00, 0xff, 0xff }
#define BROWN (RGB){ 0xA0, 0x64, 0x00 }
#define DARK_YELLOW (RGB){ 0xA0, 0xA0, 0x00 }
#define DARK_GREEN (RGB){ 0x00, 0xA0, 0x00 }
#define DARK_CYAN (RGB){ 0x00, 0xA0, 0xA0 }
#define DARK_BROWN (RGB){ 0x6E, 0x32, 0x00 }

// Constants
const char *window_name = "Pixel Test";

const unsigned int buffer_width = 160;
const unsigned int buffer_height = 120;
const unsigned int buffer_channels = 3;

const double targetFPS = 35.0; // default: 35

// Structs
typedef struct
{
	union
	{
		struct
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
		};
		unsigned int rgb;
	};
} RGB;

typedef struct
{
	float cos[360];
	float sin[360];
} Math;

typedef struct
{
	int w, a, s, d; // Movement
	int sl, sr;		// Strafing
	int m;			// Modifier
} PlayerInput;

typedef struct
{
	int x, y, z;
	int angle;
	int look;
} Player;

typedef struct
{
	int x1, y1;	// Bottom line coordinates.
	int x2, y2;	//
	int c;		// Wall Color.
	int wt;		// Wall texture.
	int u, v;	// Wall UVs.
	int shade;	// Wall shade.
} Wall;

typedef struct
{
	int ws, we;		// Wall start and end.
	int z1, z2;		// Height of sector.
	int x, y;		// Sector position.
	int d;			// For sorting drawing order.
	int c1, c2;		// Bottom and top colors.
	int st, ss;		// Surface texture, and the scale.
	int surf[160];	// Surface points. TODO: Set array count to Buffer Width
	int surface;	// Surface check.
} Sector;

typedef struct
{
	int w, h;					// Texture width and height.
	const unsigned char *name;	// Texture Name.
} TextureMap;

// Global Variables
GLFWwindow *window;

unsigned int indiceCount = 0;

unsigned int VAO;
unsigned int VBO;
unsigned int EBO;
unsigned int PBO[2];

unsigned int vertexShader, fragmentShader, shaderProgram;
mat4 view, projection;

unsigned int texture;

unsigned char *imageBuffer;
unsigned char *framebuffer[4];
size_t fbuffer_count = 4;
size_t buffer_size;
unsigned int activeFramebuffer = 0;

unsigned int scale = 4;
unsigned int screen_width;
unsigned int screen_height;

float fov = 200;

Math math;
PlayerInput playerInput;
Player player;

TextureMap textures[64];

unsigned int sectorCount;
unsigned int wallCount;
Wall walls[256];
Sector sectors[128];

// Callbacks
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

// Forward Declaration
void start();
void shutdown();

void initSharedMemory();

void runGame();
void initGame();
void tick();
void render();
void cleanupGame();

void initOpenGL();
void startOpenGLRender();
void endOpenGLRender();
void cleanupOpenGL();

void clearBackground(unsigned char *framebuffer, const RGB color);
void drawPixel(unsigned char *framebuffer, const int x, const int y, const RGB color);

void loadScene();
void draw3D();
void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack);
void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2);
int distance(int x1, int y1, int x2, int y2);

// Entry Point
int main(int argc, char *argv[])
{
	screen_width = buffer_width * scale;
	screen_height = buffer_height * scale;

	start();
	shutdown();

	return 0;
}

// Function Definitions
void start()
{
	initSharedMemory();
	initOpenGL();
	runGame();
}
void shutdown()
{
	cleanupOpenGL();
}

void initSharedMemory()
{
	buffer_size = buffer_width * buffer_height * buffer_channels;

	// Create Image Buffer.
	imageBuffer = (unsigned char *)calloc(buffer_size, sizeof(unsigned char));

	// Create Frame Buffers.
	for (int i = 0; i < fbuffer_count; ++i)
		framebuffer[i] = (unsigned char *)calloc(buffer_size, sizeof(unsigned char));
}

void initOpenGL()
{
	// Constants
	const char *vertexCode =
		"#version 330 core\n"
		"layout (location = 0) in vec2 aPos;\n"
		"out vec2 texCoord;\n"
		"uniform mat4 view;\n"
		"uniform mat4 projection;\n"
		"void main()\n"
		"{\n"
		"	texCoord = aPos;\n"
		"	gl_Position = projection * view * vec4(aPos.xy, 0.0f, 1.0);\n"
		"}\n";
	const char *fragmentCode =
		"#version 330 core\n"
		"out vec4 fragColor;\n"
		"in vec2 texCoord;\n"
		"uniform sampler2D texture1;\n"
		"void main()\n"
		"{\n"
		"	vec4 color = texture(texture1, texCoord);\n"
		"	fragColor = color;\n"
		"}\n";

	const float vertices[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	const unsigned int indices[] =
	{
		0, 3, 1,
		3, 2, 1
	};
	indiceCount = sizeof(indices) / sizeof(unsigned int);

	// Setup OpenGL Context.
	glfwInit();

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(screen_width, screen_height, window_name, NULL, NULL);
	glfwMakeContextCurrent(window);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Setup Callbacks.
	glfwSetKeyCallback(window, keyCallback);

	// Initialize OpenGL Scene.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearStencil(0);
	glClearDepth(1.0f);

	glDepthFunc(GL_LEQUAL);
	glViewport(0, 0, screen_width, screen_height);

	// Create Shaders.
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexCode, NULL);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentCode, NULL);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Create Quad Geometry.
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Create Texture.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer_width, buffer_height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBuffer);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create Pixel Buffers.
	glGenBuffers(2, PBO);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, buffer_size, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, buffer_size, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	// Setup Perspective Matrices.
	glm_mat4_identity(view);
	glm_translate(view, (vec3) { 0.0f, 0.0f, -1.0f });

	glm_ortho(0.0f, 1.0f, 1.0f, 0.0f, 0.1f, 1000.0f, projection);

	// Set Shader Uniforms.
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);
}
void startOpenGLRender()
{
	// Copy active framebuffer to image buffer.
	memcpy(imageBuffer, framebuffer[activeFramebuffer], buffer_size);

	// Get Dual Pixel Buffer index.
	static int index = 0;
	int nextIndex = 0;

	index = (index + 1) % 2;
	nextIndex = (index + 1) % 2;

	// Update Texture with Pixel Buffer.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[index]);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer_width, buffer_height, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Copy Image Buffer to Pixel Buffer.
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[nextIndex]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, buffer_size, 0, GL_STREAM_DRAW);
	unsigned char *dst = (unsigned char *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (dst)
	{
		unsigned char *ptr = (unsigned char *)dst;
		for (size_t i = 0; i < buffer_size; ++i)
		{
			*ptr = imageBuffer[i];
			++ptr;
		}

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
void endOpenGLRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Draw quad.
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_INT, 0);

	// Unbind.
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, texture);
}
void cleanupOpenGL()
{
	// Destroy.
	for (int i = 0; i < fbuffer_count; ++i)
	{
		free(framebuffer[i]);
		framebuffer[i] = 0;
	}

	free(imageBuffer);
	imageBuffer = 0;

	glDeleteTextures(1, &texture);
	glDeleteBuffers(2, PBO);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);

	glfwDestroyWindow(window);
	glfwTerminate();
}

void runGame()
{
	initGame();

	// Game Loop.
	double timeBetweenFrames = 1.0f / targetFPS;

	double now = 0.0;
	double lastTime = glfwGetTime();
	double deltaTime = 0.0;
	double timer = lastTime;

	int ticks = 0;
	int frames = 0;

	bool shouldRender = false;
	while (!glfwWindowShouldClose(window))
	{
		now = glfwGetTime();
		deltaTime += (now - lastTime) / timeBetweenFrames;
		lastTime = now;

		shouldRender = false;
		while (deltaTime >= 1.0)
		{
			tick();
			ticks++;
			shouldRender = true;

			deltaTime -= 1.0;
		}

		double timeToSleep = (1 - deltaTime) * 1000.0 / targetFPS;
#ifdef _WIN32
		Sleep(timeToSleep);
#endif

		if (shouldRender)
		{
			startOpenGLRender();
			render();
			endOpenGLRender();
			frames++;
		}

		if (glfwGetTime() - timer > 1.0)
		{
			timer += 1.0;

			printf("%i ticks, %i fps\n", ticks, frames);

			ticks = 0;
			frames = 0;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	cleanupGame();
}

void initGame()
{
	// Setup Math Lookups.
	for (int i = 0; i < 360; ++i)
	{
		math.cos[i] = cos(i/180.0*M_PI);
		math.sin[i] = sin(i/180.0*M_PI);
	}

	// Setup Textures.
	textures[0].name = T_00; textures[0].h = T_00_HEIGHT; textures[0].w = T_00_WIDTH;
	textures[1].name = T_01; textures[1].h = T_01_HEIGHT; textures[1].w = T_01_WIDTH;
	textures[2].name = T_02; textures[2].h = T_02_HEIGHT; textures[2].w = T_02_WIDTH;
	textures[3].name = T_03; textures[3].h = T_03_HEIGHT; textures[3].w = T_03_WIDTH;
	textures[4].name = T_04; textures[4].h = T_04_HEIGHT; textures[4].w = T_04_WIDTH;
	textures[5].name = T_05; textures[5].h = T_05_HEIGHT; textures[5].w = T_05_WIDTH;
	textures[6].name = T_06; textures[6].h = T_06_HEIGHT; textures[6].w = T_06_WIDTH;
	textures[7].name = T_07; textures[7].h = T_07_HEIGHT; textures[7].w = T_07_WIDTH;
	textures[8].name = T_08; textures[8].h = T_08_HEIGHT; textures[8].w = T_08_WIDTH;
	textures[9].name = T_09; textures[9].h = T_09_HEIGHT; textures[9].w = T_09_WIDTH;
	textures[10].name = T_10; textures[10].h = T_10_HEIGHT; textures[10].w = T_10_WIDTH;
	textures[11].name = T_11; textures[11].h = T_11_HEIGHT; textures[11].w = T_11_WIDTH;
	textures[12].name = T_12; textures[12].h = T_12_HEIGHT; textures[12].w = T_12_WIDTH;
	textures[13].name = T_13; textures[13].h = T_13_HEIGHT; textures[13].w = T_13_WIDTH;
	textures[14].name = T_14; textures[14].h = T_14_HEIGHT; textures[14].w = T_14_WIDTH;
	textures[15].name = T_15; textures[15].h = T_15_HEIGHT; textures[15].w = T_15_WIDTH;
	textures[16].name = T_16; textures[16].h = T_16_HEIGHT; textures[16].w = T_16_WIDTH;
	textures[17].name = T_17; textures[17].h = T_17_HEIGHT; textures[17].w = T_17_WIDTH;
	textures[18].name = T_18; textures[18].h = T_18_HEIGHT; textures[18].w = T_18_WIDTH;
	textures[19].name = T_19; textures[19].h = T_19_HEIGHT; textures[19].w = T_19_WIDTH;

	// Setup Player.
	player.x = 70;
	player.y = -110;
	player.z = 20;
	player.angle = 0;
	player.look = 0;
}
void cleanupGame()
{

}

int tickCount = 0;
void tick()
{
	// Handle Player Movement.
	int dx = math.sin[player.angle] * 10;
	int dy = math.cos[player.angle] * 10;
	if (playerInput.m) // Modifier Key Down
	{
		// Look.
		if (playerInput.w) { player.z -= 4; }
		if (playerInput.s) { player.z += 4; }
		if (playerInput.a) { player.look -= 1; }
		if (playerInput.d) { player.look += 1; }
	}
	else // Modifier Key Up
	{
		// Movement.
		if (playerInput.w) { player.x += dx; player.y += dy; }
		if (playerInput.s) { player.x -= dx; player.y -= dy; }
		if (playerInput.a) { player.angle -= 4; if (player.angle < 0) { player.angle += 360; } }
		if (playerInput.d) { player.angle += 4; if (player.angle >= 360) { player.angle -= 360; } }
	}
	// Handle Strafing
	if (playerInput.sr) { player.x += dy; player.y -= dx; }
	if (playerInput.sl) { player.x -= dy; player.y += dx; }

	tickCount++;
}

void render()
{
	// Draw to Image Buffer.
	clearBackground(framebuffer[0], BACKGROUND);
	//drawFloors();
	draw3D();
}

void clearBackground(unsigned char *framebuffer, const RGB color)
{
	for (size_t y = 0; y < buffer_height; ++y)
	{
		for (size_t x = 0; x < buffer_width; ++x)
		{
			drawPixel(framebuffer, x, y, color);
		}
	}
}
void drawPixel(unsigned char *framebuffer, const int x, const int y, const RGB color)
{
	if (x > buffer_width-1 || x < 0 || y > buffer_height-1 || y < 0) // Only draw pixel within buffer resolution.
		return;

	int xx = x * buffer_channels;
	int yy = y * buffer_channels;
	int index = xx + yy * buffer_width;

	framebuffer[index++] = color.r;
	framebuffer[index++] = color.g;
	framebuffer[index++] = color.b;
}

void loadScene()
{
	// Open and read file.
	FILE *fp = fopen("./res/levels/level", "r");
	if (fp == NULL) { printf("Error opening level."); return; }

	// Load Scene.
	fscanf(fp, "%i", &sectorCount);				// Number of sectors.
	for (int s = 0; s < sectorCount; s++)		// Loop through sectors.
	{
		fscanf(fp, "%i", &sectors[s].ws);
		fscanf(fp, "%i", &sectors[s].we);
		fscanf(fp, "%i", &sectors[s].z1);
		fscanf(fp, "%i", &sectors[s].z2);
		fscanf(fp, "%i", &sectors[s].st);
		fscanf(fp, "%i", &sectors[s].ss);
	}
	fscanf(fp, "%i", &wallCount);				// Number of walls.
	for (int w = 0; w < wallCount; w++)			// Loop through walls.
	{
		fscanf(fp, "%i", &walls[w].x1);
		fscanf(fp, "%i", &walls[w].y1);
		fscanf(fp, "%i", &walls[w].x2);
		fscanf(fp, "%i", &walls[w].y2);
		fscanf(fp, "%i", &walls[w].wt);
		fscanf(fp, "%i", &walls[w].u);
		fscanf(fp, "%i", &walls[w].v);
		fscanf(fp, "%i", &walls[w].shade);
	}
	// Load player properties.
	fscanf(fp, "%i %i %i %i %i", &player.x, &player.y, &player.z, &player.angle, &player.look);

	// Close file.
	fclose(fp);
}

void draw3D()
{
	int cycles = 0;

	// Draw 3D
	int wx[4], wy[4], wz[4]; // World Positions
	float CS = math.cos[player.angle]; // Player Cosine
	float SN = math.sin[player.angle]; // Player Sine

	// Sort sectors.
	for (int s = 0; s < sectorCount; ++s)
	{
		for (int w = 0; w < sectorCount-s-1; ++w)
		{
			if (sectors[w].d < sectors[w+1].d)
			{
				Sector st = sectors[w];
				sectors[w] = sectors[w+1];
				sectors[w+1] = st;
			}
		}
	}

	// Draw Sectors.
	for (int s = 0; s < sectorCount; ++s)
	{
		sectors[s].d = 0; // Clear distance.

		if		(player.z < sectors[s].z1)	{ sectors[s].surface = 1; cycles = 2; for (int x = 0; x < buffer_width; ++x) { sectors[s].surf[x] = buffer_height; } }
		else if (player.z > sectors[s].z2)	{ sectors[s].surface = 2; cycles = 2; for (int x = 0; x < buffer_width; ++x) { sectors[s].surf[x] = 0; } }
		else								{ sectors[s].surface = 0; cycles = 1; }

		for (int frontBack = 0; frontBack < cycles; ++frontBack)
		{
			for (int w = sectors[s].ws; w < sectors[s].we; ++w) // Loop through walls
			{
				// Offset by player.
				int x1 = walls[w].x1 - player.x;
				int x2 = walls[w].x2 - player.x;
				int y1 = walls[w].y1 - player.y;
				int y2 = walls[w].y2 - player.y;

				if (frontBack == 1)
				{
					// Reverse draw order.
					int swap = x1;
					x1 = x2;
					x2 = swap;
					swap = y1;
					y1 = y2;
					y2 = swap;
				}

				// World X Position
				wx[0] = x1 * CS - y1 * SN;
				wx[1] = x2 * CS - y2 * SN;
				wx[2] = wx[0]; // top line.
				wx[3] = wx[1];
				// World Y Position (Depth)
				wy[0] = y1 * CS + x1 * SN;
				wy[1] = y2 * CS + x2 * SN;
				wy[2] = wy[0]; // top line.
				wy[3] = wy[1];

				sectors[s].d += distance(0, 0, (wx[0] + wx[1]) / 2, (wy[0] + wy[1]) / 2); // Store wall distance.
				// World Z Position (Height)
				wz[0] = sectors[s].z1 - player.z + ((player.look * wy[0]) / 32.0f);
				wz[1] = sectors[s].z1 - player.z + ((player.look * wy[1]) / 32.0f);
				wz[2] = sectors[s].z2 - player.z + ((player.look * wy[0]) / 32.0f); // top line.
				wz[3] = sectors[s].z2 - player.z + ((player.look * wy[1]) / 32.0f);

				// Prevent drawing wall if behind player
				if (wy[0] < 1 && wy[1] < 1)
					continue;
				// Clip behind player
				if (wy[0] < 1) // First point behind.
				{
					clipBehindPlayer(&wx[0], &wy[0], &wz[0], wx[1], wy[1], wz[1]); // Bottom line.
					clipBehindPlayer(&wx[2], &wy[2], &wz[2], wx[3], wy[3], wz[3]); // Top line.
				}
				if (wy[1] < 1) // Second point behind.
				{
					clipBehindPlayer(&wx[1], &wy[1], &wz[1], wx[0], wy[0], wz[0]); // Bottom line.
					clipBehindPlayer(&wx[3], &wy[3], &wz[3], wx[2], wy[2], wz[2]); // Top line.
				}

				// Calculate Screen Positions
				int halfBufferWidth = buffer_width / 2.0f;
				int halfBufferHeight = buffer_height / 2.0f;

				wx[0] = wx[0] * fov / wy[0] + halfBufferWidth;
				wy[0] = wz[0] * fov / wy[0] + halfBufferHeight;
				wx[1] = wx[1] * fov / wy[1] + halfBufferWidth;
				wy[1] = wz[1] * fov / wy[1] + halfBufferHeight;

				wx[2] = wx[2] * fov / wy[2] + halfBufferWidth;
				wy[2] = wz[2] * fov / wy[2] + halfBufferHeight;
				wx[3] = wx[3] * fov / wy[3] + halfBufferWidth;
				wy[3] = wz[3] * fov / wy[3] + halfBufferHeight;

				// Draw wall in 3D
				RGB c;
				c.rgb = walls[w].c;
				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], s, w, frontBack);
			}

			sectors[s].d /= (sectors[s].we - sectors[s].ws); // Average sector distance.
		}
	}
}
void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int s, int w, int frontBack)
{
	int wt = walls[w].wt; // Get wall texture.

	// Calculate horizontal texture coordinates.
	float ht = 0;
	float ht_step = (float)textures[wt].w*walls[w].u / (float)(x2-x1);

	// 
	int dyb = b2 - b1; // Bottom line y distance
	int dyt = t2 - t1; // Top line y distance
	int dx = x2 - x1; if (dx == 0) { dx = 1; } // x distance
	int xs = x1; // Starting Position

	// Clip X
	if (x1 < 0) { ht -= ht_step * x1; x1 = 0; }
	if (x2 < 0) { x2 = 0; }
	if (x1 > buffer_width) { x1 = buffer_width; }
	if (x2 > buffer_width) { x2 = buffer_width; }

	// Draw vertical lines.
	for (int x = x1; x < x2; ++x)
	{
		int y1 = dyb * (x - xs + 0.5f) / dx + b1; // Bottom point on y axis
		int y2 = dyt * (x - xs + 0.5f) / dx + t1; // Top point on y axis

		// Calculate vertical texture coordinates.
		float vt = 0;
		float vt_step = (float)textures[wt].h*walls[wt].v / (float)(y2 - y1);

		// Clip Y
		if (y1 < 0) { vt -= vt_step * y1; y1 = 0; }
		if (y2 < 0) { y2 = 0; }
		if (y1 > buffer_height) { y1 = buffer_height; }
		if (y2 > buffer_height) { y2 = buffer_height; }

		// Draw front wall
		if (frontBack == 0)
		{
			if (sectors[s].surface == 1) { sectors[s].surf[x] = y1; } // Bottom surface top row
			if (sectors[s].surface == 2) { sectors[s].surf[x] = y2; } // Top Surface top row

			for (int y = y1; y < y2; ++y)
			{
				int textureChannels = 3;
				int r, g, b;

				int xx = ((int)ht % textures[wt].w) * textureChannels;
				int yy = ((int)vt % textures[wt].h) * textureChannels;
				int sample = xx + yy * textures[wt].w;

				float shade = 1 - ((walls[w].shade / 2) * 0.01f);
				r = textures[wt].name[sample] * shade;
				g = textures[wt].name[sample+1] * shade;
				b = textures[wt].name[sample+2] * shade;

				if (r < 0) { r = 0; }
				if (g < 0) { g = 0; }
				if (b < 0) { b = 0; }

				drawPixel(framebuffer[0], x, y, (RGB){ r, g, b });

				vt += vt_step;
			}
			ht += ht_step;
		}
		// Draw back wall and surfaces
		if (frontBack == 1)
		{
			int xo = buffer_width / 2;
			int yo = buffer_height / 2;
			int x2 = x - xo;
			int wo;

			float tile = sectors[s].ss * 3;

			if (sectors[s].surface == 1) { y2 = sectors[s].surf[x]; wo = sectors[s].z1; }
			if (sectors[s].surface == 2) { y1 = sectors[s].surf[x]; wo = sectors[s].z2; }

			float lookUpDown = -player.look * (M_PI * 2);
			if (lookUpDown > buffer_height) { lookUpDown = buffer_height; }

			float moveUpDown = (float)(player.z - wo) / (float)yo;
			if (moveUpDown == 0) { moveUpDown == 0.001f; }

			int ys = y1-yo;
			int ye = y2-yo;

			for (int y = ys; y < ye; ++y)
			{
				float z = y + lookUpDown;
				if (z == 0) { z = 0.0001f; }

				float fx = x2 / z * moveUpDown * tile;
				float fy = fov / z * moveUpDown * tile;

				float rx = fx * math.sin[player.angle] - fy * math.cos[player.angle] + (player.y / 60 * tile);
				float ry = fx * math.cos[player.angle] + fy * math.sin[player.angle] - (player.x / 60 * tile);

				if (rx < 0) { rx = -rx + 1; }
				if (ry < 0) { ry = -ry + 1; }

				int st = sectors[s].st;

				int textureChannels = 3;
				int r, g, b;

				int xx = ((int)rx % textures[st].w) * textureChannels;
				int yy = ((int)ry % textures[st].h) * textureChannels;
				int sample = xx + yy * textures[wt].w;

				r = textures[wt].name[sample];
				g = textures[wt].name[sample + 1];
				b = textures[wt].name[sample + 2];

				drawPixel(framebuffer[0], x2+xo, y+yo, (RGB){ r, g, b });
			}
		}
	}
}
void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2)
{
	float da = *y1;
	float db = y2;
	float d = da - db; if (d == 0) { d = 1; }
	float s = da / (da-db);
	*x1 = *x1 + s * (x2 - (*x1));
	*y1 = *y1 + s * (y2 - (*y1)); if (*y1 == 0) { *y1 = 1; }
	*z1 = *z1 + s * (z2 - (*z1));
}
int distance(int x1, int y1, int x2, int y2)
{
	int dist = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return dist;
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		loadScene();

	// Player Input.
	switch (action)
	{
		case GLFW_PRESS:
		{
			if (key == GLFW_KEY_W)
				playerInput.w = true;
			if (key == GLFW_KEY_A)
				playerInput.a = true;
			if (key == GLFW_KEY_S)
				playerInput.s = true;
			if (key == GLFW_KEY_D)
				playerInput.d = true;
			if (key == GLFW_KEY_COMMA)
				playerInput.sl = true;
			if (key == GLFW_KEY_PERIOD)
				playerInput.sr = true;
			if (key == GLFW_KEY_M)
				playerInput.m = true;
		} break;
		case GLFW_RELEASE:
		{
			if (key == GLFW_KEY_W)
				playerInput.w = false;
			if (key == GLFW_KEY_A)
				playerInput.a = false;
			if (key == GLFW_KEY_S)
				playerInput.s = false;
			if (key == GLFW_KEY_D)
				playerInput.d = false;
			if (key == GLFW_KEY_COMMA)
				playerInput.sl = false;
			if (key == GLFW_KEY_PERIOD)
				playerInput.sr = false;
			if (key == GLFW_KEY_M)
				playerInput.m = false;
		} break;
		default:
		{
		} return;
	}
}
