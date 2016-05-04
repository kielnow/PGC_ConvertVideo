#pragma once

#include "Stream.h"

namespace app
{

	using namespace zen;

	//-----------------------------------------------------------------------------
	// Run Length Encoding
	//-----------------------------------------------------------------------------
	template<typename T>
	void compressRLE(vector<T> &dst, const vector<T> &src)
	{
		if (!src.size())
			return;

		dst.clear();

		T data = src[0];
		u32 length = 1;
		for (u32 i = 1; i <= src.size(); ++i)
		{
			if (i != src.size() && data == src[i])
			{
				++length;
			}
			else
			{
				dst.push_back(data);
				if (length > 1) {
					dst.push_back(data);
					length -= 2;
					do {
						const u8 flag = length > 0x7F ? 0x80 : 0x00;
						dst.push_back((length & 0x7F) | flag);
						length >>= 7;
					} while (length);
				}
				if (i != src.size()) {
					data = src[i];
					length = 1;
				}
			}
		}
	}

#if 0
	class RLEDecoder
	{
	public:
		RLEDecoder(vector<u8>* pv = nullptr) : mpVector(pv) { clear(); }

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
#endif

	class RLEDecodeStream : public StreamT
	{
	public:
		RLEDecodeStream()
		{
			reset();
		}

	protected:
		virtual void evReset() override;

		virtual u8 evReadU8() override;

	protected:
		s32 mNum;
		s32 mPrev;
		bool mContinue;
	};

	//-----------------------------------------------------------------------------
	// Switched Run Length Encoding
	//-----------------------------------------------------------------------------
	template<typename T>
	void compressSRLE(vector<T> &dst, const vector<T> &src)
	{
		if (!src.size())
			return;

		dst.clear();

		bool s = true;
		vector<T> temp;
		temp.push_back(src[0]);
		for (u32 i = 1; i <= src.size(); ++i)
		{
			bool cond;
			if (i == src.size()) {
				cond = false;
			} else {
				cond = s ? temp[temp.size() - 1] != src[i] : temp[temp.size() - 1] == src[i];
			}

			if (cond) {
				temp.push_back(src[i]);
			} else {
				auto len = temp.size();
				do {
					const u8 flag = len > 0x7F ? 0x80 : 0x00;
					dst.push_back((len & 0x7F) | flag);
					len >>= 7;
				} while (len);
				if (s) {
					for (auto v : temp)
						dst.push_back(v);
				}
				temp.clear();
				if (i < src.size())
					temp.push_back(src[i]);
				s = !s;
			}
		}
	}

#if 0
	class SRLEDecoder
	{
	public:
		SRLEDecoder(vector<u8>* pv = nullptr) : mpVector(pv) { clear(); }

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
			mPrev = 0;
			mSwitch = false;
		}

	protected:
		vector<u8>* mpVector;
		u32 mPt;
		u32 mNum;
		u8 mPrev;
		bool mSwitch;
	};
#endif

	class SRLEDecodeStream : public StreamT
	{
	public:
		SRLEDecodeStream()
		{
			reset();
		}

	protected:
		virtual void evReset() override;

		virtual u8 evReadU8() override;

	protected:
		u32 mNum;
		u8 mPrev;
		bool mSwitch;
	};

	//-----------------------------------------------------------------------------
	// Burrows-Wheeler Transform
	//-----------------------------------------------------------------------------
	void convertBWT(vector<u8> &dst, vector<u16> &dst_n, const vector<u8> &src);

	//-----------------------------------------------------------------------------
	// Huffman Coding
	//-----------------------------------------------------------------------------
	struct HCData
	{
		u32				length;
		vector<u8>		data;
		vector<u8>		dict;

		void clear()
		{
			length = 0;
			data.clear();
			dict.clear();
		}

		size_t size() const
		{
			return sizeof(length) + data.size() + dict.size();
		}
	};

	void convertHC(HCData &dst, const vector<u8> &src);

	class HCDecodeStream : public Stream
	{
	public:
		HCDecodeStream() : mpDict(nullptr) {}

		void set(HCData &hc)
		{
			mBitStream.set(&hc.data, hc.length);
			mpDict = &hc.dict;
		}

		virtual bool isValid() const override { return mBitStream.isValid() && mpDict != nullptr; }

		virtual bool isEmpty() const override { return mBitStream.isEmpty(); }

	protected:
		virtual void evReset() override
		{
			mBitStream.reset();
		}

		virtual u8 evReadU8() override;

	protected:
		BitStream mBitStream;
		vector<u8>* mpDict;
	};




	struct HCData2
	{
		u32				length;
		vector<u8>		data;
		vector<u8>		dict;
		vector<u8>		nsize;
		vector<u16>		slength;

		void clear()
		{
			length = 0;
			data.clear();
			dict.clear();
		}

		size_t size() const
		{
			return sizeof(length) + data.size() + dict.size() + nsize.size() + sizeof(u16) * slength.size();
		}
	};
	
	void convertHC2(HCData2 &dst, const vector<u8> &src);

	class RLEHC2DecodeStream : public Stream
	{
	public:
		RLEHC2DecodeStream() : mpData(nullptr)
		{
			reset();
		}

		void setData(HCData2* pdata)
		{
			mBitStream.set(&pdata->data, pdata->length);
			mpData = pdata;
		}

		virtual bool isValid() const override { return mBitStream.isValid() && mpData; }

		virtual bool isEmpty() const override { return mBitStream.isEmpty(); }

	protected:
		virtual void evReset() override;

		virtual u8 evReadU8() override;

	protected:
		BitStream	mBitStream;
		HCData2*	mpData;
		u32			mDictIndex;
		u32			mDictPt;
		u32			mSegmentPt;
		u32			mSegmentIndex;
		s32			mNum;
		s32			mPrev;
		bool		mContinue;
	};

}