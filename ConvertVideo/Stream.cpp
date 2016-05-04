#include "stdafx.h"
#include "Stream.h"

using namespace zen;

void Stream::setMode(MODE mode)
{
	mMode = mode;
	mAttr = 0;
	if (mMode & MODE_READ)
		mAttr |= ATTR_READABLE | ATTR_RESETABLE;
	if (mMode & MODE_WRITE)
		mAttr |= ATTR_WRITABLE;
}




void VectorStream::set(vector<u8>* pv)
{
	mpVector = pv;
	mPt = 0;
}

void VectorStream::evReset()
{
	mPt = 0;
}

u8 VectorStream::evReadU8()
{
	return mpVector->at(mPt++);
}


void VectorStream::evWriteU8(u8 v)
{
	mpVector->push_back(v);
	mPt++;
}

size_t VectorStream::evRead(u8* pdst, size_t size)
{
	size_t pt = 0;
	while (size--) {
		if (isEmpty())
			break;
		pdst[pt++] = evReadU8();
	}
	return pt;
}

size_t VectorStream::evWrite(const u8* psrc, size_t size)
{
	size_t pt = 0;
	while (size--) {
		evWriteU8(psrc[pt++]);
	}
	return pt;
}




BitStream::BitStream(MODE mode)
	: Stream(mode), mpVector(nullptr)
{
}

void BitStream::set(vector<u8>* pv, size_t len)
{
	mpVector = pv;
	mLength = min(mpVector->size() << 3, len);
	mPt = 0;
	mBitPt = 0;
	mValue = 0;
	reset();
}

void BitStream::evFlush()
{
	if (mBitPt > 0) {
		mpVector->push_back(mValue);
		mValue = 0;
		mBitPt = 0;
	}
}

void BitStream::evReset()
{
	mPt = 0;
	mBitPt = 8;
	mValue = 0;
}

u8 BitStream::readBit()
{
	if (!isReadable())
		return 0;

	if (mBitPt >= 8) {
		mValue = mpVector->at(mPt++);
		mBitPt = 0;
	}
	return (mValue & ZEN_BIT(mBitPt++)) ? 1 : 0;
}

void BitStream::writeBit(u8 v)
{
	if (!isWritable())
		return;

	if (v)
		mValue |= ZEN_BIT(mBitPt);
	if (++mBitPt >= 8) {
		mpVector->push_back(mValue);
		mValue = 0;
		mBitPt = 0;
	}
	mLength++;
	mPt++;
}
