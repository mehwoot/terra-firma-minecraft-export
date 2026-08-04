#pragma once

#include <memory>
#include <istream>
#include <filesystem>
#include <stdexcept>

class Buffer;

namespace Util {
	namespace Serialisation {
		class ReadBuffer {
		protected:
			std::unique_ptr<std::istream> stream = nullptr;
			size_t totalSize = 0;
			bool memoryIsLittleEndian = false;

			static bool getIsLittleEndian();
			static void swapBytes(char* memory, size_t length);

		public:
			ReadBuffer(const std::filesystem::path& filename);
			ReadBuffer(std::unique_ptr<char> data, size_t size);
			ReadBuffer(Buffer&& buffer);

			void skip(size_t length);
			bool hasUnreadBytes();
			std::string readString(size_t length);

			template<class T>
			T read(bool streamIsLittleEndian = false) {
				constexpr size_t size = sizeof(T);

				char memory[size];
				stream->read(memory, size);
				if (stream->fail()) {
					throw ::std::runtime_error("file did not contain enough bytes");
				}

				if (streamIsLittleEndian != memoryIsLittleEndian) {
					swapBytes(memory, size);
				}

				return *reinterpret_cast<T*>(memory);
			}
		};
	}
}

class Buffer {
protected:
	std::unique_ptr<char> data;
	size_t at, totalSize, writtenSize;
	bool memoryIsLittleEndian;

	bool getIsLittleEndian();
	void swapBytes(char* memory, size_t length);
	void expand();

public:
	Buffer(size_t size);
	Buffer(std::unique_ptr<char> data, size_t size);
	Buffer(const std::filesystem::path& file);

	template<class T>
	void write(T value, bool bufferIsLittleEndian = false) {
		constexpr size_t size = sizeof(T);

		if (at + size > totalSize) {
			expand();
		}

		if (bufferIsLittleEndian != memoryIsLittleEndian) {
			char memory[size];
			memcpy(memory, reinterpret_cast<char*>(&value), size);
			swapBytes(memory, size);
			memcpy(data.get() + at, memory, size);
		} else {
			memcpy(data.get() + at, reinterpret_cast<char*>(&value), size);
		}

		at += size;
		writtenSize = std::max(writtenSize, at);
	}
	void writeString(const std::string& value);
	void writeBytes(const char* bytes, size_t length);
	void writeBytes(const unsigned char* bytes, size_t length);

	template<class T>
	T read(bool bufferIsLittleEndian = false) {
		constexpr size_t size = sizeof(T);

		if (bufferIsLittleEndian != memoryIsLittleEndian) {
			char memory[size];
			memcpy(memory, data.get() + at, size);
			swapBytes(memory, size);
			T value = *reinterpret_cast<T*>(memory);
			at += size;
			return value;
		} else {
			T value = *reinterpret_cast<T*>(data.get() + at);
			at += size;
			return value;
		}
	}
	/* These methods are an absolute mess and should be cleaned up */
	std::string readString(size_t length);
	char* getCurrentPosition() const { return data.get() + at; }
	size_t getAt() const { return at; }
	void incrementPosition(size_t length) { at += length; }
	void setPosition(size_t position);
	size_t getWrittenSize() const { return writtenSize; }
	bool hasUnreadBytes() { return at < writtenSize; }
	void rewind() { at = 0; }
	void writeToFile(const std::filesystem::path& path);
	void setWrittenSize(size_t writtenSize) { this->writtenSize = writtenSize; }
	size_t getSize() const { return totalSize; }

	friend class Util::Serialisation::ReadBuffer;
};