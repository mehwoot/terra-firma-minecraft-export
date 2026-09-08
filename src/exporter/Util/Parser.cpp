#include "Parser.h"

#include "mc-export-plugin-module.h"

#include <vector>
#include <iostream>
#include <format>

using namespace Util;
using namespace Util::Serialisation;
using namespace Util::Serialisation::NBT;

std::vector<TagUPtr> Parser::parse(::Util::Serialisation::ReadBuffer& buffer, Options options) {
	std::vector<TagUPtr> values;

	while (buffer.hasUnreadBytes() && !finished) {
		int8_t tagType = buffer.read<int8_t>();
		if (tagType == 0) {
			return values;
		}
		uint16_t nameLength = buffer.read<uint16_t>();
		std::string name = buffer.readString(nameLength);

		auto result = parsePayload(buffer, tagType, name, options);
		if (result) values.push_back(std::move(result));
	}

	return values;
}

void Parser::printIndent() {
	for (int i = 0; i < indent; i++) {
		std::cout << " ";
	}
}

TagUPtr Parser::parsePayload(::Util::Serialisation::ReadBuffer& buffer, int8_t tagType, const std::string& name, Options options) {
	if (options.endTag && options.endTag.value() == name) {
		finished = true;
		return nullptr;
	}

	switch (tagType) {
	case 0:
		return std::make_unique<TagEnd>();
	case 1: {
		int8_t value = buffer.read<int8_t>();
		if (debugOutput) { printIndent(); std::cout << "byte " << name << "=" << (int)value << std::endl; }
		return std::make_unique<TagByte>(value, name);
	}
	case 2: {
		int16_t value = buffer.read<int16_t>();
		if (debugOutput) { printIndent(); std::cout << "short " << name << "=" << value << std::endl; }
		return std::make_unique<TagShort>(value, name);
	}
	case 3: {
		int32_t value = buffer.read<int32_t>();
		if (debugOutput) { printIndent(); std::cout << "int " << name << "=" << value << std::endl; }
		return std::make_unique<TagInt>(value, name);
	}
	case 4: {
		int64_t value = buffer.read<int64_t>();
		if (debugOutput) { printIndent(); std::cout << "long " << name << "=" << value << std::endl; }
		return std::make_unique<TagLong>(value, name);
	}
	case 5: {
		float value = buffer.read<float>();
		if (debugOutput) { printIndent(); std::cout << "float " << name << "=" << value << std::endl; }
		return std::make_unique<TagFloat>(value, name);
	}
	case 6: {
		double value = buffer.read<double>();
		if (debugOutput) { printIndent(); std::cout << "double " << name << "=" << value << std::endl; }
		return std::make_unique<TagDouble>(value, name);
	}
	case 7: {
		int32_t size = buffer.read<int32_t>();
		if (debugOutput) { printIndent(); std::cout << "byte array name=" << name << " size=" << size << std::endl; }

		std::vector<uint8_t> values;
		values.reserve(size);
		for (int i = 0; i < size; i++) {
			values.push_back(buffer.read<uint8_t>());
		}

		return std::make_unique<TagByteArray>(std::move(values), name);
	}
	case 8: {
		uint16_t length = buffer.read<uint16_t>();
		std::string value = buffer.readString(length);
		if (debugOutput) { printIndent(); std::cout << "string " << name << "=" << value << std::endl; }
		return std::make_unique<TagString>(value, name);
	}
	case 9: {
		int8_t valuesTagType = buffer.read<int8_t>();
		int32_t size = buffer.read<int32_t>();
		if (debugOutput) { printIndent(); std::cout << "taglist name=" << name << " size=" << size << " type=" << (int)valuesTagType << std::endl; }
		indent++;
		std::vector<TagUPtr> values;
		values.reserve(size);
		for (int i = 0; i < size; i++) {
			auto result = parsePayload(buffer, valuesTagType, "", options);
			if (result) values.push_back(std::move(result));
		}
		indent--;
		return std::make_unique<TagList>(std::move(values), name, valuesTagType);
	}
	case 10: {
		if (debugOutput) { printIndent(); std::cout << "compound name=" << name << std::endl; }
		indent++;
		std::vector<TagUPtr> children = parse(buffer, options);
		indent--;
		return std::make_unique<TagCompound>(std::move(children), name);
	}
	case 11: {
		int32_t size = buffer.read<int32_t>();
		std::vector<int32_t> values;
		values.reserve(size);
		for (int i = 0; i < size; i++) {
			values.push_back(buffer.read<int32_t>());
		}
		if (debugOutput) { printIndent(); std::cout << "int array name=" << name << " size=" << size << std::endl; }
		return std::make_unique<TagIntArray>(std::move(values), name);
	}
	case 12: {
		int32_t size = buffer.read<int32_t>();
		std::vector<int64_t> values;
		values.reserve(size);
		for (int i = 0; i < size; i++) {
			values.push_back(buffer.read<int64_t>());
		}
		if (debugOutput) { printIndent(); std::cout << "long array name=" << name << " size=" << size << std::endl; }
		return std::make_unique<TagLongArray>(values, name);
	}
	default:
		reportFatalErrorC("not supported");
		return nullptr;
	}
}

void Util::Serialisation::saveTagToFile(Util::Serialisation::NBT::Tag& tag, const std::filesystem::path& path) {
	::Buffer buffer(4 * 1024 * 1024);
	tag.write(buffer);
	buffer.writeToFile(path);
}

Util::Serialisation::NBT::TagUPtr Util::Serialisation::loadTagFromFile(const std::filesystem::path& path) {
	::Util::Serialisation::ReadBuffer buffer(path);
	auto tags = NBT::Parser().parse(buffer);
	if (tags.size() != 1) {
		reportFatalErrorC(std::format("Parsing NBT file failed- should have one root tag. file: {}", path.string()).c_str());
	}

	return std::move(tags.front());
}