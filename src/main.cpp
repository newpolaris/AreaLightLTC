#include <GL/glew.h>
#include <glfw3.h>
#include <string.h>
#include <chrono>
#include <thread>

float rotx, roty = 0;
GLfloat global[] = { 1.0, 0.0, 0.0, 1.0 };
float ambient[4] = { 0.0, 0.0, 0.0, 1.0};
float diffuse[4] = { 0.0, 0.0, 0.0, 1.0 };
float lightpos[4] = {-5, 5, 10, 0};

int main(void)
{
	GLFWwindow* window;

	if (!glfwInit()) {
		return -1;
	}

	if (true)
	{
#ifdef __APPLE__
		GLuint minor = 1;
#else
		GLuint minor = 5;
#endif

		glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	}
	if (true) {
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	} else {
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	}

	if (true) {
		window = glfwCreateWindow(640, 480, "Spinning Cube", NULL, NULL);
		if (window == NULL) {
			fprintf(stderr, "Failed to open GLFW window\n");
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(window);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_LIGHT0);

	glViewport(0, 0, 640, 480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-1, 1, -1, 1, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	while (!glfwWindowShouldClose(window)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(33));

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0, 0, -3);
		glRotatef(rotx, 1, 0, 0);
		glRotatef(roty, 0, 1, 1);

		rotx += 0.5;
		roty += 0.5;

		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);

		glNormal3f(0, 0, 1);
		glVertex3f(-1, -1, 1);
		glVertex3f(1, -1, 1);
		glVertex3f(1, 1, 1);
		glVertex3f(-1, 1, 1);

		glNormal3f(0, 0, -1);
		glVertex3f(-1, -1, -1);
		glVertex3f(-1, 1, -1);
		glVertex3f(1, 1, -1);
		glVertex3f(1, -1, -1);

		glNormal3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		glVertex3f(-1, 1, 1);
		glVertex3f(1, 1, 1);
		glVertex3f(1, 1, -1);

		glNormal3f(0, -1, 0);
		glVertex3f(-1, -1, -1);
		glVertex3f(1, -1, -1);
		glVertex3f(1, -1, 1);
		glVertex3f(-1, -1, 1);

		glNormal3f(1, 0, 0);
		glVertex3f(1, -1, -1);
		glVertex3f(1, 1, -1);
		glVertex3f(1, 1, 1);
		glVertex3f(1, -1, 1);

		glNormal3f(-1, 0, 0);
		glVertex3f(-1, -1, -1);
		glVertex3f(-1, -1, 1);
		glVertex3f(-1, 1, 1);
		glVertex3f(-1, 1, -1);

		glEnd();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
