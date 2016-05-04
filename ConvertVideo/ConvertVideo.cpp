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

	const u32 ID_TRACKBAR = 0;

	enum {
		LCD_WIDTH		= 128,
		LCD_HEIGHT		= 64,
		LCD_PAGE_SIZE	= 8,
	};

}


ConvertVideo::ConvertVideo()
	: mpPlayer(nullptr)
	, mpBitmap(nullptr)
	, mpBitmapWIC(nullptr)
	, mpDecodeIndex(nullptr)
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

	getMainWindow()->setSize(640, 480);

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
		ID_TRACKBAR,
		GetModuleHandle(nullptr),
		nullptr);

	mpPlayer = new MoviePlayer;

	mFrameRate = 8;

	s32 argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

	if (!argv)
		return;

	if (argc == 2)
		open(argv[1]);

	LocalFree(argv);
}

void ConvertVideo::clear()
{
	SAFE_RELEASE(mpBitmap);
	SAFE_DELETE(mpBitmapWIC);

	mData.clear();

	memset(mFrame, 0, sizeof(mFrame));
	mFramePt = 0;

	mMemPos = (u32)-1;
	mIsReady = false;
	mIsOpen = false;

	mDecodePt = mDecodeIndexPt = mDecodePagePt = 0;
	mDecodeFrame.clear();
	SAFE_DELETE_ARRAY(mpDecodeIndex)

	mRLEIndex	.setStream(new VectorStream(&mData.c_index), true);
	mRLEPage	.setStream(new VectorStream(&mData.c_page), true);
	mSRLEIndex	.setStream(new VectorStream(&mData.c2_index), true);
	mSRLEPage	.setStream(new VectorStream(&mData.c2_page), true);
	mRLEHCIndex	.setStream(new HCDecodeStream, true);
	mRLEHCPage	.setStream(new HCDecodeStream, true);
}

void ConvertVideo::open(ZEN_CWSTR path)
{
	if (!path)
		return;

	clear();

	if (!mpPlayer->open(path))
		return;

	mIsOpen = true;

	const u32 w = mpPlayer->getWidth();
	const u32 h = mpPlayer->getHeight();
	if (w * LCD_HEIGHT < h * LCD_WIDTH) {
		mFrameHeight = LCD_HEIGHT;
		mFrameWidth = static_cast<u32>((f32)w * ((f32)mFrameHeight / h));
	} else {
		mFrameWidth = LCD_WIDTH;
		mFrameHeight = static_cast<u32>((f32)h * ((f32)mFrameWidth / w));
	}
	mFramePageHeight = (mFrameHeight + 7) >> 3;
	mpPlayer->setDestSize(mFrameWidth, mFrameHeight);

	mpBitmap = mpPlayer->createBitmap(getMainRenderTarget(), 0);
	mpBitmapWIC = new BitmapWIC(mFrameWidth, mFrameHeight);

	createFrame(mFrame[mFramePt], mpPlayer->getBitmapWIC());
	mData.frame.push_back(mFrame[mFramePt]);

	const s64 t = 10000000 / mFrameRate;
	mFrameCount = static_cast<u32>(mpPlayer->getDuration() / t);

	HWND hTrack = GetDlgItem(getMainWindow()->getHandle(), ID_TRACKBAR);
	SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, mFrameCount));
	SendMessage(hTrack, TBM_SETPOS, TRUE, 0);

	// preparing to decode
	mDecodeFrame.alloc(mFrameWidth, mFrameHeight);
	mpDecodeIndex = new u8[mFrameWidth];

	getMainWindow()->redraw();
}

void ConvertVideo::finalize()
{
	clear();

	SAFE_DELETE(mpPlayer);

	MoviePlayer::finalize();
	super::finalize();
}

