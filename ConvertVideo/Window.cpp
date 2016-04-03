#include "stdafx.h"
#include "Window.h"

using namespace zen;

wstring Window::gClassName = L"zen::WindowClass";

Window::DESC Window::DEFAULT_DESC =
{
	L"New Window",
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	nullptr,
	nullptr,
};

Window::Window(const DESC &desc)
{
	create(desc);
}

Window::~Window()
{
}

void Window::initialize()
{
	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = gClassName.c_str();

	ZEN_ASSERT(RegisterClassEx(&wc), L"RegisterClass is failed.");
}

void Window::create(const DESC &desc)
{
	mHandle = CreateWindow(
		gClassName.c_str(),
		desc.title.c_str(),
		desc.style,
		desc.x,
		desc.y,
		desc.width,
		desc.height,
		desc.hWndParent,
		desc.hMenu,
		GetModuleHandle(nullptr),
		nullptr);
	
	ZEN_ASSERT(mHandle, L"CreateWindow is failed.");

	ShowWindow(mHandle, SW_SHOWDEFAULT);
	UpdateWindow(mHandle);

	SetProp(mHandle, L"thisPtr", (HANDLE)this);
}

#define HANDLE_MESSAGE(msg, fn)\
case msg:\
	thisPtr->fn();\
	return S_OK

LRESULT CALLBACK Window::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto* thisPtr = static_cast<Window*>(GetProp(hWnd, L"thisPtr"));

	if (!thisPtr)
		goto RETURN;

	switch (message) {

		HANDLE_MESSAGE(WM_CLOSE, onClose);
		HANDLE_MESSAGE(WM_DESTROY, onDestroy);

	default:
		break;
	}

RETURN:
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void Window::onClose()
{
	DestroyWindow(mHandle);
}