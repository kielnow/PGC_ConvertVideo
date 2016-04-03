#include "stdafx.h"
#include "App.h"
#include "Window.h"

using namespace zen;

App* App::gInstance = nullptr;

HINSTANCE App::gHandle = nullptr;

App::App()
	: mpMainWindow(nullptr)
{
	ZEN_ASSERT(!gInstance, L"App is already created.");
	gInstance = this;
}

App::~App()
{
	gInstance = nullptr;
}

s32 App::execute(const DESC &desc)
{
	gHandle = desc.hInstance;

	Graphics2D::initialize();
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

	finalize();
	Graphics2D::finalize();

	return static_cast<s32>(msg.wParam);
}

void App::initialize()
{
	HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

	Window::DESC desc = Window::DEFAULT_DESC;

	desc.title = L"Main WIndow";
	desc.width = 1280;
	desc.height = 720;
	desc.isMain = true;

	mpMainWindow = new Window(desc);
}

void App::finalize()
{
	SAFE_DELETE(mpMainWindow);
}

void App::update()
{
}

void App::draw(ID2D1RenderTarget* pRT)
{
	pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
}
