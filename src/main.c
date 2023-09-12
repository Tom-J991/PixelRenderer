#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
// TODO: Get Sleep function in other Operating Systems.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cglm/cglm.h>

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

const unsigned int buffer_width = 320;
const unsigned int buffer_height = 240;
const unsigned int buffer_channels = 3;

const double targetFPS = 35.0;

// Structs
typedef struct
{
	union
	{
		struct
		{
			unsigned char b;
			unsigned char g;
			unsigned char r;
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
	int x1, y1; // Bottom line coordinates.
	int x2, y2; //
	int c;		// Wall Color.
} Wall;

typedef struct
{
	int ws, we;				// Wall start and end.
	int z1, z2;				// Height of sector.
	int x, y;				// Sector position.
	int d;					// For sorting drawing order.
	int c1, c2;				// Bottom and top colors.
	int surf[160];			// Surface points. TODO: Set 160 to Buffer Width
	int surface;			// Surface check.
} Sector;

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
unsigned char *imageData;
size_t buffer_size;

unsigned int scale = 2;
unsigned int screen_width;
unsigned int screen_height;

Math math;
PlayerInput playerInput;
Player player;

unsigned int sectorCount = 4;
unsigned int wallCount = 16;
Wall walls[30];
Sector sectors[30];

// Callbacks
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

// Forward Declaration
void start();
void shutdown();

void runGame();
void initGame();
void tick();
void render();
void cleanupGame();

void initOpenGL();
void startOpenGLRender();
void endOpenGLRender();
void cleanupOpenGL();

void drawPixel(unsigned char *imageData, const int x, const int y, const RGB colorData);

void draw3D();
void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, RGB c, int s);
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
	initOpenGL();
	runGame();
}
void shutdown()
{
	cleanupOpenGL();
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

	// Create Image Buffer.
	buffer_size = buffer_width * buffer_height * buffer_channels;
	imageData = (unsigned char *)calloc(buffer_size, sizeof(unsigned char));

	// Create Texture.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer_width, buffer_height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
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
			*ptr = imageData[i];
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
	free(imageData);
	imageData = 0;

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

	// Setup Player.
	player.x = 70;
	player.y = -110;
	player.z = 20;
	player.angle = 0;
	player.look = 0;

	// Setup Scene
	int loadSectors[] =
	{
		// wall start, wall end, z1 height, z2 height, bottom color, top color
		0,  4,  0, 40, DARK_GREEN.rgb,	GREEN.rgb,	// 1
		4,  8,  0, 40, DARK_CYAN.rgb,	CYAN.rgb,	// 2
		8,  12, 0, 40, DARK_BROWN.rgb,	BROWN.rgb,	// 3
		12, 16, 0, 40, DARK_YELLOW.rgb, YELLOW.rgb,	// 4
	};
	int loadWalls[] =
	{
		// x1, y1, x2, y2, color
		0,  0,  32, 0,  YELLOW.rgb,
		32, 0,  32, 32, DARK_YELLOW.rgb,
		32, 32, 0,  32, YELLOW.rgb,
		0,  32, 0,  0,  DARK_YELLOW.rgb,

		64, 0,  96, 0,  GREEN.rgb,
		96, 0,  96, 32, DARK_GREEN.rgb,
		96, 32, 64, 32, GREEN.rgb,
		64, 32, 64, 0,  DARK_GREEN.rgb,

		64, 64, 96, 64, CYAN.rgb,
		96, 64, 96, 96, DARK_CYAN.rgb,
		96, 96, 64, 96, CYAN.rgb,
		64, 96, 64, 64, DARK_CYAN.rgb,

		0,  64, 32, 64, BROWN.rgb,
		32, 64, 32, 96, DARK_BROWN.rgb,
		32, 96, 0,  96, BROWN.rgb,
		0,  96, 0,  64, DARK_BROWN.rgb
	};

	int v1 = 0, v2 = 0;
	for (int s = 0; s < sectorCount; ++s)
	{
		sectors[s].ws = loadSectors[v1+0];
		sectors[s].we = loadSectors[v1+1];
		sectors[s].z1 = loadSectors[v1+2];
		sectors[s].z2 = loadSectors[v1+3]-loadSectors[v1+2];
		sectors[s].c1 = loadSectors[v1+4];
		sectors[s].c2 = loadSectors[v1+5];
		v1 += 6;
		for (int w = sectors[s].ws; w < sectors[s].we; ++w)
		{
			walls[w].x1 = loadWalls[v2+0];
			walls[w].y1 = loadWalls[v2+1];
			walls[w].x2 = loadWalls[v2+2];
			walls[w].y2 = loadWalls[v2+3];
			walls[w].c = loadWalls[v2+4];
			v2 += 5;
		}
	}
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
	for (size_t y = 0; y < buffer_height; ++y)
	{
		for (size_t x = 0; x < buffer_width; ++x)
		{
			drawPixel(imageData, x, y, BACKGROUND);
		}
	}

	draw3D();
}

