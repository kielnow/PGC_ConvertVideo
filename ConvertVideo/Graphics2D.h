#pragma once

#include <d2d1.h>
#include <d2d1helper.h>
#include <d2dbasetypes.h>
#include <D2DErr.h>

#pragma comment(lib, "d2d1.lib")

namespace zen
{

	class Graphics2D : public Singleton<Graphics2D>
	{
	public:
		ZEN_DECLARE_SINGLETON(Graphics2D);

		ID2D1Factory* getFactory() const { return mpFactory; }

	protected:
		Graphics2D();

		~Graphics2D();

		ID2D1Factory* mpFactory;
	};




#if 0
	class BitmapWIC;
	
	class Bitmap2D
	{
	public:
		Bitmap2D(ID2D1RenderTarget* pRT, u32 w, u32 h, u8* pData);

		Bitmap2D(ID2D1RenderTarget* pRT, BitmapWIC* pBitmapWIC);

		virtual ~Bitmap2D();

		u32 getWidth() const { return mWidth; }

		u32 getHeight() const { return mHeight; }

	protected:
		ID2D1Bitmap* mpHandle;
		u32 mWidth;
		u32 mHeight;
	};
#endif

}

#define IGraphics2D		(zen::Graphics2D::getInstance())
