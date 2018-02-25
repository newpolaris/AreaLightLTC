#ifdef _WIN32
  #include <windows.h>
#endif

// OpenGL & GLEW (GL Extension Wrangler)
#include <GL/glew.h>
// GLFW
#include <glfw3.h>

// GLM for matrix transformation
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtx/quaternion.hpp>

// GLSL Wrangler
#include <glsw/glsw.h>

// Standard libraries
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// imgui
#include <tools/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <tools/TCamera.hpp>
#include <tools/Timer.hpp>
#include <tools/Logger.hpp>
#include <tools/gltools.hpp>
#include <tools/SimpleProfile.h>

#include <GLType/ProgramShader.h>
#include <GLType/BaseTexture.h>
#include <GLType/Framebuffer.h>
#include <SkyBox.h>
#include <Mesh.h>

#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>

namespace light
{
    SphereMesh m_sphereMesh(16);
    PlaneMesh m_areaMesh(1.0);
    ProgramShader m_shaderFlat;

    void initialize()
    {
        m_shaderFlat.initalize();
        m_shaderFlat.addShader(GL_VERTEX_SHADER, "Flat.Vertex");
        m_shaderFlat.addShader(GL_FRAGMENT_SHADER, "Flat.Fragment");
        m_shaderFlat.link();

        m_areaMesh.init();
        m_sphereMesh.init();
    }

    void shutdown()
    {
        m_areaMesh.destroy();
        m_sphereMesh.destroy();
    }

    struct LightBlock
    {
        float enabled;
        float type; // 0 = pointlight, 1 = arealight
        float pad0[2];
        glm::vec4 ambient;
        glm::vec4 position; // where are we
        glm::vec4 diffuse; // how diffuse
        glm::vec4 specular; // what kinda specular stuff we got going on?
        // attenuation
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        float pad1;
        // spot and area
        glm::vec3 spotDirection;
        float pad2;
        // only for area
        float width;
        float height;
        float pad3[2];
        glm::vec3 right;
        float pad4;
        glm::vec3 up;
        float pad5;
    };

    class Light
    {
    public:

        Light();

        void draw(const TCamera& camera);
        void update(const TCamera& camera, GLuint bufferID);

        const glm::vec3& getPosition() noexcept;
        void setPosition(const glm::vec3& position) noexcept;
        const glm::quat& getRotation() noexcept;
        void setRotation(const glm::quat& quaternion) noexcept;
        float getType() noexcept;
        void setType(float type) noexcept;
        void setAttenuation(const glm::vec3& attenuation) noexcept;
        const glm::vec3& getAttenuation() noexcept;

        glm::vec3 m_position;
        glm::vec3 m_attenuation;
        glm::quat m_rotation;
        float m_type;
        float m_width;
        float m_height;
    };

    Light::Light() :
        m_position(glm::vec3(0.f)),
        m_rotation(1, 0, 0, 0),
        m_attenuation(glm::vec3(1.f, 0.f, 0.f)),
        m_type(0.f),
        m_width(5.f),
        m_height(5.f)
    {
    }

    void Light::draw(const TCamera& camera)
    {
        glm::mat4 projection = camera.getProjectionMatrix();
        glm::mat4 view = camera.getViewMatrix();

        glDisable(GL_CULL_FACE);

        glm::mat4 model = glm::mat4(1.f);
        model = glm::translate(model, glm::vec3(m_position));
        model = model * glm::toMat4(m_rotation);

        if (m_type == 0.f)
            model = glm::scale(model, glm::vec3(1.f));
        else if (m_type == 1.f)
            model = glm::scale(model, glm::vec3(m_width, 1, m_height));

        m_shaderFlat.bind();
        m_shaderFlat.setUniform("projection", projection);
        m_shaderFlat.setUniform("view", view);
        m_shaderFlat.setUniform("model", model);
        m_shaderFlat.setUniform("color", glm::vec3(1.f));

        if (m_type == 0.f)
            m_sphereMesh.draw();
        else if (m_type == 1.f)
            m_areaMesh.draw();

        glEnable(GL_CULL_FACE);
    }

