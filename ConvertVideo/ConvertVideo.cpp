#include "stdafx.h"
#include "ConvertVideo.h"

#include "MoviePlayer.h"

using namespace app;

namespace {

	u8 bitNum(u8 data)
	{
		data = (data & 0x55) + ((data & 0xAA) >> 1);
		data = (data & 0x33) + ((data & 0xCC) >> 2);
		data = (data & 0x0F) + ((data & 0xF0) >> 4);
		return data;
	}

}




u8 RLDecoder::get()
{
	if (mContinue) {
		if (mNum == 0)
			mNum = (*mpVector)[mPt++];

		if (mNum > 0) {
			const u8 prev = mPrev;
			if (--mNum <= 0) {
				mPrev = -1;
				mContinue = false;
			}
			return prev;
		}
	}

	const u8 curr = (*mpVector)[mPt++];
	mContinue = mPrev == (s32)curr;
	return mPrev = curr;
}




ConvertVideo::ConvertVideo()
	: mpPlayer(nullptr)
	, mpBitmap(nullptr)
{
}

ConvertVideo::~ConvertVideo()
{
	for (auto& frame : mData.frame)
		SAFE_DELETE_ARRAY(frame.data);
}

void ConvertVideo::initialize()
{
	super::initialize();
	MoviePlayer::initialize();

	mpPlayer = new MoviePlayer;
	mpBitmapWIC = new BitmapWIC(80, 64);

	s32 argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

	if (!argv)
		return;

	if (argc == 2)
		mpPlayer->open(argv[1]);

	mpBitmap = mpPlayer->createBitmap(getMainRenderTarget(), 0);

	for (u32 i = 0; i < 2; ++i)
		mFrame[i].clear();
	mFramePt = 0;

	createFrame(mFrame[mFramePt], mpPlayer->getBitmapWIC());
	mData.frame.push_back(mFrame[mFramePt]);

	mCompressed = false;
	mFrameCount = 0;
	mWordCount = 0;
	mWordNumMax = 0;
	mWordCountMaxMax = 0;

	LocalFree(argv);

	HWND hTrack = CreateWindowEx(
		0,
		TRACKBAR_CLASS,
		L"TrackBar",
		WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_TOOLTIPS,
		0, 
		0, 
		512, 
		24,
		getMainWindow()->getHandle(),
		0,
		GetModuleHandle(nullptr),
		nullptr);

	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 6572));	// 30fps
	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 5256));	// 24fps
	SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 3285));	// 15fps
	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 2190));	// 10fps
	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 1752));	// 8fps
	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 876));	// 4fps
	//SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 400));	// test
	SendMessage(hTrack, TBM_SETPOS, TRUE, 0);

	getMainWindow()->redraw();

	mDecodePt = 0;
	mDecodePt2.clear();
	mRLDecoderIndex.set(&mData.c_index);
	mRLDecoderPage.set(&mData.c_page);
}

void ConvertVideo::finalize()
{
	SAFE_DELETE(mpPlayer);

	//for (u32 i = 0; i < 2; ++i)
	//	mFrame[i].clear();

	MoviePlayer::finalize();
	super::finalize();
}

