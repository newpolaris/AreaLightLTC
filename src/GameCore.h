#pragma once

#include <tools/Rtti.h>
#include <string>

namespace gamecore
{
	class IGameApp : public rtti::Interface
	{
	__DeclareSubInterface(IGameApp, rtti::Interface)
	public:
		IGameApp() noexcept;
		virtual ~IGameApp() noexcept;

		virtual void startup() = 0;
		virtual void closeup() = 0;

		virtual void update() noexcept;
		virtual void updateHUD() noexcept;
		virtual void render() noexcept;
		virtual void renderHUD() noexcept;

		virtual bool isDone() const noexcept;
		virtual bool isWireframe() const noexcept;

		uint32_t GetWindowWidth() const noexcept;
		uint32_t GetWindowHeight() const noexcept;

		virtual void charCallback(uint32_t c) noexcept;
		virtual void keyboardCallback(uint32_t c, bool bPressed) noexcept;
		virtual void reshapeCallback(uint32_t width, uint32_t height) noexcept;
		virtual void motionCallback(float xpos, float ypos, bool bPressed) noexcept;
		virtual void mouseCallback(float xpos, float ypos, bool bPressed) noexcept;
	};

	bool updateApplication(IGameApp& app);
	void terminateApplication(IGameApp& app);
	void runApplication(IGameApp& app, std::string name);
}

#define CREATE_APPLICATION(app_class) \
	int main()\
	{\
		app_class app;\
		gamecore::runApplication(app, #app_class);\
		return 0;\
	}
