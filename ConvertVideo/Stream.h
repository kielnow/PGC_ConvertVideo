#pragma once

namespace zen
{

	class Stream
	{
	public:
		enum MODE {
			MODE_READ		= 0x01,
			MODE_WRITE		= 0x02,
			//MODE_READWRITE	= 0x03,
		};

		enum ORIGIN {
			ORIGIN_CURRENT,
			ORIGIN_BEGIN,
			ORIGIN_END,
		};

		Stream(MODE mode = MODE_READ) { setMode(mode); }

		virtual ~Stream() { close(); }

		virtual size_t	getLength() const { return 0; }

		virtual bool	isValid() const { return false; }

		virtual bool	isEmpty() const { return true; }

		bool	isReadable() const { return (mAttr & ATTR_READABLE) ? true : false; }

		bool	isWritable() const { return (mAttr & ATTR_WRITABLE) ? true : false; }

		bool	isResetable() const { return (mAttr & ATTR_RESETABLE) ? true : false; }

		bool	isSeekable() const { return (mAttr & ATTR_SEEKABLE) ? true : false; }

		void	close() { evClose(); }

		void	flush() { if (isWritable()) evFlush(); }

		void	reset() { if (isResetable()) evReset(); }

		void	seek(ptrdiff_t pos, ORIGIN origin = ORIGIN_BEGIN) { if (isSeekable()) evSeek(); }

		u8		readU8() { if (isReadable()) return evReadU8(); else return 0; }

		void	writeU8(u8 v) { if (isWritable()) evWriteU8(v); }

		size_t	read(u8* pdst, size_t size) { if (isReadable()) return evRead(pdst, size); else return 0; }

		size_t	write(const u8* psrc, size_t size) { if (isWritable()) return evWrite(psrc, size); else return 0; }

	protected:
		enum ATTR {
			ATTR_READABLE		= 0x01,
			ATTR_WRITABLE		= 0x02,
			ATTR_RESETABLE		= 0x04,
			ATTR_SEEKABLE		= 0x08,
		};

		void setMode(MODE mode);

		virtual void	evClose() { flush(); }

		virtual void	evFlush() {}

		virtual void	evReset() { ZEN_DEBUG_BREAK(); }

		virtual void	evSeek() { ZEN_DEBUG_BREAK(); }

		virtual u8		evReadU8() { ZEN_DEBUG_BREAK(); return 0; }

		virtual void	evWriteU8(u8 v) { ZEN_DEBUG_BREAK(); }

		virtual size_t	evRead(u8* pdst, size_t size) { ZEN_DEBUG_BREAK(); return 0; }

		virtual size_t	evWrite(const u8* psrc, size_t size) { ZEN_DEBUG_BREAK(); return 0; }

	protected:
		MODE mMode;
		u32 mAttr;
	};




	// Stream transformer
	class StreamT : public Stream
	{
	public:
		StreamT(MODE mode = MODE_READ)
			: Stream(mode)
			, mpStream(nullptr)
		{
		}

		virtual ~StreamT()
		{
			if (mAutoDelete)
				SAFE_DELETE(mpStream);
		}

		Stream* getStream() const { return mpStream; }

		void setStream(Stream* ps, bool autoDelete = false)
		{
			mpStream = ps;
			mAutoDelete = autoDelete;
		}

		bool isAutoDelete() const { return mAutoDelete; }

		void setAutoDelete(bool b) { mAutoDelete = b; }

		virtual bool isValid() const override { return mpStream != nullptr; }

		virtual bool isEmpty() const override { return mpStream->isEmpty(); }

	protected:
		virtual void evReset() override
		{
			if (mpStream)
				mpStream->reset();
		}

	protected:
		Stream* mpStream;
		bool mAutoDelete;
	};




	class VectorStream : public Stream
	{
	public:
		VectorStream(vector<u8>* pv, MODE mode = MODE_READ)
			: Stream(mode), mpVector(nullptr)
		{
			set(pv);
		}

		void set(vector<u8>* pv);

		virtual size_t getLength() const override { return mpVector->size(); }

		virtual bool isValid() const override { return mpVector != nullptr; }

		virtual bool isEmpty() const override { return mPt >= getLength(); }

	protected:
		virtual void	evReset() override;

		virtual u8		evReadU8() override;

		virtual void	evWriteU8(u8 v) override;

		virtual size_t	evRead(u8* pdst, size_t size) override;

		virtual size_t	evWrite(const u8* psrc, size_t size) override;

	protected:
		vector<u8>* mpVector;
		size_t mPt;
	};




	class BitStream : public Stream
	{
	public:
		BitStream(MODE mode = MODE_READ);

		void set(vector<u8>* pv, size_t len = 0);

		virtual size_t getLength() const override { return mLength; }

		virtual bool isValid() const override { return mpVector != nullptr; }

		virtual bool isEmpty() const override { return (mPt << 3) + mBitPt >= mLength; }

		u8 readBit();

		void writeBit(u8 v);

	protected:
		virtual void	evFlush() override;

		virtual void	evReset() override;

		virtual u8		evReadU8() override { return readBit(); }

		virtual void	evWriteU8(u8 v) override { writeBit(v); }

	protected:
		vector<u8>* mpVector;
		size_t mLength;
		size_t mPt;
		u8 mBitPt;
		u8 mValue;
	};

}