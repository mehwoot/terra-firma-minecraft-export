#include "Tags.h"
#include <stdexcept>

using namespace Util;
using namespace Util::Serialisation;
using namespace Util::Serialisation::NBT;

int TagByte::ID = 1;
int TagShort::ID = 2;
int TagInt::ID = 3;
int TagLong::ID = 4;
int TagFloat::ID = 5;
int TagDouble::ID = 6;
int TagByteArray::ID = 7;
int TagString::ID = 8;
int TagList::ID = 9;
int TagCompound::ID = 10;
int TagIntArray::ID = 11;
int TagLongArray::ID = 12;

void Tag::write(Buffer& buffer) {
	buffer.write<int8_t>(id);
	buffer.write<int16_t>((int16_t)name.size());
	buffer.writeString(name);
	writePayload(buffer);
}

Tag* TagComposite::findChildByName(const std::string& name) const {
	for (auto& child : children) {
		if (child->name == name) {
			return child.get();
		}
	}
	return nullptr;
}

void TagByte::writePayload(Buffer& buffer) {
	buffer.write<int8_t>(value);
}

void TagEnd::write(Buffer& buffer) {
	buffer.write<int8_t>(id);
}

void TagEnd::writePayload(Buffer& buffer) {
}

void TagShort::writePayload(Buffer& buffer) {
	buffer.write<int16_t>(value);
}

void TagInt::writePayload(Buffer& buffer) {
	buffer.write<int32_t>(value);
}

void TagLong::writePayload(Buffer& buffer) {
	buffer.write<int64_t>(value);
}

void TagFloat::writePayload(Buffer& buffer) {
	buffer.write<float>(value);
}

void TagDouble::writePayload(Buffer& buffer) {
	buffer.write<double>(value);
}

void TagByteArray::writePayload(Buffer& buffer) {
	buffer.write<int32_t>((int32_t)values.size());
	for (int i = 0; i < values.size(); i++) {
		buffer.write<int8_t>(values[i]);
	}
}

void TagString::writePayload(Buffer& buffer) {
	buffer.write<uint16_t>((uint16_t)value.size());
	buffer.writeString(value);
}

TagList::TagList(const std::vector<std::string>& values, const std::string& name) : Tag(9, name), TagComposite(std::vector<TagUPtr>{}), valueId(8) {
	for (auto& value : values) {
		children.push_back(std::make_unique<TagString>(value, ""));
	}
}

TagList::TagList(const std::set<std::string>& values, const std::string& name) : Tag(9, name), TagComposite(std::vector<TagUPtr>{}), valueId(8) {
	for (auto& value : values) {
		children.push_back(std::make_unique<TagString>(value, ""));
	}
}

TagList::TagList(const std::set<int>& values, const std::string& name) : Tag(9, name), TagComposite({}), valueId(TagInt::ID) {
	for (auto& value : values) {
		children.push_back(std::make_unique<TagInt>(value, ""));
	}
}

std::vector<std::string> TagList::deserialiseStringList(const TagList& tagList) {
	std::vector<std::string> values;
	for (auto& child : tagList.children) {
		values.push_back(child->as<TagString>().value);
	}
	return values;
}

std::set<std::string> TagList::deserialiseStringSet(const TagList& tagList) {
	std::set<std::string> values;
	for (auto& child : tagList.children) {
		values.insert(child->as<TagString>().value);
	}
	return values;
}

std::set<int> TagList::deserialiseIntSet(const TagList& tagList) {
	std::set<int> values;
	for (auto& child : tagList.children) {
		values.insert(child->as<TagInt>().value);
	}
	return values;
}

void TagList::writePayload(Buffer& buffer) {
	buffer.write<int8_t>(valueId);
	buffer.write<int32_t>((int32_t)children.size());
	for (auto& child : children) {
		child->writePayload(buffer);
	}
}

TagUPtr TagList::serialiseivec2(const tf_v0_ivec2& value, const std::string& name) {
	Tags children;
	children.push_back(std::make_unique<TagInt>(value.x, "x"));
	children.push_back(std::make_unique<TagInt>(value.y, "y"));
	return std::make_unique<TagList>(std::move(children), name, 3);
}

