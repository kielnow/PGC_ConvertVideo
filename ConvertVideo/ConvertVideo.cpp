#include "stdafx.h"
#include "ConvertVideo.h"

#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>

#pragma comment (lib, "mf.lib")
#pragma comment (lib, "mfplat.lib")

using namespace app;

ConvertVideo::ConvertVideo()
{
}

ConvertVideo::~ConvertVideo()
{
}

void ConvertVideo::initialize()
{
	super::initialize();

	CHECK_HRESULT(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
	CHECK_HRESULT(MFStartup(MF_VERSION));
}

void ConvertVideo::finalize()
{
	MFShutdown();
	CoUninitialize();

	super::finalize();
}

void ConvertVideo::update()
{
}

void ConvertVideo::draw(ID2D1RenderTarget* pRT)
{
	pRT->Clear(D2D1::ColorF(D2D1::ColorF::DarkBlue));
}