    void Light::update(const TCamera& camera, GLuint bufferID)
    {
        const auto makeYaxisFoward = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));
        auto rotation = glm::toMat3(m_rotation * makeYaxisFoward);
        auto lightEyePosition = glm::vec4(m_position, 1.0f);

        LightBlock light = { 0, };
        light.enabled = true;
        light.type = m_type;
        light.position = lightEyePosition;
        light.ambient = glm::vec4(glm::vec3(0.1f), 1.0f);
        light.diffuse = glm::vec4(1.0f);
        light.specular = glm::vec4(1.0f);
        light.constantAttenuation = m_attenuation.x;
        light.linearAttenuation = m_attenuation.y;
        light.quadraticAttenuation = m_attenuation.z;
        light.width = m_width;
        light.height = m_height;
        light.right = rotation[0];
        light.up = rotation[1];
        light.spotDirection = rotation[2];
        glNamedBufferSubData(bufferID, 0, sizeof(light), &light);
    }

    const glm::vec3& Light::getPosition() noexcept
    {
        return m_position;
    }

    void Light::setPosition(const glm::vec3& position) noexcept
    {
        m_position = position;
    }

    const glm::quat& Light::getRotation() noexcept
    {
        return m_rotation;
    }

    void Light::setRotation(const glm::quat& quaternion) noexcept
    {
        m_rotation = glm::toMat4(quaternion);
    }

    float Light::getType() noexcept
    {
        return m_type;
    }

    void Light::setType(float type) noexcept
    {
        m_type = type;
    }

    void Light::setAttenuation(const glm::vec3& attenuation) noexcept
    {
        m_attenuation = attenuation;
    }

    const glm::vec3& Light::getAttenuation() noexcept
    {
        return m_attenuation;
    }
}

namespace
{
    const uint32_t SHADOW_WIDTH = 1024;
    const uint32_t WINDOW_WIDTH = 1280;
    const uint32_t WINDOW_HEIGHT = 720;
	const char* WINDOW_NAME = "Point Shadow";

	bool bCloseApp = false;
	GLFWwindow* window = nullptr;  
    CubeMesh m_cube;
    PlaneMesh m_plane(500.f);
    light::Light m_light;
    ProgramShader m_shader;

    GLuint g_lightUniformBuffer = GL_NONE;
    const int g_lightBlockPoint = 0;

    //?

    TCamera m_camera;

    bool bWireframe = false;

    //?

	void initialize(int argc, char** argv);
	void initExtension();
	void initGL();
	void initWindow(int argc, char** argv);
    void finalizeApp();
	void mainLoopApp();
    void moveCamera( int key, bool isPressed );
	void prepareRender();
    void render();
	void renderHUD();
	void update();
	void updateHUD();

	void glfw_error_callback(int error, const char* description);
    void glfw_keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void glfw_reshape_callback(GLFWwindow* window, int width, int height);
    void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void glfw_motion_callback(GLFWwindow* window, double xpos, double ypos);
	void glfw_char_callback(GLFWwindow* windows, unsigned int c);
	void glfw_scroll_callback(GLFWwindow* windows, double xoffset, double yoffset);
}

// Breakpoints that should ALWAYS trigger (EVEN IN RELEASE BUILDS) [x86]!
#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else 
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP)
// __builtin_trap() 
#endif

void APIENTRY OpenglCallbackFunction(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    using namespace std;

    // ignore these non-significant error codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 131184) 
        return;

    cout << "---------------------opengl-callback-start------------" << endl;
    cout << "message: " << message << endl;
    cout << "type: ";
    switch(type) {
    case GL_DEBUG_TYPE_ERROR:
        cout << "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        cout << "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        cout << "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        cout << "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        cout << "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        cout << "OTHER";
        break;
    }
    cout << endl;

    cout << "id: " << id << endl;
    cout << "severity: ";
    switch(severity){
    case GL_DEBUG_SEVERITY_LOW:
        cout << "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        cout << "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        cout << "HIGH";
        break;
    }
    cout << endl;
    cout << "---------------------opengl-callback-end--------------" << endl;
    if (type == GL_DEBUG_TYPE_ERROR)
        DEBUG_BREAK;
}

