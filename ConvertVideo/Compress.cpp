#include "stdafx.h"
#include "Compress.h"

using namespace app;

namespace {

	struct Word {
		u8		value;
		u32		count;
		u32		length;

		Word(u8 v = 0) : value(v), count(0), length(0) {}
	};

	void convertHC2_aux(HCData2 &dst, vector<Word> &words, u16 &slength, u32 &mn)
	{
		u8 nsize = 0;
		while (mn >= (u32)ZEN_BIT(1 + nsize++));
		dst.nsize.push_back(nsize);

		sort(words.begin(), words.end(), [](const Word &x, const Word &y) { return x.count > y.count; });
#if 1
		u8 mv1 = 0, mv2 = 0;
		for (auto j = 0; j < 7; ++j)	mv1 = max(mv1, words[j].value);
		for (auto j = 7; j < words.size(); ++j)	mv2 = max(mv2, words[j].value);
		u8 mv = max(mv1, mv2);
		u8 vsize = 0;
		while (mv >= (u32)ZEN_BIT(1 + vsize++));
#endif
		for (u32 j = 0; j < 7; ++j)	dst.dict.push_back(words[j].value);

		dst.slength.push_back(slength);

		//ZEN_TRACE(L"slength = %4d, nmax = %4d, nsize = %2d, vmax = %4d %4d, vsize = %2d, value = ", slength, mn, nsize, mv1, mv2, vsize);
		ZEN_TRACE(L"slength = %4d, nmax = %4d, nsize = %2d, vsize = %2d, value = ", slength, mn, nsize, vsize);
		for (u32 j = 0; j < 7; ++j)	ZEN_TRACE(L"%2x ", words[j].value);
		ZEN_TRACE(L", count = ");
		for (u32 j = 0; j < 7; ++j)	ZEN_TRACE(L"%4d ", words[j].count);
		ZEN_TRACELINE(L"");

		slength = 0;
		mn = 0;
		words.clear();
	}

}