tf_v0_ivec2 TagList::deserialiseivec2() const {
	tf_v0_ivec2 value;
	if (children.size() != 2) {
		throw ParseError("ivec2 does not have two children");
	}
	value.x = children[0]->as<NBT::TagInt>().value;
	value.y = children[1]->as<NBT::TagInt>().value;

	return value;
}

TagUPtr TagList::serialiseivec3(const tf_v0_ivec3& value, const std::string& name) {
	Tags children;
	children.push_back(std::make_unique<TagInt>(value.x, "x"));
	children.push_back(std::make_unique<TagInt>(value.y, "y"));
	children.push_back(std::make_unique<TagInt>(value.z, "z"));
	return std::make_unique<TagList>(std::move(children), name, 3);
}

tf_v0_ivec3 TagList::deserialiseivec3() const {
	tf_v0_ivec3 value;
	if (children.size() != 3) {
		throw ParseError("ivec3 does not have two children");
	}
	value.x = children[0]->as<NBT::TagInt>().value;
	value.y = children[1]->as<NBT::TagInt>().value;
	value.z = children[2]->as<NBT::TagInt>().value;

	return value;
}

TagUPtr TagList::serialisevec2(const tf_v0_vec2& value, const std::string& name) {
	Tags children;
	children.push_back(std::make_unique<TagFloat>(value.x, "x"));
	children.push_back(std::make_unique<TagFloat>(value.y, "y"));
	return std::make_unique<TagList>(std::move(children), name, 5);
}

tf_v0_vec2 TagList::deserialisevec2() const {
	tf_v0_vec2 value;
	if (children.size() != 2) {
		throw ParseError("vec2 does not have two children");
	}
	value.x = children[0]->as<NBT::TagFloat>().value;
	value.y = children[1]->as<NBT::TagFloat>().value;

	return value;
}

TagUPtr TagList::serialisevec3(const tf_v0_vec3& value, const std::string& name) {
	Tags children;
	children.push_back(std::make_unique<TagFloat>(value.x, "x"));
	children.push_back(std::make_unique<TagFloat>(value.y, "y"));
	children.push_back(std::make_unique<TagFloat>(value.z, "z"));
	return std::make_unique<TagList>(std::move(children), name, 5);
}

tf_v0_vec3 TagList::deserialisevec3() const {
	tf_v0_vec3 value;
	if (children.size() != 3) {
		throw ParseError("vec3 does not have two children");
	}
	value.x = children[0]->as<NBT::TagFloat>().value;
	value.y = children[1]->as<NBT::TagFloat>().value;
	value.z = children[2]->as<NBT::TagFloat>().value;

	return value;
}

// TagUPtr TagList::serialisevec4(const tf_v0_vec4& value, const std::string& name) {
// 	Tags children;
// 	children.push_back(std::make_unique<TagFloat>(value.x, "x"));
// 	children.push_back(std::make_unique<TagFloat>(value.y, "y"));
// 	children.push_back(std::make_unique<TagFloat>(value.z, "z"));
// 	children.push_back(std::make_unique<TagFloat>(value.w, "w"));
// 	return std::make_unique<TagList>(std::move(children), name, 5);
// }
// tf_v0_vec4 TagList::deserialisevec4() const {
// 	tf_v0_vec4 value;
// 	if (children.size() != 4) {
// 		throw ParseError("vec4 does not have four children");
// 	}
// 	value.x = children[0]->as<NBT::TagFloat>().value;
// 	value.y = children[1]->as<NBT::TagFloat>().value;
// 	value.z = children[2]->as<NBT::TagFloat>().value;
// 	value.w = children[3]->as<NBT::TagFloat>().value;

// 	return value;
// }

void TagCompound::writePayload(Buffer& buffer) {
	for (auto& child : children) {
		child->write(buffer);
	}
	TagEnd().write(buffer);
}

void TagIntArray::writePayload(Buffer& buffer) {
	buffer.write<int32_t>((int32_t)values.size());
	for (auto value : values) {
		buffer.write<int32_t>(value);
	}
}

void TagLongArray::writePayload(Buffer& buffer) {
	buffer.write<int32_t>((int32_t)values.size());
	for (auto value : values) {
		buffer.write<int64_t>(value);
	}
}