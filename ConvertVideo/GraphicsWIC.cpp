#include "stdafx.h"
#include "GraphicsWIC.h"

using namespace zen;

ZEN_IMPLEMENT_SINGLETON(GraphicsWIC);

GraphicsWIC::GraphicsWIC()
	: mpFactory(nullptr)
{
	CHECK_HRESULT(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mpFactory)));
}

GraphicsWIC::~GraphicsWIC()
{
	SAFE_RELEASE(mpFactory);
}




BitmapWIC::BitmapWIC(u32 w, u32 h)
	: mpHandle(nullptr), mpLock(nullptr)
{
	CHECK_HRESULT(IGraphicsWIC->getFactory()->CreateBitmap(w, h, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnDemand, &mpHandle));
	CHECK_HRESULT(mpHandle->Lock(nullptr, WICBitmapLockRead, &mpLock));
	CHECK_HRESULT(mpLock->GetSize(&mWidth, &mHeight));
	CHECK_HRESULT(mpLock->GetStride(&mStride));
	u8* pData = nullptr;
	CHECK_HRESULT(mpLock->GetDataPointer(&mDataSize, &pData));
	SAFE_RELEASE(mpLock);
	mDepth = mStride / mWidth;
}

BitmapWIC::~BitmapWIC()
{
	unmap();
	SAFE_RELEASE(mpHandle);
}

u8* BitmapWIC::map()
{
	if (mpLock)
		return nullptr;

	if (FAILED(mpHandle->Lock(nullptr, WICBitmapLockRead | WICBitmapLockWrite, &mpLock)))
		return nullptr;

	u32 dataSize = 0;
	u8* pData = nullptr;
	if (FAILED(mpLock->GetDataPointer(&dataSize, &pData)))
		return nullptr;

	return pData;
}

void BitmapWIC::unmap()
{
	SAFE_RELEASE(mpLock);
}