//-----------------------------------------------------------------------------
// Run Length Encoding
//-----------------------------------------------------------------------------
#if 0
u8 RLEDecoder::get()
{
	if (mContinue) {
		if (mNum == 0) {
			u8 n = 0, i = 0;
			do {
				n = (*mpVector)[mPt++];
				mNum |= (n & 0x7F) << (7 * i++);
			} while (n & 0x80);
		}

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
#endif

void RLEDecodeStream::evReset()
{
	StreamT::evReset();
	mNum = 0;
	mPrev = -1;
	mContinue = false;
}

u8 RLEDecodeStream::evReadU8()
{
	if (mContinue) {
		if (mNum == 0) {
			u8 n = 0, i = 0;
			do {
				n = mpStream->readU8();
				mNum |= (n & 0x7F) << (7 * i++);
			} while (n & 0x80);
		}
		if (mNum > 0) {
			const u8 prev = static_cast<u8>(mPrev);
			if (--mNum <= 0) {
				mPrev = -1;
				mContinue = false;
			}
			return prev;
		}
	}
	const u8 curr = mpStream->readU8();
	mContinue = mPrev == static_cast<s32>(curr);
	return mPrev = curr;
}

//-----------------------------------------------------------------------------
// Switched Run Length Encoding
//-----------------------------------------------------------------------------
#if 0
u8 SRLEDecoder::get()
{
	if (mNum) {
		if (mSwitch)
			mPrev = (*mpVector)[mPt++];
		--mNum;
		return mPrev;
	} else {
		mSwitch = !mSwitch;
		u8 n = 0, i = 0;
		do {
			n = (*mpVector)[mPt++];
			mNum |= (n & 0x7F) << (7 * i++);
		} while (n & 0x80);

		if (mSwitch)
			mPrev = (*mpVector)[mPt++];
		--mNum;
		return mPrev;
	}
}
#endif

void SRLEDecodeStream::evReset()
{
	StreamT::evReset();
	mNum = 0;
	mPrev = 0;
	mSwitch = false;
}

u8 SRLEDecodeStream::evReadU8()
{
	if (mNum) {
		if (mSwitch)
			mPrev = mpStream->readU8();
		--mNum;
		return mPrev;
	} else {
		mSwitch = !mSwitch;
		u8 n = 0, i = 0;
		do {
			n = mpStream->readU8();
			mNum |= (n & 0x7F) << (7 * i++);
		} while (n & 0x80);

		if (mSwitch)
			mPrev = mpStream->readU8();
		--mNum;
		return mPrev;
	}
}

//-----------------------------------------------------------------------------
// Burrows-Wheeler Transform
//-----------------------------------------------------------------------------
void app::convertBWT(vector<u8> &dst, vector<u16> &dst_n, const vector<u8> &src)
{
#if 0
	if (!src.size())
		return;

	dst.clear();
	dst_n.clear();

	const u32 BLOCK_SIZE = 1024;
	const u32 bnum = (src.size() + (BLOCK_SIZE - 1)) / BLOCK_SIZE;

	for (u32 i = 0; i < bnum; ++i) {
		vector<u8> bsrc;
		bsrc.reserve(BLOCK_SIZE << 1);
		for (u32 k = 0; k < 2; ++k) {
			for (u32 j = 0; j < BLOCK_SIZE; ++j) {
				const u32 index = i * BLOCK_SIZE + j;
				if (index >= src.size())
					break;
				bsrc.push_back(src[index]);
			}
		}
		const u32 bsize = bsrc.size() >> 1;

		vector< vector<u8> > table;
		table.reserve(bsize);
		for (u32 j = 0; j < bsize; ++j) {
			auto first = bsrc.cbegin() + j;
			auto last = bsrc.cbegin() + j + bsize;
			vector<u8> sv(first, last);
			table.push_back(sv);
		}

		sort(table.begin(), table.end());

		vector< vector<u8> > bwt;
		bwt.reserve(bsize);
		for (u32 j = 0; j < bsize; ++j) {
			const u8 suffix = table[j][bsize - 1];
			dst.push_back(suffix);
			//ZEN_TRACE(L"%d, ", suffix);
		}
		//ZEN_TRACE(L"\n");

		auto it = find(table.begin(), table.end(), vector<u8>(bsrc.cbegin(), bsrc.cbegin() + bsize));
		const u16 n = it - table.begin();
		dst_n.push_back(n);
		//ZEN_TRACELINE(L"n = %d", n);
	}
#endif
}

//-----------------------------------------------------------------------------
// Huffman Coding
//-----------------------------------------------------------------------------
void app::convertHC(HCData &dst, const vector<u8> &src)
{
	// 辞書を作成
	{
		vector<Word> words;
		words.reserve(0x100);
		for (u32 i = 0; i < 0x100; ++i)
			words.push_back(Word(i));
		for (auto v : src)
			words[v].count++;
		sort(words.begin(), words.end(), [](const Word &x, const Word &y) { return x.count > y.count; });
		u32 length = 1;
		for (auto &word : words) {
			if (!word.count)
				break;
			word.length = length++;
		}

		// 辞書を出力
		dst.dict.clear();
		for (u32 i = 0; i < 7; ++i) {
			dst.dict.push_back(words[i].value);
		}
	}

	// ハフマン符号化
	{
		dst.data.clear();
		BitStream bs(BitStream::MODE_WRITE);
		bs.set(&dst.data);
		for (u8 v : src) {
			// 辞書にあるか探す
			s32 dict = -1;
			for (s32 i = 0; i < 7; ++i) {
				if (v == dst.dict[i]) {
					dict = i;
					break;
				}
			}

			if (dict >= 0) {
				// 辞書にある場合はハフマン符号
				bs.writeBit(1);
				for (s32 i = 0; i < dict; ++i)
					bs.writeBit(1);
				bs.writeBit(0);
			} else {
				// 辞書にない場合は直値
				bs.writeBit(0);
				for (u32 i = 0; i < 8; ++i)
					bs.writeBit((v >> (7 - i)) & 1);
			}
		}
		bs.flush();
		dst.length = static_cast<u32>(bs.getLength());
	}
}

u8 HCDecodeStream::evReadU8()
{
	u8 v = 0;
	if (mBitStream.readBit()){
		// 辞書にある場合はハフマン符号
		u32 wlen = 0;
		while (mBitStream.readBit())
			wlen++;
		v = mpDict->at(wlen);
	} else {
		// 辞書にない場合は直値
		for (u32 i = 0; i < 8; ++i)
			v = (v << 1) | mBitStream.readBit();
	}
	return v;
}




void app::convertHC2(HCData2 &dst, const vector<u8> &src)
{
	// 辞書を作成
	{
		vector<Word> words;
		u16 slength = 0;
		u32 n = 0, i = 0, mn = 0;
		s32 prev = -1;
		bool skip = false;
		for (auto v : src) {
			if (skip) {
				n |= (v & 0x7F) << (7 * i++);
				if (!(v & 0x80)) {
					mn = max(mn, n);
					n = i = 0;
					prev = -1;
					skip = false;
					slength++;
				}
				continue;
			}

			if (prev == (s32)v)
				skip = true;
			prev = (s32)v;

			auto it = find_if(words.begin(), words.end(), [&](const Word &x) { return x.value == v; });
			if (it == words.end()) {
				const auto size = count_if(words.begin(), words.end(), [](const Word &x) { return x.count >= 16; });
				if (size >= 7) {
					convertHC2_aux(dst, words, slength, mn);
				}

				words.push_back(Word(v));
			} else {
				it->count++;
			}
			slength++;
		}

		for (auto j = words.size(); j < 7; ++j) {
			words.push_back(Word(0));
		}

		convertHC2_aux(dst, words, slength, mn);

		ZEN_TRACELINE(L"num = %4d, size = %8.2f KB", dst.slength.size(), (f32)dst.size() / 1024);
		ZEN_TRACELINE(L"--------------------------------------------");
	}

	// ハフマン符号化
	{
		dst.data.clear();
		BitStream bs(BitStream::MODE_WRITE);
		bs.set(&dst.data);
		u32 spt = 0;
		u32 sindex = 0;
		u32 n = 0, i = 0;
		s32 prev = -1;
		bool skip = false;
		for (auto v : src) {
			if (skip) {
				n |= (v & 0x7F) << (7 * i++);
				if (!(v & 0x80)) {
					const u32 nsize = dst.nsize[sindex];
					for (u32 j = 0; j < nsize; ++j)
						bs.writeBit((n >> (nsize - 1 - j)) & 1);

					n = i = 0;
					prev = -1;
					skip = false;
					if (++spt >= dst.slength[sindex]) {
						spt = 0;
						sindex++;
					}
				}
				continue;
			}

			if (prev == (s32)v)
				skip = true;
			prev = (s32)v;

			// 辞書にあるか探す
			s32 dict = -1;
			for (s32 j = 0; j < 7; ++j) {
				if (v == dst.dict[sindex * 7 + j]) {
					dict = j;
					break;
				}
			}

			if (dict >= 0) {
				// 辞書にある場合はハフマン符号
				bs.writeBit(1);
				for (s32 j = 0; j < dict; ++j)
					bs.writeBit(1);
				bs.writeBit(0);
			} else {
				// 辞書にない場合は直値
				bs.writeBit(0);
				for (u32 j = 0; j < 8; ++j)
					bs.writeBit((v >> (7 - j)) & 1);
			}

			if (++spt >= dst.slength[sindex]) {
				spt = 0;
				sindex++;
			}
		}
		bs.flush();
		dst.length = static_cast<u32>(bs.getLength());
	}
}

void RLEHC2DecodeStream::evReset()
{
	mBitStream.reset();
	mSegmentPt = mSegmentIndex = 0;
	mNum = 0;
	mPrev = -1;
	mContinue = false;
}

u8 RLEHC2DecodeStream::evReadU8()
{
	if (mContinue) {
		if (mNum == 0) {
			const u32 nsize = mpData->nsize[mSegmentIndex];
			for (u32 i = 0; i < nsize; ++i)
				mNum = (mNum << 1) | mBitStream.readBit();
			if (++mSegmentPt >= mpData->slength[mSegmentIndex]) {
				mSegmentPt = 0;
				mSegmentIndex++;
			}
		}
		if (mNum > 0) {
			const u8 prev = static_cast<u8>(mPrev);
			if (--mNum <= 0) {
				mPrev = -1;
				mContinue = false;
			}
			return prev;
		}
	}
	
	u8 curr = 0;
	if (mBitStream.readBit()) {
		// 辞書にある場合はハフマン符号
		u32 wlen = 0;
		while (mBitStream.readBit())
			wlen++;
		curr = mpData->dict[mSegmentIndex * 7 + wlen];
	} else {
		// 辞書にない場合は直値
		for (u32 i = 0; i < 8; ++i)
			curr = (curr << 1) | mBitStream.readBit();
	}
	mContinue = mPrev == static_cast<s32>(curr);
	if (++mSegmentPt >= mpData->slength[mSegmentIndex]) {
		mSegmentPt = 0;
		mSegmentIndex++;
	}
	return mPrev = curr;
}
