#pragma once

#include <wincodecsdk.h>

#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <mfreadwrite.h>

#pragma comment (lib, "mf.lib")
#pragma comment (lib, "mfplat.lib")
#pragma comment (lib, "mfuuid.lib")
#pragma comment (lib, "mfreadwrite.lib")

#include <propvarutil.h>

#pragma comment (lib, "propsys.lib")

namespace zen
{

	class MoviePlayer
	{
	public:
		struct VideoInfo
		{
			u32		width;
			u32		height;
			bool	topDown;
			RECT	rect;
			s64		duration;
			bool	seekable;
			f32		frameRate;
			bool	fixedSizeSamples;
			u32		sampleSize;

			VideoInfo()
				: width(0), height(0), topDown(false), duration(0), seekable(false)
			{
				SetRectEmpty(&rect);
			}
		};

		static void initialize();

		static void finalize();

		MoviePlayer();

		virtual ~MoviePlayer();

		bool open(ZEN_CWSTR path);

		bool setCurrentPosition(s64 pos);

		ID2D1Bitmap* createBitmap(ID2D1RenderTarget* pRT, s64 pos);

		u32 getWidth() const { return mVideoInfo.width; }

		u32 getHeight() const { return mVideoInfo.height; }

		s64 getDuration() const { return mVideoInfo.duration; }

		bool isSeekable() const { return mVideoInfo.seekable; }

		f32 getFramRate() const { return mVideoInfo.frameRate; }

		BitmapWIC* getBitmapWIC() const { return mpBitmapWIC; }

		void setDestSize(u32 width, u32 height);

		void setThreshold(u8 min, u8 max) { mThresholdMin = min; mThresholdMax = max; }

	protected:
		bool selectVideoStream();
		bool getVideoInfo(VideoInfo& dst);

		void createBitmapRT();

		IMFSourceReader* mpReader;
		VideoInfo mVideoInfo;
		s64 mCurrentPosition;

		u32 mDestWidth;
		u32 mDestHeight;
		u8 mThresholdMin;
		u8 mThresholdMax;

		BitmapWIC* mpBitmapWIC;
		ComPtr<ID2D1RenderTarget> mpBitmapRT;
	};

}