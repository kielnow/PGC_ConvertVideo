#pragma once

namespace zen
{

	typedef HWND	WindowHandle;

	class Window
	{
	public:
		static void initialize();

		struct DESC {
			wstring		title;
			u32			style;
			s32			x;
			s32			y;
			s32			width;
			s32			height;
			HWND		hWndParent;
			HMENU		hMenu;

			bool		isMain;
		};

		static DESC DEFAULT_DESC;

		Window(const DESC &desc = DEFAULT_DESC);

		virtual ~Window();

		virtual void draw();

		HWND getHandle() const { return mHandle; }

		ID2D1RenderTarget* getRenderTarget() const { return mpRenderTarget; }

		void redraw();

		void setSize(u32 width, u32 height);

	protected:
		static wstring gClassName;

		static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void create(const DESC &desc);

		virtual bool onCreate(LPCREATESTRUCT lpCreateStruct);

		virtual void onClose();

		virtual void onDestroy();

		virtual void onPaint();

		virtual void onSize(u32 state, u32 width, u32 height);

		HWND mHandle;
		bool mIsMain;
		ID2D1HwndRenderTarget* mpRenderTarget;
	};

}