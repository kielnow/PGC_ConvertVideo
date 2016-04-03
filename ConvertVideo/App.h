#pragma once

namespace zen {

	class Window;

	class App {
	public:
		struct DESC {
			HINSTANCE	hInstance;
			HINSTANCE	hPrevInstance;
			LPWSTR		lpCmdLine;
			int			nCmdShow;
		};

		App(const DESC &desc);

		virtual ~App();

		virtual s32 execute();

		virtual void initialize();

		virtual void update();

		static HINSTANCE getHandle() { return gHandle; }

	protected:
		static HINSTANCE gHandle;

		Window* mpMainWindow;
	};

}