void ConvertVideo::update()
{
	if (!mIsOpen)
		return;

	if (mIsReady)
	{
		//play();
		//playDelta();
		//playDeltaRLE();
		//playDeltaSRLE();
		//playDeltaRLEHC();
		playDeltaRLEHC2();

		getMainWindow()->redraw();
		const u32 t = (u32)(1000 / mFrameRate) >> 1;
		//Sleep(t);
		return;
	}

	HWND hTrack = GetDlgItem(getMainWindow()->getHandle(), ID_TRACKBAR);
	const s32 rangeMax = (s32)SendMessage(hTrack, TBM_GETRANGEMAX, 0, 0);
	s32 pos = (s32)SendMessage(hTrack, TBM_GETPOS, 0, 0);
	pos = min(pos + 1, rangeMax);

	if (mMemPos != pos)
	{
		mMemPos = pos;
		SendMessage(hTrack, TBM_SETPOS, TRUE, pos);

		SAFE_RELEASE(mpBitmap);

		const s64 t = static_cast<s64>(mpPlayer->getDuration() * pos / rangeMax);
		mpBitmap = mpPlayer->createBitmap(getMainRenderTarget(), t);

		//mFrame[mFramePt].clear();
		if (createFrame(mFrame[mFramePt], mpPlayer->getBitmapWIC()))
		{
			//-----------------------------------------------------------------------------
			// uncompressed full-frame data
			mData.frame.push_back(mFrame[mFramePt]);

			//-----------------------------------------------------------------------------
			// delta compression
			const u32 w = mFrameWidth;
			const u32 h = mFrameHeight;
			const u32 pageH = mFramePageHeight;
			u8* curr = mFrame[mFramePt].data;
			u8* prev = mFrame[(mFramePt + 1) % 2].data;

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
#endif

			//-----------------------------------------------------------------------------
			// sparse index compression
			{
				const u32 dsize = (w + 7) >> 3;
				u8* dist = new u8[dsize];
				memset(dist, 0, dsize);

				for (u32 x = 0; x < w; ++x) {
					if (index[x]) {
						dist[x >> 3] |= ZEN_BIT(x & 0x7);
						mData.sindex.push_back(index[x]);
					}
				}

				for (u32 i = 0; i < dsize; ++i)
					mData.dist.push_back(dist[i]);

				SAFE_DELETE_ARRAY(dist);
			}

			SAFE_DELETE_ARRAY(index);
		}
		mFramePt = (mFramePt + 1) % 2;

		getMainWindow()->redraw();
	}

	if (pos == rangeMax && !mIsReady)
	{
		mIsReady = true;

#if 0
		convertBWT(mData.bwt_index, mData.bwt_n_index, mData.index)	
		compressRLE(mData.c_bwt_index, mData.bwt_index);
		compressRLE(mData.c_bwt_n_index, mData.bwt_n_index);

		convertBWT(mData.bwt_page, mData.bwt_n_page, mData.page);
		compressRLE(mData.c_bwt_page, mData.bwt_page);
		compressRLE(mData.c_bwt_n_page, mData.bwt_n_page);
#endif

		// RLE
		compressRLE(mData.c_index, mData.index);
		compressRLE(mData.c_page, mData.page);
		compressRLE(mData.c_sindex, mData.sindex);
		compressRLE(mData.c_dist, mData.dist);

		// Switched RLE
		compressSRLE(mData.c2_index, mData.index);
		compressSRLE(mData.c2_page, mData.page);

		// RLE/HC
		convertHC(mData.hc_index, mData.c_index);
		convertHC(mData.hc_page, mData.c_page);
		convertHC(mData.hc_sindex, mData.c_sindex);
		convertHC(mData.hc_dist, mData.c_dist);
		static_cast<HCDecodeStream*>(mRLEHCIndex.getStream())->set(mData.hc_index);
		static_cast<HCDecodeStream*>(mRLEHCPage.getStream())->set(mData.hc_page);

		// Adaptive RLE/HC
		convertHC2(mData.hc2_index, mData.c_index);
		convertHC2(mData.hc2_page, mData.c_page);
		convertHC2(mData.hc2_sindex, mData.c_sindex);
		convertHC2(mData.hc2_dist, mData.c_dist);
		mRLEHC2Index.setData(&mData.hc2_index);
		mRLEHC2Page.setData(&mData.hc2_page);

		const size_t sz_index			= mData.index.size();
		const size_t sz_page			= mData.page.size();
		const size_t sz_sindex			= mData.sindex.size();
		const size_t sz_dist			= mData.dist.size();
		const size_t sz_sindex_dist		= mData.sindex.size() + mData.dist.size();
		const size_t sz_c_sindex_dist	= mData.c_sindex.size() + mData.c_dist.size();
		const size_t sz_hc_sindex_dist	= mData.hc_sindex.size() + mData.hc_dist.size();
		const size_t sz_hc2_sindex_dist	= mData.hc2_sindex.size() + mData.hc2_dist.size();
		ZEN_TRACELINE(L"FrameCount                 = %8d", mFrameCount);
		ZEN_TRACELINE(L"Index Size                 = %8.2f KB",				(f32)sz_index					/ 1024);
		ZEN_TRACELINE(L"Index RLE Size             = %8.2f KB (%8.2f %%)",	(f32)mData.c_index.size()		/ 1024,	100.f * mData.c_index.size()		/ sz_index);
		ZEN_TRACELINE(L"Index SRLE Size            = %8.2f KB (%8.2f %%)",	(f32)mData.c2_index.size()		/ 1024,	100.f * mData.c2_index.size()		/ sz_index);
		ZEN_TRACELINE(L"Index RLE/HC Size          = %8.2f KB (%8.2f %%)",	(f32)mData.hc_index.size()		/ 1024,	100.f * mData.hc_index.size()		/ sz_index);
		ZEN_TRACELINE(L"Index RLE/HC2 Size         = %8.2f KB (%8.2f %%)",	(f32)mData.hc2_index.size()		/ 1024,	100.f * mData.hc2_index.size()		/ sz_index);
		ZEN_TRACELINE(L"SIndex+Dist Size           = %8.2f KB (%8.2f %%)",	(f32)sz_sindex_dist				/ 1024,	100.f * sz_sindex_dist				/ sz_index);
		ZEN_TRACELINE(L"SIndex+Dist RLE Size       = %8.2f KB (%8.2f %%)",	(f32)sz_c_sindex_dist			/ 1024,	100.f * sz_c_sindex_dist			/ sz_index);
		ZEN_TRACELINE(L"SIndex+Dist RLE/HC Size    = %8.2f KB (%8.2f %%)",	(f32)sz_hc_sindex_dist			/ 1024,	100.f * sz_hc_sindex_dist			/ sz_index);
		ZEN_TRACELINE(L"SIndex+Dist RLE/HC2 Size   = %8.2f KB (%8.2f %%)",	(f32)sz_hc2_sindex_dist			/ 1024,	100.f * sz_hc2_sindex_dist			/ sz_index);
		ZEN_TRACELINE(L"Page Size                  = %8.2f KB",				(f32)sz_page					/ 1024);
		ZEN_TRACELINE(L"Page RLE Size              = %8.2f KB (%8.2f %%)",	(f32)mData.c_page.size()		/ 1024,	100.f * mData.c_page.size()			/ sz_page);
		ZEN_TRACELINE(L"Page SRLE Size             = %8.2f KB (%8.2f %%)",	(f32)mData.c2_page.size()		/ 1024,	100.f * mData.c2_page.size()		/ sz_page);
		ZEN_TRACELINE(L"Page RLE/HC Size           = %8.2f KB (%8.2f %%)",	(f32)mData.hc_page.size()		/ 1024,	100.f * mData.hc_page.size()		/ sz_page);
		ZEN_TRACELINE(L"Page RLE/HC2 Size          = %8.2f KB (%8.2f %%)",	(f32)mData.hc2_page.size()		/ 1024,	100.f * mData.hc2_page.size()		/ sz_page);
		ZEN_TRACELINE(L"--------------------------------------------");
#if 0
		ZEN_TRACELINE(L"SIndex Size			       = %8.2f KB",				(f32)sz_sindex					/ 1024);
		ZEN_TRACELINE(L"SIndex RLE Size	           = %8.2f KB (%8.2f %%)",	(f32)mData.c_sindex.size()		/ 1024,	100.f * mData.c_sindex.size()		/ sz_sindex);
		ZEN_TRACELINE(L"SIndex RLE/HC Size	       = %8.2f KB (%8.2f %%)",	(f32)mData.hc_sindex.size()		/ 1024,	100.f * mData.hc_sindex.size()		/ sz_sindex);
		ZEN_TRACELINE(L"SIndex RLE/HC2 Size	       = %8.2f KB (%8.2f %%)",	(f32)mData.hc2_sindex.size()	/ 1024,	100.f * mData.hc2_sindex.size()		/ sz_sindex);
		ZEN_TRACELINE(L"Dist Size                  = %8.2f KB",				(f32)sz_dist					/ 1024);
		ZEN_TRACELINE(L"Dist RLE Size              = %8.2f KB (%8.2f %%)",	(f32)mData.c_dist.size()		/ 1024,	100.f * mData.c_dist.size()			/ sz_dist);
		ZEN_TRACELINE(L"Dist RLE/HC Size           = %8.2f KB (%8.2f %%)",	(f32)mData.hc_dist.size()		/ 1024,	100.f * mData.hc_dist.size()		/ sz_dist);
		ZEN_TRACELINE(L"Dist RLE/HC2 Size          = %8.2f KB (%8.2f %%)",	(f32)mData.hc2_dist.size()		/ 1024,	100.f * mData.hc2_dist.size()		/ sz_dist);
		ZEN_TRACELINE(L"--------------------------------------------");
#endif
#if 0
		ZEN_TRACELINE(L"BWT Index Size                = %8.2f KB", (f32)mData.bwt_index.size() / 1024);
		ZEN_TRACELINE(L"BWT Index Compressed Size     = %8.2f KB", (f32)mData.c_bwt_index.size() / 1024);
		ZEN_TRACELINE(L"BWT Index Compression Ratio   = %8.2f %%", 100.f * mData.c_bwt_index.size() / mData.bwt_index.size());
		ZEN_TRACELINE(L"BWT N Index Size              = %8.2f KB", (f32)mData.bwt_n_index.size() * 2/ 1024);
		ZEN_TRACELINE(L"BWT N Index Compressed Size   = %8.2f KB", (f32)mData.c_bwt_n_index.size() * 2 / 1024);
		ZEN_TRACELINE(L"BWT N Index Compression Ratio = %8.2f %%", 100.f * mData.c_bwt_n_index.size() / mData.bwt_n_index.size());
		ZEN_TRACELINE(L"--------------------------------------------");
		ZEN_TRACELINE(L"BWT Page Size                = %8.2f KB", (f32)mData.bwt_page.size() / 1024);
		ZEN_TRACELINE(L"BWT Page Compressed Size     = %8.2f KB", (f32)mData.c_bwt_page.size() / 1024);
		ZEN_TRACELINE(L"BWT Page Compression Ratio   = %8.2f %%", 100.f * mData.c_bwt_page.size() / mData.bwt_page.size());
		ZEN_TRACELINE(L"BWT N Page Size              = %8.2f KB", (f32)mData.bwt_n_page.size() * 2 / 1024);
		ZEN_TRACELINE(L"BWT N Page Compressed Size   = %8.2f KB", (f32)mData.c_bwt_n_page.size() * 2 / 1024);
		ZEN_TRACELINE(L"BWT N Page Compression Ratio = %8.2f %%", 100.f * mData.c_bwt_n_page.size() / mData.bwt_n_page.size());
		ZEN_TRACELINE(L"--------------------------------------------");
#endif
		const size_t totalSize			= mData.index.size() + mData.page.size();
		const size_t totalRLESize		= mData.c_index.size() + mData.c_page.size();
		const size_t totalSRLESize		= mData.c2_index.size() + mData.c2_page.size();
		const size_t totalRLEHCSize		= mData.hc_index.size() + mData.hc_page.size();
		const size_t totalRLEHC2Size	= mData.hc2_index.size() + mData.hc2_page.size();
		const size_t totalSIRLEHCSize	= sz_hc_sindex_dist + mData.hc_page.size();
		const size_t totalSIRLEHC2Size	= sz_hc2_sindex_dist + mData.hc2_page.size();
		ZEN_TRACELINE(L"Total Size                 = %8.2f KB", (f32)totalSize / 1024);
		ZEN_TRACELINE(L"Total RLE Size             = %8.2f KB (%8.2f %%)", (f32)totalRLESize		/ 1024, 100.f * totalRLESize		/ totalSize);
		ZEN_TRACELINE(L"Total SRLE Size            = %8.2f KB (%8.2f %%)", (f32)totalSRLESize		/ 1024, 100.f * totalSRLESize		/ totalSize);
		ZEN_TRACELINE(L"Total RLE/HC Size          = %8.2f KB (%8.2f %%)", (f32)totalRLEHCSize		/ 1024, 100.f * totalRLEHCSize		/ totalSize);
		ZEN_TRACELINE(L"Total RLE/HC2 Size         = %8.2f KB (%8.2f %%)", (f32)totalRLEHC2Size		/ 1024, 100.f * totalRLEHC2Size		/ totalSize);
		ZEN_TRACELINE(L"Total SI/RLE/HC Size       = %8.2f KB (%8.2f %%)", (f32)totalSIRLEHCSize	/ 1024, 100.f * totalSIRLEHCSize	/ totalSize);
		ZEN_TRACELINE(L"Total SI/RLE/HC2 Size      = %8.2f KB (%8.2f %%)", (f32)totalSIRLEHC2Size	/ 1024, 100.f * totalSIRLEHC2Size	/ totalSize);

		//output("BadAppleResource.h");
	}
}

