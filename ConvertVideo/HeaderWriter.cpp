#include "stdafx.h"
#include "HeaderWriter.h"

using namespace zen;

HeaderWriter::HeaderWriter()
{
}

HeaderWriter::HeaderWriter(const string &path)
{
	open(path);
}

HeaderWriter::~HeaderWriter()
{
	close();
}

void HeaderWriter::open(const string &path)
{
	mStream.open(path.c_str());
	mStream << "#pragma once" << endl;
	mStream << endl;
}

void HeaderWriter::close()
{
	mStream.close();
}

void HeaderWriter::newLine()
{
	mStream << endl;
}

void HeaderWriter::writeComment(const string &comment)
{
	size_t bpos = 0;
	while (1) {
		const size_t epos = comment.find('\n', bpos);
		if (epos == string::npos) {
			mStream << "// " << comment.substr(bpos) << endl;
			break;
		}
		const size_t count = epos - bpos;
		mStream << "// " << comment.substr(bpos, count) << endl;
		bpos = epos + 1;
	}
}

void HeaderWriter::write(const string &str)
{
	size_t bpos = 0;
	while (1) {
		const size_t epos = str.find('\n', bpos);
		if (epos == string::npos) {
			mStream << str.substr(bpos) << endl;
			break;
		}
		const size_t count = epos - bpos;
		mStream << str.substr(bpos, count) << endl;
		bpos = epos + 1;
	}
}

void HeaderWriter::writeU8(const string &name, u8 value)
{
	mStream << "const u8 " << name << " = " << static_cast<u32>(value) << ";" << endl;
	mStream << endl;
}

void HeaderWriter::writeU32(const string &name, u32 value)
{
	mStream << "const u32 " << name << " = " << value << ";" << endl;
	mStream << endl;
}

void HeaderWriter::writeU8Array(const string &name, const u8* value, u32 count)
{
	if (!count)
		return;

	const u32 lnum = (count + (NUM_PER_LINE - 1)) / NUM_PER_LINE;

	mStream << "const u8 " << name << "[" << count << "] = { ";
	if (lnum > 1)
		mStream << endl << '\t';
	for (auto y = 0u; y < lnum; ++y) {
		for (auto x = 0u; x < NUM_PER_LINE; ++x) {
			const u32 i = y * NUM_PER_LINE + x;
			if (i == count)
				break;
			mStream << setw(3) << static_cast<u32>(value[i]) << ", ";
		}
		if (y == lnum - 1)
			mStream << "};" << endl;
		else
			mStream << endl << '\t';
	}
	mStream << endl;
}
