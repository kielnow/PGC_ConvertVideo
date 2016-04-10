#pragma once

#include "Window.h"

namespace zen {

	class Window;

	class App {
	public:
		static App* getInstance() { return gInstance; }

		static HINSTANCE getHandle() { return gHandle; }

		struct DESC {
			HINSTANCE	hInstance;
			HINSTANCE	hPrevInstance;
			LPWSTR		lpCmdLine;
			int			nCmdShow;
		};

		App();

		virtual ~App();

		s32 execute(const DESC &desc);

		virtual void initialize();

		virtual void finalize();

		virtual void update();

		virtual void draw(ID2D1RenderTarget* pRT);

		Window* getMainWindow() const { return mpMainWindow; }

		ID2D1RenderTarget* getMainRenderTarget() const { return mpMainWindow->getRenderTarget(); }

	protected:
		static App* gInstance;
		static HINSTANCE gHandle;

		Window* mpMainWindow;
	};

}

#define IApp	(zen::App::getInstance())