void ConvertVideo::output(const string &path)
{
	HeaderWriter w(path);
	w.write("using namespace pgc1000;");
	w.newLine();
	w.writeU32("MOVIE_FRAME_RATE", mFrameRate);
	w.writeU32("MOVIE_FRAME_COUNT", mFrameCount);
	w.writeU32("MOVIE_FRAME_WIDTH", mFrameWidth);
	w.writeU32("MOVIE_FRAME_HEIGHT", mFrameHeight);
	w.writeU32("MOVIE_FRAME_PAGE_HEIGHT", mFramePageHeight);
	w.writeU32("MOVIE_C_INDEX_SIZE", (u32)mData.c_index.size());
	w.writeU8Array("MOVIE_C_INDEX", mData.c_index.data(), (u32)mData.c_index.size());
	w.writeU32("MOVIE_C_PAGE_SIZE", (u32)mData.c_page.size());
	w.writeU8Array("MOVIE_C_PAGE", mData.c_page.data(), (u32)mData.c_page.size());
	w.close();
}

void ConvertVideo::play()
{
	if (mDecodePt >= mData.frame.size()) {
		mDecodePt = 0;
	}

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mData.frame[mDecodePt++]);
}

void ConvertVideo::playDelta()
{
	if (mDecodeIndexPt >= mData.index.size()) {
		mDecodeIndexPt = mDecodePagePt = 0;
	}

#if 0
	for (u32 x = 0; x < mFrameWidth; ++x) {
		index[x] = mData.index[mDecodePt2.index++];
		for (u32 y = 0; y < mFramePageHeight; ++y) {
			if (index[x] & ZEN_BIT(y)) {
				const u8 page = mData.page[mDecodePt2.page++];
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}
#else
	for (u32 x = 0; x < mFrameWidth; ++x)
		mpDecodeIndex[x] = mData.index[mDecodeIndexPt++];

	for (u32 y = 0; y < mFramePageHeight; ++y) {
		for (u32 x = 0; x < mFrameWidth; ++x) {
			if (mpDecodeIndex[x] & ZEN_BIT(y)) {
				const u8 page = mData.page[mDecodePagePt++];
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}
#endif

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mDecodeFrame, mpDecodeIndex);
}

void ConvertVideo::playDeltaRLE()
{
	if (mRLEIndex.isEmpty()) {
		mRLEIndex.reset();
		mRLEPage.reset();
	}

	for (u32 x = 0; x < mFrameWidth; ++x)
		mpDecodeIndex[x] = mRLEIndex.readU8();

	for (u32 y = 0; y < mFramePageHeight; ++y) {
		for (u32 x = 0; x < mFrameWidth; ++x) {
			if (mpDecodeIndex[x] & ZEN_BIT(y)) {
				const u8 page = mRLEPage.readU8();
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mDecodeFrame, mpDecodeIndex);
}

void ConvertVideo::playDeltaSRLE()
{
	if (mSRLEIndex.isEmpty()) {
		mSRLEIndex.reset();
		mSRLEPage.reset();
	}

	for (u32 x = 0; x < mFrameWidth; ++x)
		mpDecodeIndex[x] = mSRLEIndex.readU8();

	for (u32 y = 0; y < mFramePageHeight; ++y) {
		for (u32 x = 0; x < mFrameWidth; ++x) {
			if (mpDecodeIndex[x] & ZEN_BIT(y)) {
				const u8 page = mSRLEPage.readU8();
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mDecodeFrame, mpDecodeIndex);
}

void ConvertVideo::playDeltaRLEHC()
{
	if (mRLEHCIndex.isEmpty()) {
		mRLEHCIndex.reset();
		mRLEHCPage.reset();
	}

	for (u32 x = 0; x < mFrameWidth; ++x)
		mpDecodeIndex[x] = mRLEHCIndex.readU8();

	for (u32 y = 0; y < mFramePageHeight; ++y) {
		for (u32 x = 0; x < mFrameWidth; ++x) {
			if (mpDecodeIndex[x] & ZEN_BIT(y)) {
				const u8 page = mRLEHCPage.readU8();
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mDecodeFrame, mpDecodeIndex);
}

void ConvertVideo::playDeltaRLEHC2()
{
	if (mRLEHC2Index.isEmpty()) {
		mRLEHC2Index.reset();
		mRLEHC2Page.reset();
	}

	for (u32 x = 0; x < mFrameWidth; ++x)
		mpDecodeIndex[x] = mRLEHC2Index.readU8();

	for (u32 y = 0; y < mFramePageHeight; ++y) {
		for (u32 x = 0; x < mFrameWidth; ++x) {
			if (mpDecodeIndex[x] & ZEN_BIT(y)) {
				const u8 page = mRLEHC2Page.readU8();
				mDecodeFrame.data[y * mFrameWidth + x] = page;
			}
		}
	}

	SAFE_RELEASE(mpBitmap);
	mpBitmap = createBitmapFromFrame(mDecodeFrame, mpDecodeIndex);
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

				for (u32 j = 0; j < d; ++j)
					pData[i + j] = p;
				//pData[i] = 0;
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
		const u32 frameSize = (w * h) >> 3;
		u8* frame = new u8[frameSize];
		const u32 pageH = h >> 3;
		for (u32 y = 0; y < pageH; ++y) {
			for (u32 x = 0; x < w; ++x) {
				const u32 i = (y << 3) * s + x * d;
#if 1
				frame[y * w + x] =
					  (pData[i        ] ? ZEN_BIT(0) : 0)
					| (pData[i + s    ] ? ZEN_BIT(1) : 0)
					| (pData[i + s * 2] ? ZEN_BIT(2) : 0)
					| (pData[i + s * 3] ? ZEN_BIT(3) : 0)
					| (pData[i + s * 4] ? ZEN_BIT(4) : 0)
					| (pData[i + s * 5] ? ZEN_BIT(5) : 0)
					| (pData[i + s * 6] ? ZEN_BIT(6) : 0)
					| (pData[i + s * 7] ? ZEN_BIT(7) : 0);
#else
				for (u32 j = 0; j < 8; ++j)
					if (pData[i + sy * j]) frame[y * w + x] |= ZEN_BIT(j);
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
