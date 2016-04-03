#include "stdafx.h"

#if 1

int APIENTRY wWinMain(
	_In_		HINSTANCE hInstance,
	_In_opt_	HINSTANCE hPrevInstance,
	_In_		LPWSTR    lpCmdLine,
	_In_		int       nCmdShow)
{
	using namespace zen;

	App::DESC desc = {
		hInstance,
		hPrevInstance,
		lpCmdLine,
		nCmdShow,
	};

	auto* app = new App(desc);

	return app->execute();
}

#endif