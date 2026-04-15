#include "stdafx.h"

#include <cstdio>

#include "ByteBuffer.h"
#include "ZoneIo.h"

ZoneIo::ZoneIo(std::FILE *channel, int64_t pos)
{
	this->channel = channel;
	this->pos = pos;
}

void ZoneIo::write(byteArray bb, int size)
{
    ByteBuffer *buff = ByteBuffer::wrap(bb);
//    if (bb.length != size) throw new IllegalArgumentException("Expected " + size + " bytes, got " + bb.length);	// 4J - TODO
    buff->order(ZonedChunkStorage::BYTEORDER);
    buff->position(bb.length);
    buff->flip();
    write(buff, size);
	delete buff;
}

void ZoneIo::write(ByteBuffer *bb, int size)
{
	if (channel == nullptr)
	{
		return;
	}

	if (std::fseek(channel, static_cast<long>(pos), SEEK_SET) != 0)
	{
		return;
	}

	std::fwrite(bb->getBuffer(), 1, bb->getSize(), channel);
    pos += size;
}

ByteBuffer *ZoneIo::read(int size)
{
    byteArray bb = byteArray(size);
	if (channel != nullptr)
	{
		std::fseek(channel, static_cast<long>(pos), SEEK_SET);
	}
    ByteBuffer *buff = ByteBuffer::wrap(bb);
	// 4J - to investigate - why is this buffer flipped before anything goes in it?
    buff->order(ZonedChunkStorage::BYTEORDER);
    buff->position(size);
    buff->flip();
	if (channel != nullptr)
	{
		std::fread(buff->getBuffer(), 1, buff->getSize(), channel);
	}
    pos += size;
    return buff;
}

void ZoneIo::flush()
{
	if (channel != nullptr)
	{
		std::fflush(channel);
	}
}
