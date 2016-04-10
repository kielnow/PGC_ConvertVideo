#pragma once

#include <wincodec.h>
#include <wincodecsdk.h>

namespace zen
{

	class GraphicsWIC : public Singleton<GraphicsWIC>
	{
	public:
		ZEN_DECLARE_SINGLETON(GraphicsWIC);

		IWICImagingFactory* getFactory() const { return mpFactory; }

	protected:
		GraphicsWIC();

		~GraphicsWIC();

		IWICImagingFactory* mpFactory;
	};




	class BitmapWIC
	{
	public:
		BitmapWIC(u32 w, u32 h);

		virtual ~BitmapWIC();

		u8* map();

		void unmap();

		u32 getWidth() const { return mWidth; }

		u32 getHeight() const { return mHeight; }

		u32 getStride() const { return mStride; }

		u32 getDataSize() const { return mDataSize; }

		u32 getDepth() const { return mDepth; }

		IWICBitmap* getHandle() const { return mpHandle; }

	protected:
		IWICBitmap* mpHandle;
		IWICBitmapLock* mpLock;
		u32 mWidth;
		u32 mHeight;
		u32 mStride;
		u32 mDataSize;
		u32 mDepth;
	};

}

#define IGraphicsWIC		(zen::GraphicsWIC::getInstance())
