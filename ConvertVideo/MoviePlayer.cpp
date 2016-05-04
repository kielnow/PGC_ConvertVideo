#include "stdafx.h"
#include "MoviePlayer.h"

using namespace zen;

namespace {

	RECT correctAspectRatio(const RECT& src, const MFRatio& par)
	{
		RECT rc = { 0, 0, src.right - src.left, src.bottom - src.top };

		if ((par.Numerator != 1) || (par.Denominator != 1))
		{
			if (par.Numerator > par.Denominator)
			{
				rc.right = MulDiv(rc.right, par.Numerator, par.Denominator);
			}
			else if (par.Numerator < par.Denominator)
			{
				rc.bottom = MulDiv(rc.bottom, par.Denominator, par.Numerator);
			}
		}
		return rc;
	}

	template<typename T>
	T clamp(T x, T min, T max)
	{
		if (x < min)
			return min;
		else if (x > max)
			return max;
		else
			return x;
	}

	enum {
		LCD_WIDTH		= 128,
		LCD_HEIGHT		= 64,
		LCD_PAGE_SIZE	= 8,
	};

}

void MoviePlayer::initialize()
{
	CHECK_HRESULT(MFStartup(MF_VERSION));
}

void MoviePlayer::finalize()
{
	MFShutdown();
}

MoviePlayer::MoviePlayer()
	: mpReader(nullptr)
	, mCurrentPosition(0)
	, mDestWidth(80), mDestHeight(64)
	, mThresholdMin(32), mThresholdMax(192)
	, mpBitmapWIC(nullptr)
{
}

MoviePlayer::~MoviePlayer()
{
}

bool MoviePlayer::open(ZEN_CWSTR path)
{
	HRESULT hr = S_OK;
	IMFAttributes* pAttributes = nullptr;

	SAFE_RELEASE(mpReader);

	CHECK_HRESULT(MFCreateAttributes(&pAttributes, 1));
	CHECK_HRESULT(pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE));

	if (FAILED(MFCreateSourceReaderFromURL(path, pAttributes, &mpReader)))
		return false;

	if (!selectVideoStream())
		return false;

	if (!getVideoInfo(mVideoInfo))
		return false;

	mCurrentPosition = 0;

	return true;
}

bool MoviePlayer::selectVideoStream()
{
	ComPtr<IMFMediaType> pType = nullptr;

	CHECK_HRESULT(MFCreateMediaType(&pType));
	CHECK_HRESULT(pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HRESULT(pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));

	if (FAILED(mpReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType.Get())))
		return false;

	if (FAILED(mpReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE)))
		return false;

	return true;
}

bool MoviePlayer::getVideoInfo(VideoInfo& dst)
{
	ComPtr<IMFMediaType> pType = nullptr;

	if (FAILED(mpReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType)))
		return false;

	u32 width, height;
	if (FAILED(MFGetAttributeSize(pType.Get(), MF_MT_FRAME_SIZE, &width, &height)))
		return false;

	s32 stride = (s32)MFGetAttributeUINT32(pType.Get(), MF_MT_DEFAULT_STRIDE, 1);

	RECT rcImage;
	MFRatio par = { 0 };
	HRESULT hr = MFGetAttributeRatio(pType.Get(), MF_MT_PIXEL_ASPECT_RATIO, (u32*)&par.Numerator, (u32*)&par.Denominator);
	if (SUCCEEDED(hr) && par.Denominator != par.Numerator)
	{
		RECT rc = { 0, 0, (s32)width, (s32)height };
		rcImage = correctAspectRatio(rc, par);
	}
	else
	{
		SetRect(&rcImage, 0, 0, (s32)width, (s32)height);
	}

	PROPVARIANT var;
	PropVariantInit(&var);

	s64 duration = 0;
	if (SUCCEEDED(mpReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var)))
	{
		duration = var.hVal.QuadPart;
	}

	PropVariantClear(&var);

	bool seekable = false;
	if (SUCCEEDED(mpReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS, &var)))
	{
		u32 flags = 0;
		PropVariantToUInt32(var, &flags);

		seekable = (flags & MFMEDIASOURCE_CAN_SEEK) && !(flags & MFMEDIASOURCE_HAS_SLOW_SEEK);
	}

	MFRatio frameRate = {};
	MFGetAttributeRatio(pType.Get(), MF_MT_FRAME_RATE, (u32*)&frameRate.Numerator, (u32*)&frameRate.Denominator);
	BOOL fixedSizeSamples = MFGetAttributeUINT32(pType.Get(), MF_MT_FIXED_SIZE_SAMPLES, FALSE);
	u32 sampleSize = MFGetAttributeUINT32(pType.Get(), MF_MT_SAMPLE_SIZE, 0);