void ConvertVideo::update()
{
#if 0
	//===================================================================================
	// Decode uncompressed full-frame data
	//===================================================================================
	if (mCompressed)
	{
		if (mDecodePt >= mData.frame.size())
			mDecodePt = 0;

		SAFE_RELEASE(mpBitmap);
		mpBitmap = createBitmapFromFrame(mData.frame[mDecodePt++]);

		getMainWindow()->redraw();
		return;
	}
#elif 0
	//===================================================================================
	// Decode compressed data
	//===================================================================================
	if (mCompressed)
	{
		if (mDecodePt2.index >= mData.index.size())
			mDecodePt2.clear();

		const u32 w = 80;
		const u32 h = 64;
		const u32 pageH = h >> 3;

		if (!mDecodeFrame.data) {
			mDecodeFrame.width = w;
			mDecodeFrame.height = 64;
			mDecodeFrame.size = w * 8;
			mDecodeFrame.data = new u8[mDecodeFrame.size];
			memset(mDecodeFrame.data, 0, mDecodeFrame.size);
		}

		u8* index = new u8[w];

#if 0
		for (u32 x = 0; x < w; ++x) {
			index[x] = mData.index[mDecodePt2.index++];
			for (u32 y = 0; y < pageH; ++y) {
				if (index[x] & ZEN_BIT(y)) {
					const u8 page = mData.page[mDecodePt2.page++];
					mDecodeFrame.data[y * w + x] = page;
				}
			}
		}
#else
		for (u32 x = 0; x < w; ++x)
			index[x] = mData.index[mDecodePt2.index++];

		for (u32 y = 0; y < pageH; ++y) {
			for (u32 x = 0; x < w; ++x) {
				if (index[x] & ZEN_BIT(y)) {
					const u8 page = mData.page[mDecodePt2.page++];
					mDecodeFrame.data[y * w + x] = page;
				}
			}
		}
#endif

		SAFE_RELEASE(mpBitmap);
		mpBitmap = createBitmapFromFrame(mDecodeFrame, index);

		SAFE_DELETE_ARRAY(index);

		getMainWindow()->redraw();
		Sleep(420);
		return;
	}
#elif 1
	//===================================================================================
	// Decode compressed data RLE
	//===================================================================================
	if (mCompressed)
	{
		if (mRLDecoderIndex.isEmpty()) {
			mRLDecoderIndex.clear();
			mRLDecoderPage.clear();
		}

		const u32 w = 80;
		const u32 h = 64;
		const u32 pageH = h >> 3;

		if (!mDecodeFrame.data) {
			mDecodeFrame.width = w;
			mDecodeFrame.height = 64;
			mDecodeFrame.size = w * 8;
			mDecodeFrame.data = new u8[mDecodeFrame.size];
			memset(mDecodeFrame.data, 0, mDecodeFrame.size);
		}

		u8* index = new u8[w];

		for (u32 x = 0; x < w; ++x)
			index[x] = mRLDecoderIndex.get();

		for (u32 y = 0; y < pageH; ++y) {
			for (u32 x = 0; x < w; ++x) {
				if (index[x] & ZEN_BIT(y)) {
					const u8 page = mRLDecoderPage.get();
					mDecodeFrame.data[y * w + x] = page;
				}
			}
		}

		SAFE_RELEASE(mpBitmap);
		mpBitmap = createBitmapFromFrame(mDecodeFrame, index);

		SAFE_DELETE_ARRAY(index);

		getMainWindow()->redraw();
		Sleep(33);
		return;
	}
#else
	if (mCompressed)
	{
		if (mNumDecoder.pt >= mData.num.size())
		{
			mNumDecoder.pt = mIndexDecoder.pt = mPageDecoder.pt = 0;
		}

		auto num = mData.num[mNumDecoder.pt++];

		u8 index[80];
		memset(index, 0, sizeof(index));
		for (u32 i = 0; i < 80; ++i) {
			if (mData.distr[i / 8] & (1 << (i % 8))) {
				index[i] = mData.index[mIndexDecoder.pt++];
			}
		}

		IWICBitmapLock* pLock = nullptr;
		if (FAILED(mpPlayer->getWICBitmap()->Lock(nullptr, WICBitmapLockWrite, &pLock)))
			return;

		u32 bufsize;
		u8* buf;
		pLock->GetDataPointer(&bufsize, &buf);

		u32 s;
		pLock->GetStride(&s);

		u32 w, h;
		pLock->GetSize(&w, &h);

		if (mNumDecoder.pt == 1)
		{
			memset(buf, 0, s * h);
		}

		const u32 pitch = s / w;
		const u32 pageH = h >> 3;
		for (u32 y = 0; y < pageH; ++y) {
			for (u32 x = 0; x < w; ++x) {
				const u32 i = (y << 3) * s + x * pitch;

				if (index[x] & (1 << y)) {
					const u8 page = mData.page[mPageDecoder.pt++];
					for (u32 yy = 0; yy < 8; ++yy) {
						for (u32 j = 0; j < pitch; ++j) {
							buf[i + yy * s + j] = page;
						}
					}
				}
			}
		}

		pLock->Release();

		SAFE_RELEASE(mpBitmap);
		getMainRenderTarget()->CreateBitmapFromWicBitmap(mpPlayer->getWICBitmap().Get(), &mpBitmap);

		InvalidateRect(getMainWindow()->getHandle(), nullptr, FALSE);
		return;
	}
#endif

	HWND hTrack = GetDlgItem(getMainWindow()->getHandle(), 0);
	s32 pos = SendMessage(hTrack, TBM_GETPOS, 0, 0);

	const s32 rangeMax = SendMessage(hTrack, TBM_GETRANGEMAX, 0, 0);
	pos = min(pos + 1, rangeMax);

	if (mMemPos != pos)
	{
		mMemPos = pos;
		SendMessage(hTrack, TBM_SETPOS, TRUE, pos);

		SAFE_RELEASE(mpBitmap);

		//const s32 rangeMax = SendMessage(hTrack, TBM_GETRANGEMAX, 0, 0);
		s64 t = static_cast<s64>((f32)mpPlayer->getDuration() * pos / rangeMax);
		mpBitmap = mpPlayer->createBitmap(getMainRenderTarget(), t);

		//mFrame[mFramePt].clear();
		if (createFrame(mFrame[mFramePt], mpPlayer->getBitmapWIC()))
		{
			//-----------------------------------------------------------------------------
			// uncompressed full-frame data
			mData.frame.push_back(mFrame[mFramePt]);

			//-----------------------------------------------------------------------------
			// compressed data
			const u32 size = mFrame[mFramePt].size;
			const u32 w = mFrame[mFramePt].width;
			const u32 h = mFrame[mFramePt].height;
			u8* curr = mFrame[mFramePt].data;
			u8* prev = mFrame[(mFramePt + 1) % 2].data;

			const u32 indexSize = w;
			const u32 pageH = h >> 3;

#if 0
			for (u32 x = 0; x < w; ++x) {
				u8 index = 0;
				for (u32 y = 0; y < pageH; ++y) {
					const u32 i = y * w + x;
					const u8 page = curr[i];
					if (!prev || prev[i] != page) {
						index |= ZEN_BIT(y);
						mData.page.push_back(page);
					}
				}
				mData.index.push_back(index);
			}
#else
			u8* index = new u8[w];
			memset(index, 0, w);

			for (u32 y = 0; y < pageH; ++y) {
				for (u32 x = 0; x < w; ++x) {
					const u32 i = y * w + x;
					if (!prev || prev[i] != curr[i]) {
						index[x] |= ZEN_BIT(y);
						mData.page.push_back(curr[i]);
					}
				}
			}

			for (u32 x = 0; x < w; ++x)
				mData.index.push_back(index[x]);

			//-----------------------------------------------------------------------------
			// compressed data 2
			{
				const u32 dsize = w >> 3;
				u8* sindex_d = new u8[dsize];
				memset(sindex_d, 0, dsize);

				for (u32 x = 0; x < w; ++x) {
					if (index[x]) {
						sindex_d[x >> 3] |= ZEN_BIT(x & 0x7);
						mData.sindex.push_back(index[x]);
					}
				}

				for (u32 i = 0; i < dsize; ++i)
					mData.sindex_d.push_back(sindex_d[i]);

				SAFE_DELETE_ARRAY(sindex_d);
			}

			//-----------------------------------------------------------------------------
			// compressed data 3
			{
				u32 n = 0;
				u32 dict[0x100];
				memset(dict, 0, sizeof(dict));

				for (u32 y = 0; y < pageH; ++y) {
					for (u32 x = 0; x < w; ++x) {
						const u32 i = y * w + x;
						if (!prev || prev[i] != curr[i]) {
							if (!dict[curr[i]])
								++n;
							++dict[curr[i]];
						}
					}
				}

				u32 mx = 0;
				for (u32 i = 0; i < 0x100; ++i)
					mx = max(mx, dict[i]);

				ZEN_TRACELINE(L"WordNum = %d", n);
				ZEN_TRACELINE(L"WordCountMax = %d", mx);
				mWordNumMax = max(mWordNumMax, n);
				mWordCountMaxMax = max(mWordCountMaxMax, mx);
				mWordCount += n;
			}
			
			SAFE_DELETE_ARRAY(index);
#endif
		}
		mFramePt = (mFramePt + 1) % 2;

		++mFrameCount;

		getMainWindow()->redraw();
	}

	if (pos == rangeMax && !mCompressed)
	{
		mCompressed = true;

		compressRLE(mData.c_index, mData.index);
		compressRLE(mData.c_page, mData.page);
		compressRLE(mData.c_sindex_d, mData.sindex_d);
		compressRLE(mData.c_sindex, mData.sindex);

		ZEN_TRACELINE(L"FrameCount = %d", mFrameCount);
		ZEN_TRACELINE(L"WordNumAvg = %d", mWordCount / mFrameCount);
		ZEN_TRACELINE(L"WordNumMax = %d", mWordNumMax);
		ZEN_TRACELINE(L"WordCountMaxMax = %d", mWordCountMaxMax);

		ZEN_TRACELINE(L"Index Size                 = %8.2f KB", (f32)mData.index.size() / 1024);
		ZEN_TRACELINE(L"Index Compressed Size      = %8.2f KB", (f32)mData.c_index.size() / 1024);
		ZEN_TRACELINE(L"Index Compression Ratio    = %8.2f %%", 100.f * mData.c_index.size() / mData.index.size());
		ZEN_TRACELINE(L"Page Size                  = %8.2f KB", (f32)mData.page.size() / 1024);
		ZEN_TRACELINE(L"Page Compressed Size       = %8.2f KB", (f32)mData.c_page.size() / 1024);
		ZEN_TRACELINE(L"Page Compression Ratio     = %8.2f %%", 100.f * mData.c_page.size() / mData.page.size());
		ZEN_TRACELINE(L"--------------------------------------------");
		ZEN_TRACELINE(L"SIndex Size                = %8.2f KB", (f32)mData.sindex.size() / 1024);
		ZEN_TRACELINE(L"SIndex Compressed Size     = %8.2f KB", (f32)mData.c_sindex.size() / 1024);
		ZEN_TRACELINE(L"SIndex Compression Ratio   = %8.2f %%", 100.f * mData.c_sindex.size() / mData.sindex.size());
		ZEN_TRACELINE(L"SIndexD Size               = %8.2f KB", (f32)mData.sindex_d.size() / 1024);
		ZEN_TRACELINE(L"SIndexD Compressed Size    = %8.2f KB", (f32)mData.c_sindex_d.size() / 1024);
		ZEN_TRACELINE(L"SIndexD Compression Ratio  = %8.2f %%", 100.f * mData.c_sindex_d.size() / mData.sindex_d.size());
		ZEN_TRACELINE(L"--------------------------------------------");
		const size_t totalSize = mData.index.size() + mData.page.size();
		const size_t totalCompressedSize = mData.c_index.size() + mData.c_page.size();
		ZEN_TRACELINE(L"Total Size                 = %8.2f KB", (f32)totalSize / 1024);
		ZEN_TRACELINE(L"Total Compressed Size      = %8.2f KB", (f32)totalCompressedSize / 1024);
		ZEN_TRACELINE(L"Total Comression Ratio     = %8.2f %%", 100.f * totalCompressedSize / totalSize);
	}
}