namespace {
	using namespace std;

	void initialize(int argc, char** argv)
	{
		// window maanger
		initWindow(argc, argv);

		// OpenGL extensions
		initExtension();

		// OpenGL
		initGL();

		// ImGui initialize without callbacks
		ImGui_ImplGlfwGL3_Init(window, false);

		// Load FontsR
		// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
		// ImGuiIO& io = ImGui::GetIO();
		// io.Fonts->AddFontDefault();
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);

		// GLSW : shader file manager
		glswInit();
		glswSetPath("./shaders/", ".glsl");
	#ifdef __APPLE__
		glswAddDirectiveToken("*", "#version 330 core");
	#else
		glswAddDirectiveToken("*", "#version 430 core");
	#endif

        Timer::getInstance().start();

        // App Objects
        m_camera.setViewParams( glm::vec3( 0.0f, 0.0f, 3.0f), glm::vec3( 0.0f, 0.0f, 0.0f) );
        m_camera.setMoveCoefficient(0.35f);

		// to prevent osx input bug
		fflush(stdout);

		prepareRender();
	}

	void initExtension()
	{
        glewExperimental = GL_TRUE;

        GLenum result = glewInit(); 
        if (result != GLEW_OK)
        {
            fprintf( stderr, "Error: %s\n", glewGetErrorString(result));
            exit( EXIT_FAILURE );
        }

        fprintf( stderr, "GLEW version : %s\n", glewGetString(GLEW_VERSION));

	#ifndef __APPLE__
        assert(GLEW_ARB_direct_state_access);
	#endif
	}

	void initGL()
	{
        // Clear the error buffer (it starts with an error).
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL error: " << err << std::endl;
		}

        std::printf("%s\n%s\n", 
			glGetString(GL_RENDERER),  // e.g. Intel HD Graphics 3000 OpenGL Engine
			glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
        );

    #if _DEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        if (glDebugMessageCallback) {
            cout << "Register OpenGL debug callback " << endl;
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            glDebugMessageCallback(OpenglCallbackFunction, nullptr);
        }
        else
            cout << "glDebugMessageCallback not available" << endl;
    #endif

        glClearColor( 0.15f, 0.15f, 0.15f, 0.0f);

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );

        glDisable( GL_STENCIL_TEST );
        glClearStencil( 0 );

        glEnable(GL_CULL_FACE);
        glCullFace( GL_BACK );    
        glFrontFace(GL_CCW);

        glDisable( GL_MULTISAMPLE );
	}

	void initWindow(int argc, char** argv)
	{
		glfwSetErrorCallback(glfw_error_callback);

		// Initialise GLFW
		if( !glfwInit() )
		{
			fprintf( stderr, "Failed to initialize GLFW\n" );
			exit( EXIT_FAILURE );
		}
        
        glfwWindowHint(GLFW_SAMPLES, 4);
    #ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    #endif
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	#endif
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL );
		if ( window == NULL ) {
			fprintf( stderr, "Failed to open GLFW window\n" );
			glfwTerminate();
			exit( EXIT_FAILURE );
		}
		glfwMakeContextCurrent(window);
		glfw_reshape_callback( window, WINDOW_WIDTH, WINDOW_HEIGHT );

		// GLFW Events' Callback
		glfwSetWindowSizeCallback( window, glfw_reshape_callback );
		glfwSetKeyCallback( window, glfw_keyboard_callback );
		glfwSetMouseButtonCallback( window, glfw_mouse_callback );
		glfwSetCursorPosCallback( window, glfw_motion_callback );
		glfwSetCharCallback( window, glfw_char_callback );
		glfwSetScrollCallback( window, glfw_scroll_callback );
	}

	void finalizeApp()
	{
        light::shutdown();
        glswShutdown();  
		m_cube.destroy();
        m_plane.destroy();

        Logger::getInstance().close();
		ImGui_ImplGlfwGL3_Shutdown();
		glfwTerminate();
	}

	void mainLoopApp()
	{
		do {
			update();
            updateHUD();
			render();
            renderHUD();

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();
		}
		while (!bCloseApp && glfwWindowShouldClose(window) == 0);
	}

    // GLFW Callbacks_________________________________________________  
	void glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "Error: %s\n", description);
	}

    void glfw_reshape_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);

        float aspectRatio = ((float)width) / ((float)height);
        m_camera.setProjectionParams( 45.0f, aspectRatio, 0.1f, 100.0f);
    }

	void update()
	{
        Timer::getInstance().update();
        m_camera.update();

        // move light position over time
        auto pos = m_light.getPosition();
        pos.z = sin(static_cast<float>(glfwGetTime()) * 0.5f) * 3.0f;
        m_light.setPosition(pos);

        m_light.update(m_camera, g_lightUniformBuffer);
	}

	void updateHUD()
	{
		ImGui_ImplGlfwGL3_NewFrame();
	}

    void render()
    {    
        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 0);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );    
        glPolygonMode(GL_FRONT_AND_BACK, (bWireframe) ? GL_LINE : GL_FILL);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = m_camera.getProjectionMatrix();
        glm::mat4 view = m_camera.getViewMatrix();
        glm::vec4 mat_ambient = glm::vec4(glm::vec3(0.1f), 1.f);
        glm::vec4 mat_diffuse = glm::vec4(glm::vec3(0.8f), 1.f);
        glm::vec4 mat_specular = glm::vec4(glm::vec3(0.8f), 1.f);;
        glm::vec4 mat_emissive = glm::vec4(0.0f);
        float mat_shininess = 10.0;

        // Bind the buffer object to the uniform block
        glBindBufferBase(GL_UNIFORM_BUFFER, g_lightBlockPoint, g_lightUniformBuffer);

        m_shader.bind();
        m_shader.setUniform("uCameraPos", m_camera.getPosition());
        m_shader.setUniform("projection", projection);
        m_shader.setUniform("view", view);
        // set lighting uniforms
        m_shader.setUniform("mat_ambient", mat_ambient);
        m_shader.setUniform("mat_diffuse", mat_diffuse);
        m_shader.setUniform("mat_specular", mat_specular);
        m_shader.setUniform("mat_emissive", mat_emissive);
        m_shader.setUniform("mat_shininess", mat_shininess);

        m_shader.setUniform("mat_ambient", mat_ambient);
        m_shader.setUniform("mat_diffuse", mat_diffuse);
        m_shader.setUniform("mat_specular", mat_specular);

        // plane
        {
            glm::mat4 model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(3.0f, -3.5f, 0.0));
            m_shader.setUniform("model", model);
            m_plane.draw();
        }
        // cubes
        {
            glm::mat4 model = glm::mat4(1.f);
            model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
            model = glm::scale(model, glm::vec3(0.5f));
            m_shader.setUniform("model", model);
            m_cube.draw();
            model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
            model = glm::scale(model, glm::vec3(0.75f));
            m_shader.setUniform("model", model);
            m_cube.draw();
            model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
            model = glm::scale(model, glm::vec3(0.5f));
            m_shader.setUniform("model", model);
            m_cube.draw();
            model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
            model = glm::scale(model, glm::vec3(0.5f));
            m_shader.setUniform("model", model);
            m_cube.draw();
            model = glm::mat4(1.f);
            model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
            model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            model = glm::scale(model, glm::vec3(0.75f));
            m_shader.setUniform("model", model);
            m_cube.draw();
        }
        m_light.draw(m_camera);
    }

	void renderHUD()
	{
		// restore some state
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        ImGui::Render();
	}

	void prepareRender()
    {
        light::initialize();

        // rotate toward ground and tilt slightly
        auto rot = glm::angleAxis(glm::pi<float>(), glm::vec3(1, 0, 0));
        rot = glm::angleAxis(glm::pi<float>()*0.25f, glm::vec3(0, 0, 1)) * rot;

        m_light.setType(1.0);
        m_light.setPosition(glm::vec3(0, -1, 2));
        m_light.setRotation(rot);
        m_light.setAttenuation(glm::vec3(0.8f, 1e-2f, 1e-1f));

        m_shader.initalize();
        m_shader.addShader(GL_VERTEX_SHADER, "AreaShadow.Vertex");
        m_shader.addShader(GL_FRAGMENT_SHADER, "AreaShadow.Fragment");
        m_shader.link();

		m_cube.init();
        m_plane.init();

        GLuint lightBlock = glGetUniformBlockIndex(m_shader.getShaderID(), "Light");
        glUniformBlockBinding(m_shader.getShaderID(), lightBlock, g_lightBlockPoint);

        //Setup our Uniform Buffers
        glCreateBuffers(1, &g_lightUniformBuffer);
        glNamedBufferStorage(g_lightUniformBuffer, sizeof(light::LightBlock), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    void glfw_keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) 
	{
		ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);

		moveCamera( key, action != GLFW_RELEASE);
		bool bPressed = action == GLFW_PRESS;
		if (bPressed) {
			switch (key)
			{
				case GLFW_KEY_ESCAPE:
					bCloseApp = true;

				case GLFW_KEY_W:
					bWireframe = !bWireframe;
					break;

				case GLFW_KEY_T:
					{
						Timer &timer = Timer::getInstance();
						printf( "fps : %d [%.3f ms]\n", timer.getFPS(), timer.getElapsedTime());
					}
					break;

				case GLFW_KEY_U:
                    // lightPos.y += 0.1f;
					break;

				case GLFW_KEY_D:
                    // lightPos.y -= 0.1f;
					break;

				default:
					break;
			}
		}
    }

    void moveCamera( int key, bool isPressed )
    {
        switch (key)
        {
		case GLFW_KEY_UP:
			m_camera.keyboardHandler( MOVE_FORWARD, isPressed);
			break;

		case GLFW_KEY_DOWN:
			m_camera.keyboardHandler( MOVE_BACKWARD, isPressed);
			break;

		case GLFW_KEY_LEFT:
			m_camera.keyboardHandler( MOVE_LEFT, isPressed);
			break;

		case GLFW_KEY_RIGHT:
			m_camera.keyboardHandler( MOVE_RIGHT, isPressed);
			break;
        }
    }

    void glfw_motion_callback(GLFWwindow* window, double xpos, double ypos)
    {
		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		const bool bPressed = (state == GLFW_PRESS);
		const bool mouseOverGui = ImGui::MouseOverArea();
		if (!mouseOverGui && bPressed) m_camera.motionHandler( int(xpos), int(ypos), false);    
    }  

    void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods)
    {
        ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { 
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			const bool mouseOverGui = ImGui::MouseOverArea();
			if (!mouseOverGui)
				m_camera.motionHandler( int(xpos), int(ypos), true); 
		}    
    }


	void glfw_char_callback(GLFWwindow* windows, unsigned int c)
	{
		ImGui_ImplGlfwGL3_CharCallback(windows, c);
	}

	void glfw_scroll_callback(GLFWwindow* windows, double xoffset, double yoffset)
	{
		ImGui_ImplGlfwGL3_ScrollCallback(windows, xoffset, yoffset);
	}
}

int main(int argc, char** argv)
{
	initialize(argc, argv);
	mainLoopApp();
	finalizeApp();
    return EXIT_SUCCESS;
}