#if 0
	BOOL allSamplesIndependent = MFGetAttributeUINT32(pType.Get(), MF_MT_ALL_SAMPLES_INDEPENDENT, FALSE);
	u32 maxKeyframeSpacing = MFGetAttributeUINT32(pType.Get(), MF_MT_MAX_KEYFRAME_SPACING, 0);

	PropVariantClear(&var);

	u64 totalFileSize = 0;
	if (SUCCEEDED(mpReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_TOTAL_FILE_SIZE, &var)))
	{
		totalFileSize = var.hVal.QuadPart;
	}
#endif

#if 0
	ComPtr<IMFRateSupport> pRateSupport;
	mpReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&pRateSupport));
	f32 fastestRate = 0.f;
	pRateSupport->GetFastestRate(MFRATE_FORWARD, FALSE, &fastestRate);
	f32 slowestRate = 0.f;
	pRateSupport->GetSlowestRate(MFRATE_FORWARD, FALSE, &slowestRate);

	ComPtr<IMFRateControl> pRateControl;
	mpReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&pRateControl));
	BOOL thin = FALSE;
	f32 rate = 0.f;
	pRateControl->GetRate(&thin, &rate);
#endif

	dst.width = width;
	dst.height = height;
	dst.topDown = stride > 0;
	dst.rect = rcImage;
	dst.seekable = seekable;
	dst.duration = duration;
	dst.frameRate = (f32)frameRate.Numerator / frameRate.Denominator;
	dst.fixedSizeSamples = fixedSizeSamples == TRUE;
	dst.sampleSize = sampleSize;

	return true;
}

bool MoviePlayer::setCurrentPosition(s64 pos)
{
	if (!mpReader)
		return false;

	if (mVideoInfo.seekable)
	{
		PROPVARIANT var;
		PropVariantInit(&var);

		var.vt = VT_I8;
		var.hVal.QuadPart = pos;

		if (FAILED(mpReader->SetCurrentPosition(GUID_NULL, var)))
			return false;
	}

	return true;
}

