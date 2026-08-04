#include "Compression.h"
#include "miniz.h"
#include <cstdint>
#include <cstddef>
#include <memory>


namespace Util {
	namespace Serialisation {
		unsigned long calculateCrc(unsigned char* buf, size_t len) {
			/* from RFC 1952 */
			static unsigned long crcTable[256];
			static bool tableCalculated = false;

			if (!tableCalculated) {
				unsigned long c;
				int n, k;
				for (n = 0; n < 256; n++) {
					c = (unsigned long)n;
					for (k = 0; k < 8; k++) {
						if (c & 1) {
							c = 0xedb88320L ^ (c >> 1);
						} else {
							c = c >> 1;
						}
					}
					crcTable[n] = c;
				}
				tableCalculated = true;
			}

			unsigned long c = 0 ^ 0xffffffffL;
			for (int n = 0; n < len; n++) {
				c = crcTable[(c ^ buf[n]) & 0xff] ^ (c >> 8);
			}
			return c ^ 0xffffffffL;
		}

		Data compress(const unsigned char* data, size_t _length) {
			uLong length = static_cast<uLong>(_length);
			uLong compressedLength = compressBound(length) + 4;
			std::unique_ptr<unsigned char[]> compressedData = std::make_unique<unsigned char[]>(compressedLength);

			auto result = mz_compress(compressedData.get() + 4, &compressedLength, data, length);
			compressedLength += 4;
			int32_t originalLength = length;
			memcpy(compressedData.get(), &originalLength, 4);

			if (result != Z_OK) {
				throw CompressionException();
			}

			return { std::move(compressedData), compressedLength };
		}

		Data compress(const char* data, size_t length) {
			return compress(reinterpret_cast<const unsigned char*>(data), length);
		}

		Data compress(std::istream& stream) {
			stream.seekg(0, std::ios::end);
			unsigned long length = stream.tellg();
			stream.seekg(0, std::ios::beg);

			std::unique_ptr<unsigned char[]> fileContents = std::make_unique<unsigned char[]>(length);
			stream.read(reinterpret_cast<char*>(fileContents.get()), length);

			int charactersRead = stream.gcount();

			return compress(fileContents.get(), length);
		}

		Data compress(const Data& input) {
			return compress(input.data.get(), input.size);
		}

		Data decompress(const unsigned char* data, size_t _length) {
			uLong length = static_cast<uLong>(_length);
			int32_t inputUncompressedLength;
			memcpy(&inputUncompressedLength, data, 4);
			uLong uncompressedLength = inputUncompressedLength;
			std::unique_ptr<unsigned char[]> uncompressedData = std::make_unique<unsigned char[]>(uncompressedLength);

			auto result = mz_uncompress(uncompressedData.get(), &uncompressedLength, data + 4, length - 4);
			if (result != Z_OK) {
				throw DecompressionException();
			}

			return { std::move(uncompressedData), uncompressedLength };
		}

		Data decompress(const char* data, size_t length) {
			return decompress(reinterpret_cast<const unsigned char*>(data), length);
		}

		Data decompress(const Data& input) {
			return decompress(input.data.get(), input.size);
		}

		Data decompress(std::istream& stream) {
			stream.seekg(0, std::ios::end);
			unsigned long length = stream.tellg();
			stream.seekg(0, std::ios::beg);

			std::unique_ptr<unsigned char[]> streamContents = std::make_unique<unsigned char[]>(length);
			stream.read(reinterpret_cast<char*>(streamContents.get()), length);

			return decompress(streamContents.get(), length);
		}

		Buffer gzipCompress(Buffer& input) {
			Buffer output(4096);

			output.write<uint8_t>(0x1F);
			output.write<uint8_t>(0x8B);
			output.write<uint8_t>(0x08);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0x0);
			output.write<uint8_t>(0xFF);

			input.setPosition(0);
			uLong compressedLength = compressBound(static_cast<uLong>(input.getWrittenSize()));
			std::unique_ptr<unsigned char> compressedData = std::unique_ptr<unsigned char>(new unsigned char[compressedLength]);

			auto status = mz_compress_no_zlib_headers(compressedData.get(), &compressedLength, reinterpret_cast<unsigned char*>(input.getCurrentPosition()), static_cast<uLong>(input.getWrittenSize()));

			if (status != MZ_OK) {
				throw CompressionException();
			}

			output.writeBytes(compressedData.get(), compressedLength);

			uint32_t crc = calculateCrc(reinterpret_cast<unsigned char*>(input.getCurrentPosition()), input.getWrittenSize());
			output.write<uint32_t>(crc, true);
			output.write<uint32_t>(static_cast<uint32_t>(compressedLength), true);

			return output;
		}

		Buffer gzipDecompress(Buffer& input) {
			uint8_t headerBytes[10];
			for (int i = 0; i < 10; i++) {
				headerBytes[i] = input.read<uint8_t>();
			}

			if (headerBytes[0] != 0x1F || headerBytes[1] != 0x8B) {
				throw DecompressionException("wrong gzip signature");
			}

			size_t totalSize = input.getWrittenSize();
			input.setPosition(totalSize - 4);
			uLong uncompressedSize = input.read<uint32_t>(true);
			input.setPosition(totalSize - 8);
			uint32_t fileCrc = input.read<uint32_t>(true);
			uLong compressedSize = static_cast<uLong>(totalSize) - 18;
			Buffer output(uncompressedSize);
			input.setPosition(10);

			auto result = mz_uncompress_no_zlib_headers(
				reinterpret_cast<unsigned char*>(output.getCurrentPosition()), &uncompressedSize, 
				reinterpret_cast<const unsigned char*>(input.getCurrentPosition()), compressedSize
			);

			if (result != Z_OK) {
				throw DecompressionException();
			}

			output.setPosition(0);
			output.setWrittenSize(uncompressedSize);
			uint32_t calculatedCrc = calculateCrc(reinterpret_cast<unsigned char*>(output.getCurrentPosition()), output.getWrittenSize());

			if (calculatedCrc != fileCrc) {
				throw DecompressionException("CRC does not match");
			}

			return output;
		}
	}
}