void ConvertVideo::draw(ID2D1RenderTarget* pRT)
{
	pRT->Clear(D2D1::ColorF(D2D1::ColorF::DarkBlue));
	
	if (mpBitmap)
	{
		const f32 dstWidth = 512.f;
		D2D1_SIZE_F size(mpBitmap->GetSize());
		D2D1_RECT_F rect;
		rect.left = 0;
		rect.top = 24;
		rect.right = rect.left + dstWidth;
		rect.bottom = rect.top + size.height * dstWidth / size.width;
		pRT->DrawBitmap(mpBitmap, rect, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	}
}

ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src)
{
	const u32 dataSize = mpBitmapWIC->getDataSize();
	const u32 w = mpBitmapWIC->getWidth();
	const u32 h = mpBitmapWIC->getHeight();
	const u32 d = mpBitmapWIC->getDepth();
	const u32 s = mpBitmapWIC->getStride();
	if (u8* pData = mpBitmapWIC->map())
	{
		memset(pData, 0, dataSize);

		for (u32 y = 0; y < h; ++y) {
			for (u32 x = 0; x < w; ++x) {
				const u32 i = y * s + x * d;

				const u8 page = src.data[(y >> 3) * src.width + x];
				const u8 p = (page & ZEN_BIT(y & 0x07)) ? 0xFF : 0;

				pData[i] = 0;
				for (u32 j = 1; j < d; ++j)
					pData[i + j] = p;
			}
		}
		
		mpBitmapWIC->unmap();
	}

	ID2D1Bitmap* pBitmap = nullptr;
	getMainRenderTarget()->CreateBitmapFromWicBitmap(mpBitmapWIC->getHandle(), &pBitmap);
	return pBitmap;
}

