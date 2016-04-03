#pragma once

namespace zen
{

	typedef HWND	WindowHandle;

	class Window
	{
	public:
		struct DESC {
			wstring		title;
			u32			style;
			s32			x;
			s32			y;
			s32			width;
			s32			height;
			HWND		hWndParent;
			HMENU		hMenu;
		};

		static DESC DEFAULT_DESC;

		Window(const DESC &desc = DEFAULT_DESC);

		virtual ~Window();

		static void initialize();

	protected:
		static wstring gClassName;

		static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void create(const DESC &desc);

		virtual void onClose();

		virtual void onDestroy() {}

		HWND mHandle;
	};

}