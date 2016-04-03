#include "stdafx.h"
#include "Graphics2D.h"

#pragma comment (lib, "d2d1.lib")

using namespace zen;

Graphics2D* Graphics2D::gInstance = nullptr;

void Graphics2D::initialize()
{
	ZEN_ASSERT(!gInstance, L"Graphics2D is already initialized.");
	gInstance = new Graphics2D;
}

void Graphics2D::finalize()
{
	SAFE_DELETE(gInstance);
}

Graphics2D::Graphics2D()
{
	CHECK_HRESULT(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mpFactory));
}

Graphics2D::~Graphics2D()
{
	SAFE_RELEASE(mpFactory);
}

