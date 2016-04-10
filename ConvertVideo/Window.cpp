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

	false,
};

Window::Window(const DESC &desc)
{
	create(desc);
}

Window::~Window()
{
	SAFE_RELEASE(mpRenderTarget);
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

void Window::redraw()
{
	InvalidateRect(mHandle, nullptr, FALSE);
}

void Window::create(const DESC &desc)
{
	mHandle = CreateWindowEx(
		0,
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

	mIsMain = desc.isMain;

	SetProp(mHandle, L"thisPtr", (HANDLE)this);

	RECT rc = { 0 };
	GetClientRect(mHandle, &rc);

	IGraphics2D->getFactory()->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			mHandle,
			D2D1::SizeU(rc.right, rc.bottom)),
		&mpRenderTarget);

	InvalidateRect(mHandle, nullptr, false);
}

#define HANDLE_MESSAGE(msg, fn)\
case msg:\
	return thisPtr->fn(), S_OK;

LRESULT CALLBACK Window::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto* thisPtr = static_cast<Window*>(GetProp(hWnd, L"thisPtr"));

	if (!thisPtr)
		goto RETURN;

	switch (message) {

		HANDLE_MESSAGE(WM_CLOSE,	onClose);
		HANDLE_MESSAGE(WM_DESTROY,	onDestroy);
		HANDLE_MESSAGE(WM_PAINT,	onPaint);

	case WM_CREATE:
		return thisPtr->onCreate((LPCREATESTRUCT)lParam) ? S_OK : E_FAIL;

	case WM_SIZE:
		return thisPtr->onSize(static_cast<u32>(wParam), static_cast<u32>(LOWORD(lParam)), static_cast<u32>(HIWORD(lParam))), S_OK;

	default:
		break;
	}

RETURN:
	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool Window::onCreate(LPCREATESTRUCT lpCreateStruct)
{
	return true;
}

void Window::onClose()
{
	DestroyWindow(mHandle);
}

void Window::onDestroy()
{
	if (mIsMain)
		PostQuitMessage(0);
}

void Window::onPaint()
{
	PAINTSTRUCT ps;
	BeginPaint(mHandle, &ps);

	if (mpRenderTarget->CheckWindowState() != D2D1_WINDOW_STATE_OCCLUDED)
	{
		mpRenderTarget->BeginDraw();

		mpRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

		draw();

		mpRenderTarget->EndDraw();
	}

	EndPaint(mHandle, &ps);
}

void Window::onSize(u32 state, u32 width, u32 height)
{
	if (mpRenderTarget) {
		mpRenderTarget->Resize(D2D1::SizeU(width, height));
		InvalidateRect(mHandle, nullptr, false);
	}
}

void Window::draw()
{
	if (mIsMain)
		IApp->draw(mpRenderTarget);
}