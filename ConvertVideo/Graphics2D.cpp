#include "stdafx.h"
#include "Graphics2D.h"

using namespace zen;

ZEN_IMPLEMENT_SINGLETON(Graphics2D);

Graphics2D::Graphics2D()
{
	CHECK_HRESULT(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mpFactory));
}

Graphics2D::~Graphics2D()
{
	SAFE_RELEASE(mpFactory);
}