void drawPixel(unsigned char *imageData, const int x, const int y, const RGB colorData)
{
	if (x > buffer_width-1 || x < 0 || y > buffer_height-1 || y < 0) // Only draw pixel within buffer resolution.
		return;

	int xx = x * buffer_channels;
	int yy = y * buffer_channels;
	int index = xx + yy * buffer_width;

	imageData[index++] = colorData.b;
	imageData[index++] = colorData.g;
	imageData[index++] = colorData.r;
}

void draw3D()
{
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

		if		(player.z < sectors[s].z1)	{ sectors[s].surface = 1; }
		else if (player.z > sectors[s].z2)	{ sectors[s].surface = 2; }
		else								{ sectors[s].surface = 0; }

		for (int loop = 0; loop < 2; ++loop)
		{
			for (int w = sectors[s].ws; w < sectors[s].we; ++w) // Loop through walls
			{
				// Offset by player.
				int x1 = walls[w].x1 - player.x;
				int x2 = walls[w].x2 - player.x;
				int y1 = walls[w].y1 - player.y;
				int y2 = walls[w].y2 - player.y;

				if (loop == 0)
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

				wx[0] = wx[0] * 200 / wy[0] + halfBufferWidth;
				wy[0] = wz[0] * 200 / wy[0] + halfBufferHeight;
				wx[1] = wx[1] * 200 / wy[1] + halfBufferWidth;
				wy[1] = wz[1] * 200 / wy[1] + halfBufferHeight;

				wx[2] = wx[2] * 200 / wy[2] + halfBufferWidth;
				wy[2] = wz[2] * 200 / wy[2] + halfBufferHeight;
				wx[3] = wx[3] * 200 / wy[3] + halfBufferWidth;
				wy[3] = wz[3] * 200 / wy[3] + halfBufferHeight;

				// Draw wall in 3D
				RGB c;
				c.rgb = walls[w].c;
				drawWall(wx[0], wx[1], wy[0], wy[1], wy[2], wy[3], c, s);
			}

			sectors[s].d /= (sectors[s].we - sectors[s].ws); // Average sector distance.
			sectors[s].surface *= -1;
		}
	}
}
void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, RGB c, int s)
{
	int dyb = b2 - b1; // Bottom line y distance
	int dyt = t2 - t1; // Top line y distance
	int dx = x2 - x1; if (dx == 0) { dx = 1; } // x distance
	int xs = x1; // Starting Position

	// Clip X
	if (x1 < 0) { x1 = 0; }
	if (x2 < 0) { x2 = 0; }
	if (x1 > buffer_width-1) { x1 = buffer_width; }
	if (x2 > buffer_width-1) { x2 = buffer_width; }

	// Draw vertical lines.
	for (int x = x1; x < x2; ++x)
	{
		int y1 = dyb * (x - xs + 0.5f) / dx + b1; // Bottom point on y axis
		int y2 = dyt * (x - xs + 0.5f) / dx + t1; // Top point on y axis

		// Clip Y
		if (y1 < 0) { y1 = 0; }
		if (y2 < 0) { y2 = 0; }
		if (y1 > buffer_height-1) { y1 = buffer_height; }
		if (y2 > buffer_height-1) { y2 = buffer_height; }

		// Surface
		if (sectors[s].surface == 1) { sectors[s].surf[x] = y1; continue; } // Save bottom points
		if (sectors[s].surface == 2) { sectors[s].surf[x] = y2; continue; } // Save top points
		if (sectors[s].surface == -1) // Draw bottom
		{ 
			for (int y = sectors[s].surf[x]; y < y1; ++y) 
			{ 
				RGB c;
				c.rgb = sectors[s].c1;
				drawPixel(imageData, x, y, c); 
			} 
		}
		if (sectors[s].surface == -2) // Draw top
		{
			for (int y = y2; y < sectors[s].surf[x]; ++y)
			{
				RGB c;
				c.rgb = sectors[s].c2;
				drawPixel(imageData, x, y, c);
			}
		}

		// Draw normal wall
		for (int y = y1; y < y2; ++y)
		{
			drawPixel(imageData, x, y, c);
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