ID2D1Bitmap* MoviePlayer::createBitmap(ID2D1RenderTarget* pRT, s64 pos)
{
	if (!mpReader)
		return nullptr;

	if (pos < mCurrentPosition || mCurrentPosition + 10000000 < pos)
	{
		setCurrentPosition(pos);
	}

	const u32 MAX_FRAMES_TO_SKIP = 512;
	const s64 SEEK_TOLERANCE = 1000;		// 0.1ms

	ComPtr<IMFSample> pSample = nullptr;
	u32 skipped = 0;
	while (1)
	{
		DWORD dwFlags;
		ComPtr<IMFSample> pSampleTemp = nullptr;

		if (FAILED(mpReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &dwFlags, nullptr, &pSampleTemp)))
			return nullptr;

		if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			break;

		if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			if (FAILED(getVideoInfo(mVideoInfo)))
				return nullptr;
		}

		if (!pSampleTemp)
			continue;

		pSample = pSampleTemp;

		LONGLONG timestamp = 0;
		if (SUCCEEDED(pSample->GetSampleTime(&timestamp)))
		{
			if (skipped < MAX_FRAMES_TO_SKIP && timestamp + SEEK_TOLERANCE < pos)
			{
				++skipped;
				continue;
			}
		}

		mCurrentPosition = timestamp;

		const f32 target = (f32)pos * 0.0001f;
		const f32 sample = (f32)timestamp * 0.0001f;
		ZEN_TRACELINE(L"TargetTime = %8.2f ms, SampleTime = %8.2f ms", target, sample);

		pos = timestamp;
		break;
	}

	ZEN_TRACELINE(L"Skipped = %d", skipped);

	if (!pSample)
		return nullptr;

	if (!mpBitmapWIC)
	{
		createBitmapRT();
	}
	else
	{
		if (!(mDestWidth == mpBitmapWIC->getWidth() && mDestHeight == mpBitmapWIC->getHeight()))
		{
			createBitmapRT();
		}
	}

	ComPtr<IMFMediaBuffer> pBuffer = nullptr;
	if (FAILED(pSample->ConvertToContiguousBuffer(&pBuffer)))
		return nullptr;

	u8* pBitmapData = nullptr;
	DWORD bitmapDataSize = 0;

	ID2D1Bitmap* pBitmap = nullptr;
	do {
		if (FAILED(pBuffer->Lock(&pBitmapData, nullptr, &bitmapDataSize)))
			break;

		const u32 pitch = 4 * mVideoInfo.width;

		ZEN_ASSERT(bitmapDataSize == pitch * mVideoInfo.height, L"Invalid data size.");

		HRESULT hr = mpBitmapRT->CreateBitmap(
			D2D1::SizeU(mVideoInfo.width, mVideoInfo.height),
			pBitmapData,
			pitch,
			D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&pBitmap);
		if (FAILED(hr))
			break;
	} while (0);

	if (pBitmapData)
		pBuffer->Unlock();

	// Binarization
	{
		mpBitmapRT->BeginDraw();
		mpBitmapRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
		D2D1_RECT_F rect = { 0, 0, (f32)mDestWidth, (f32)mDestHeight };
		mpBitmapRT->DrawBitmap(pBitmap, rect);
		mpBitmapRT->EndDraw();

		const u32 dataSize = mpBitmapWIC->getDataSize();
		const u32 depth = mpBitmapWIC->getDepth();
		if (u8* pData = mpBitmapWIC->map())
		{
			u32 h = 0, l = 0;
			for (u32 i = 0; i < dataSize; i += depth)
			{
#if 1
				f32 b = (f32)pData[i] / 255.f;
				f32 g = (f32)pData[i + 1] / 255.f;
				f32 r = (f32)pData[i + 2] / 255.f;
				f32 luma = 0.299f * r + 0.587f * g + 0.114f * b;
				u8 uluma = (u8)(luma * 255.f) & 0xFF;
				for (u32 j = 0; j < depth; ++j)
					pData[i + j] = uluma;
#endif
				if (pData[i] > 0x7F)
					++h;
				else
					++l;
			}

			const u8 threshold = clamp((u8)(255 * h / (h + l)), mThresholdMin, mThresholdMax);
			ZEN_TRACELINE(L"Threshold = %d", threshold);

			for (u32 i = 0; i < dataSize; ++i)
				pData[i] = pData[i] > threshold ? 0 : 0xFF;

			mpBitmapWIC->unmap();
		}
	}

	ID2D1Bitmap* pDest = nullptr;
	pRT->CreateBitmapFromWicBitmap(mpBitmapWIC->getHandle(), &pDest);

	SAFE_RELEASE(pBitmap);

	return pDest;
}

void MoviePlayer::setDestSize(u32 w, u32 h)
{
	if (mDestWidth != w || mDestHeight != h)
	{
		mDestWidth = w;
		mDestHeight = clamp((h / LCD_PAGE_SIZE) * LCD_PAGE_SIZE, 0u, (u32)LCD_HEIGHT);

		createBitmapRT();
	}
}

void MoviePlayer::createBitmapRT()
{
	SAFE_DELETE(mpBitmapWIC);
	mpBitmapWIC = new BitmapWIC(mDestWidth, mDestHeight);

	CHECK_HRESULT(IGraphics2D->getFactory()->CreateWicBitmapRenderTarget(
		mpBitmapWIC->getHandle(),
		D2D1::RenderTargetProperties(),
		&mpBitmapRT));
}
