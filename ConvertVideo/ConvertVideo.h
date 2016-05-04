#pragma once

#include "Compress.h"

namespace zen
{
	class MoviePlayer;
}

namespace app
{

	using namespace zen;

	struct Frame
	{
		u8* data;
		u32 size;
		u32 width;
		u32 height;

		Frame() : data(nullptr)
		{
			clear();
		}

		void clear()
		{
			SAFE_DELETE_ARRAY(data);
			size = width = height = 0;
		}

		void alloc(u32 w, u32 h)
		{
			clear();
			width	= w;
			height	= h;
			size	= w * ((h + 7) >> 3);
			data	= new u8[size];
			memset(data, 0, size);
		}
	};

	struct Data
	{
		// uncompressed full-frame data
		vector<Frame>	frame;

		// delta compression
		vector<u8>		index;
		vector<u8>		page;

		// sparse index compression
		vector<u8>		sindex;
		vector<u8>		dist;

#if 0
		vector<u8>		bwt_index;
		vector<u16>		bwt_n_index;
		vector<u8>		c_bwt_index;
		vector<u16>		c_bwt_n_index;

		vector<u8>		bwt_page;
		vector<u16>		bwt_n_page;
		vector<u8>		c_bwt_page;
		vector<u16>		c_bwt_n_page;
#endif

		// RLE
		vector<u8>		c_index;
		vector<u8>		c_page;
		vector<u8>		c_sindex;
		vector<u8>		c_dist;

		// Switched RLE
		vector<u8>		c2_index;
		vector<u8>		c2_page;

		// RLE/HC
		HCData			hc_index;
		HCData			hc_page;
		HCData			hc_sindex;
		HCData			hc_dist;

		// Adaptive RLE/HC
		HCData2			hc2_index;
		HCData2			hc2_page;
		HCData2			hc2_sindex;
		HCData2			hc2_dist;

		void clear()
		{
			for (auto &it : frame)
				it.clear();

			frame.clear();
			
			index.clear();
			page.clear();
			sindex.clear();
			dist.clear();

			c_index.clear();
			c_page.clear();
			c_sindex.clear();
			c_dist.clear();

			c2_index.clear();
			c2_page.clear();

			hc_index.clear();
			hc_page.clear();
			hc_sindex.clear();
			hc_dist.clear();

			hc2_index.clear();
			hc2_page.clear();
			hc2_sindex.clear();
			hc2_dist.clear();
		}
	};

	class ConvertVideo : public App
	{
	public:
		typedef App super;

		ConvertVideo();

		virtual ~ConvertVideo();

		virtual void initialize() override;

		virtual void finalize() override;

		virtual void update() override;

		virtual void draw(ID2D1RenderTarget* pRT) override;

	protected:
		void clear();

		void open(ZEN_CWSTR path);

		void output(const string &path);

		void play();
		void playDelta();
		void playDeltaRLE();
		void playDeltaSRLE();
		void playDeltaRLEHC();
		void playDeltaRLEHC2();

		ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src);

		ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src, const u8* index);
			
		static bool createFrame(Frame &dst, BitmapWIC* psrc);

		MoviePlayer* 	mpPlayer;
		ID2D1Bitmap* 	mpBitmap;
		BitmapWIC* 		mpBitmapWIC;

		u32 			mFrameRate;
		u32 			mFrameCount;
		u32				mFrameWidth;
		u32				mFrameHeight;
		u32				mFramePageHeight;

		u32 			mMemPos;
		bool 			mIsReady;
		bool			mIsOpen;

		Frame 			mFrame[2];
		u32 			mFramePt;
		Data 			mData;

		u32 			mDecodePt;
		u32 			mDecodeIndexPt;
		u32 			mDecodePagePt;
		Frame 			mDecodeFrame;
		u8*				mpDecodeIndex;
		
		// RLE
		RLEDecodeStream		mRLEIndex;
		RLEDecodeStream		mRLEPage;

		// Switched RLE
		SRLEDecodeStream	mSRLEIndex;
		SRLEDecodeStream	mSRLEPage;

		// RLE/HC
		RLEDecodeStream		mRLEHCIndex;
		RLEDecodeStream		mRLEHCPage;

		// Adaptive RLE/HC
		RLEHC2DecodeStream	mRLEHC2Index;
		RLEHC2DecodeStream	mRLEHC2Page;
	};

}