ID2D1Bitmap* ConvertVideo::createBitmapFromFrame(const Frame &src, const u8* index)
{
	const u32 dataSize = mpBitmapWIC->getDataSize();
	const u32 w = mpBitmapWIC->getWidth();
	const u32 h = mpBitmapWIC->getHeight();
	const u32 d = mpBitmapWIC->getDepth();
	const u32 s = mpBitmapWIC->getStride();
	if (u8* pData = mpBitmapWIC->map())
	{
		memset(pData, 0, dataSize);

		for (u32 y = 0; y < h; ++y) {
			for (u32 x = 0; x < w; ++x) {
				const bool b = (index[x] & ZEN_BIT(y >> 3)) ? true : false;

				const u32 i = y * s + x * d;

				const u8 page = src.data[(y >> 3) * src.width + x];
				const u8 p = (page & ZEN_BIT(y & 0x07)) ? 0xFF : 0;

#if 0// purple and cyan
				pData[i    ] = b ? 0xFF : p;	// B
				pData[i + 1] = b ? p : p;		// G
				pData[i + 2] = b ? 0x80 : p;	// R
				pData[i + 3] = 0xFF;			// A
#elif 1// blue and pink
				pData[i    ] = b ? 0xFF : p;	// B
				pData[i + 1] = b ? 0x80 : p;	// G
				pData[i + 2] = b ? p : p;		// R
				pData[i + 3] = 0xFF;			// A
#else// magenta and yellow
				pData[i    ] = b ? 0x80 : p;	// B
				pData[i + 1] = b ? p : p;		// G
				pData[i + 2] = b ? 0xFF : p;	// R
				pData[i + 3] = 0xFF;			// A
#endif
			}
		}

		mpBitmapWIC->unmap();
	}

	ID2D1Bitmap* pBitmap = nullptr;
	getMainRenderTarget()->CreateBitmapFromWicBitmap(mpBitmapWIC->getHandle(), &pBitmap);
	return pBitmap;
}

