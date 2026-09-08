#include "Buffer.h"

#include <fstream>
#include <format>
#include <stdexcept>

using namespace Util::Serialisation;

/* =================== ReadBuffer =================== */

class membuf : public std::streambuf {
protected:
	std::unique_ptr<char> data;
	size_t size;

public:
	membuf(std::unique_ptr<char> data, size_t size) : data(std::move(data)), size(size) {
		char* start = const_cast<char*>(this->data.get());
		this->setg(start, start, start + size);
	}

protected:
	virtual std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override {
		char* start = const_cast<char*>(this->data.get());
		char* end = start + size;
		char* newpos = nullptr;

		if (way == std::ios_base::beg) {
			newpos = start + off;
		} else if (way == std::ios_base::cur) {
			newpos = gptr() + off;
		} else if (way == std::ios_base::end) {
			newpos = end + off;
		}

		if (newpos >= start && newpos <= end) {
			setg(start, newpos, end);
			return newpos - start;
		}
		return std::streampos(-1);
	}

	virtual std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in) override {
		return seekoff(sp, std::ios_base::beg, which);
	}
};

class memory_istream : public std::istream {
	membuf buffer;
public:
	memory_istream(std::unique_ptr<char> data, size_t size)
		: buffer(std::move(data), size), std::istream(&buffer) {}
};

ReadBuffer::ReadBuffer(const std::filesystem::path& filename) {
	memoryIsLittleEndian = getIsLittleEndian();

	auto file = std::make_unique<std::ifstream>(filename, std::ios::binary | std::ios::ate);
	if (!file->is_open()) {
		throw std::runtime_error(std::format("could not open {}", filename.string()));
	}
	stream = std::move(file);

	totalSize = stream->tellg();
	stream->seekg(0, std::ios::beg);
}

ReadBuffer::ReadBuffer(std::unique_ptr<char> data, size_t size) {
	memoryIsLittleEndian = getIsLittleEndian();
	totalSize = size;
	stream = std::make_unique<memory_istream>(std::move(data), size);
	stream->seekg(0, std::ios::beg);
}

ReadBuffer::ReadBuffer(Buffer&& buffer) {
	memoryIsLittleEndian = getIsLittleEndian();
	totalSize = buffer.getSize();
	stream = std::make_unique<memory_istream>(std::move(buffer.data), buffer.getSize());
	stream->seekg(0, std::ios::beg);
}

void ReadBuffer::skip(size_t length) {
	stream->seekg(length, std::ios::cur);
}

void ReadBuffer::swapBytes(char* memory, size_t length) {
	if (length == 1) return;
	if(length % 2 != 0) throw std::runtime_error{"Odd number of bytes"};

	for (int i = 0; i < length / 2; i++) {
		std::swap(memory[i], memory[(length - 1) - i]);
	}
}

bool ReadBuffer::getIsLittleEndian() {
	int num = 1;
	return (*(char*)&num == 1);
}

bool ReadBuffer::hasUnreadBytes() { 
	return stream->tellg() < totalSize; 
}

std::string ReadBuffer::readString(size_t length) {
	std::string value;
	std::unique_ptr<char[]> buffer(new char[length]);
	stream->read(buffer.get(), length);
	if (stream->fail()) {
		throw ::std::runtime_error("file did not contain enough bytes");
	}
	value.assign(buffer.get(), length);
	return value;
}

/* =================== Buffer =================== */

Buffer::Buffer(size_t size) : data(new char[size]), at(0), writtenSize(0), totalSize(size) {
	memoryIsLittleEndian = getIsLittleEndian();
	memset(data.get(), 0, size);
}

Buffer::Buffer(std::unique_ptr<char> data, size_t size) : data(std::move(data)), at(0), totalSize(size), writtenSize(size) {
	memoryIsLittleEndian = getIsLittleEndian();
}

Buffer::Buffer(const std::filesystem::path& path) : at(0) {
	memoryIsLittleEndian = getIsLittleEndian();

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw std::runtime_error(std::format("could not open {}", path.string()));
	}

	totalSize = file.tellg();
	writtenSize = totalSize;
	file.seekg(0, std::ios::beg);
	data = std::unique_ptr<char>(new char[totalSize]);
	file.read(data.get(), totalSize);
	file.close();
}

void Buffer::writeToFile(const std::filesystem::path& filename) {
	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error(std::format("could not open {}", filename.string()));
	}

	file.write(data.get(), writtenSize);
	file.close();
}

bool Buffer::getIsLittleEndian() {
	int num = 1;
	return (*(char*)&num == 1);
}

void Buffer::swapBytes(char* memory, size_t length) {
	if (length == 1) return;
	if(length % 2 != 0) throw std::runtime_error{ "Odd number of bytes"};

	for (int i = 0; i < length / 2; i++) {
		std::swap(memory[i], memory[(length - 1) - i]);
	}
}

void Buffer::expand() {
	totalSize = totalSize * 2;
	char* newBuffer = new char[totalSize];
	memset(newBuffer, 0, totalSize);
	memcpy(newBuffer, data.get(), writtenSize);
	data = std::unique_ptr<char>(newBuffer);
}

std::string Buffer::readString(size_t length) {
	std::string value = std::string(data.get() + at, length);
	at += length;
	return value;
}

void Buffer::writeString(const std::string& value) {
	writeBytes(value.c_str(), value.size());
}

void Buffer::writeBytes(const char* bytes, size_t length) {
	while (at + length > totalSize) {
		expand();
	}
	memcpy(data.get() + at, bytes, length);
	at += length;
	writtenSize = std::max(writtenSize, at);
}

void Buffer::writeBytes(const unsigned char* bytes, size_t length) {
	writeBytes(reinterpret_cast<const char*>(bytes), length);
}

void Buffer::setPosition(size_t position) { 
	at = position; 
	writtenSize = std::max(writtenSize, at);
}