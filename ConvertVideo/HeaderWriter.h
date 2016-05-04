#pragma once

namespace zen
{

	class HeaderWriter
	{
	public:
		HeaderWriter();

		HeaderWriter(const string &path);

		~HeaderWriter();

		void open(const string &path);

		void close();

		void newLine();

		void writeComment(const string &comment);

		void write(const string &str);

		void writeU8(const string &name, u8 value);

		void writeU32(const string &name, u32 value);

		void writeU8Array(const string &name, const u8* value, u32 count);

	protected:
		static const u32 NAME_WIDTH = 32;
		static const u32 NUM_PER_LINE = 16;

		ofstream mStream;
	};

}