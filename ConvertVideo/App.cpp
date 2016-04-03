#include "stdafx.h"
#include "App.h"
#include "Window.h"

using namespace zen;

class MainWindow : public Window
{
public:
	MainWindow(const DESC &desc = DEFAULT_DESC) : Window(desc) {}

protected:
	virtual void onDestroy() override
	{
		PostQuitMessage(0);
	}
};




HINSTANCE App::gHandle = nullptr;

App::App(const DESC &desc)
{
	gHandle = desc.hInstance;
}

App::~App()
{
}

s32 App::execute()
{
	Window::initialize();

	initialize();

	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		update();
	}

	return static_cast<s32>(msg.wParam);
}

void App::initialize()
{
	Window::DESC desc = Window::DEFAULT_DESC;

	desc.title = L"Main WIndow";
	desc.width = 1280;
	desc.height = 720;

	mpMainWindow = new MainWindow(desc);
}

void App::update()
{
}