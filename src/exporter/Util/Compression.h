#pragma once
#include "Buffer.h"
#include <stdexcept>

namespace Util {
	namespace Serialisation {
		struct DecompressionException : public std::runtime_error {
			DecompressionException() : std::runtime_error("could not decompress") {}
			DecompressionException(const std::string& message) : std::runtime_error(message) {}
		};

		struct CompressionException : public std::runtime_error {
			CompressionException() : std::runtime_error("could not compress") {}
			CompressionException(const std::string& message) : std::runtime_error(message) {}
		};

		struct Data {
			std::unique_ptr<unsigned char[]> data;
			unsigned long size;
		};

		Data compress(const unsigned char* data, size_t length);
		Data compress(const char* data, size_t length);
		Data compress(const Data& input);
		Data compress(std::istream& stream);
		Data decompress(const unsigned char* data, size_t length);
		Data decompress(const char* data, size_t length);
		Data decompress(const Data& input);
		Data decompress(std::istream& stream);
		Buffer gzipCompress(Buffer& input);
		Buffer gzipDecompress(Buffer& input);
	}
}