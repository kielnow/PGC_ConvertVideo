#pragma once

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

		Frame() : data(nullptr) { clear(); }

		void clear()
		{
			SAFE_DELETE_ARRAY(data);
			size = width = height = 0;
		}
	};

	struct Data
	{
		// uncompressed full-frame data
		vector<Frame>	frame;

		// compressed data
		vector<u8>		index;
		vector<u8>		page;

		// compressed data 2
		vector<u8>		sindex;
		vector<u8>		sindex_d;

		// RLE
		vector<u8>		c_index;
		vector<u8>		c_page;
		vector<u8>		c_sindex;
		vector<u8>		c_sindex_d;
	};

	class RLDecoder
	{
	public:
		RLDecoder(vector<u8>* pv = nullptr) : mpVector(pv) { clear(); }

		bool isEmpty() const { return !mNum && mPt >= mpVector->size(); }

		u8 get();

		void set(vector<u8>* pv)
		{
			mpVector = pv;
			clear();
		}

		void clear()
		{
			mPt = mNum = 0;
			mPrev = -1;
			mContinue = false;
		}

	protected:
		vector<u8>* mpVector;
		u32 mPt;
		s32 mNum;
		s32 mPrev;
		bool mContinue;
	};

	struct Output
	{
		u8		frameRate;
		u8		width;
		u8		height;

		u32		num;
		u32		index;
		u32		page;
		u8		data[1];

		Output() { clear(); }

		void clear()
		{
			num = index = page = 0;
			data[0] = 0;
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
		ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src);

		ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src, const u8* index);
			
		static bool createFrame(Frame &dst, BitmapWIC* psrc);

		//! Run length encoding
		static void compressRLE(vector<u8> &dst, const vector<u8> &src);

		MoviePlayer* mpPlayer;

		ID2D1Bitmap* mpBitmap;
		u32 mMemPos;

		Frame mFrame[2];
		u32 mFramePt;
		Data mData;

		bool mCompressed;
		u32 mFrameCount;
		u32 mWordCount;
		u32 mWordCountMaxMax;
		u32 mWordNumMax;

		BitmapWIC* mpBitmapWIC;

		u32 mDecodePt;

		struct {
			u32 index;
			u32 page;

			void clear()
			{
				index = page = 0;
			}
		} mDecodePt2;
		Frame mDecodeFrame;

		RLDecoder mRLDecoderIndex;
		RLDecoder mRLDecoderPage;
	};

}