bool ConvertVideo::createFrame(Frame &dst, BitmapWIC* psrc)
{
	const u32 dataSize = psrc->getDataSize();
	const u32 w = psrc->getWidth();
	const u32 h = psrc->getHeight();
	const u32 d = psrc->getDepth();
	const u32 s = psrc->getStride();
	if (u8* pData = psrc->map())
	{
		const u32 frameSize = (w * h) >> 3;//(dataSize / d) >> 3;
		u8* frame = new u8[frameSize];
		const u32 pageH = h >> 3;
		for (u32 y = 0; y < pageH; ++y) {
			for (u32 x = 0; x < w; ++x) {
				const u32 i = (y << 3) * s + x * d;
#if 1
				frame[y * w + x] =
					  (pData[i        ] ? 0x01 : 0)
					| (pData[i + s    ] ? 0x02 : 0)
					| (pData[i + s * 2] ? 0x04 : 0)
					| (pData[i + s * 3] ? 0x08 : 0)
					| (pData[i + s * 4] ? 0x10 : 0)
					| (pData[i + s * 5] ? 0x20 : 0)
					| (pData[i + s * 6] ? 0x40 : 0)
					| (pData[i + s * 7] ? 0x80 : 0);
#else
				for (u32 j = 0; j < 8; ++j)
					if (pData[i + sy * j]) frame[y * w + x] |= 1 << j;
#endif
			}
		}

		dst.data = frame;
		dst.size = frameSize;
		dst.width = w;
		dst.height = h;

		psrc->unmap();
		return true;
	}
	return false;
}

void ConvertVideo::compressRLE(vector<u8> &dst, const vector<u8> &src)
{
	if (!src.size())
		return;

	dst.clear();

	u8 data = src[0];
	u8 length = 1;
	for (auto i = 1; i <= src.size(); ++i)
	{
		if (length < 0xFE && i != src.size() && data == src[i])
		{
			++length;
		}
		else
		{
			dst.push_back(data);
			if (length > 1) {
				dst.push_back(data);
				dst.push_back(length - 2);
			}
			if (i != src.size()) {
				data = src[i];
				length = 1;
			}
		}
	}
}