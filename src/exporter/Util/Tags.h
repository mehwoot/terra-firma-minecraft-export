#pragma once
#include "Api/v0/Vector.h"
#include "Buffer.h"
#include "Macros.h"
#include <set>

namespace Util {
	namespace Serialisation {
		namespace NBT {
			class ParseError : public std::runtime_error {
			public:
				ParseError(const std::string& message) : std::runtime_error(message) {}
			};

			StandardTypesStruct(Tag);

			typedef std::vector<TagUPtr> Tags;

			struct Tag {
				int8_t id;
				std::string name;

				Tag(int8_t id, const std::string& name) : id(id), name(name) {}
				Tag(Tag&& other) : id(other.id), name(std::move(other.name)) {}
				virtual ~Tag() {}

				template <class T>
				T& as() {
					try {
						return dynamic_cast<T&>(*this);
					} catch (std::bad_cast) {
						throw ::std::runtime_error(std::format("Expected {} to be {}", name, typeid(T).name()));
					}
				}
				virtual void write(::Buffer& buffer);
				virtual void writePayload(::Buffer& buffer) = 0;
			};

			struct TagComposite {
				std::vector<TagUPtr> children;
				TagComposite(std::vector<TagUPtr> children) : children(std::move(children)) {}

				Tag* findChildByName(const std::string& name) const;
			};

			struct TagEnd : public Tag {
				TagEnd() : Tag(0, "") {}
				virtual void write(::Buffer& buffer) override;
				virtual void writePayload(::Buffer& buffer) override;
			};

			struct TagByte : public Tag {
				int8_t value;
				TagByte(int8_t value, const std::string& name) : Tag(1, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagShort : public Tag {
				int16_t value;
				TagShort(int16_t value, const std::string& name) : Tag(2, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagInt : public Tag {
				int32_t value;
				TagInt(int32_t value, const std::string& name) : Tag(3, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagLong : public Tag {
				int64_t value;
				TagLong(int64_t value, const std::string& name) : Tag(4, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagFloat : public Tag {
				float value;
				TagFloat(float value, const std::string& name) : Tag(5, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagDouble : public Tag {
				double value;
				TagDouble(double value, const std::string& name) : Tag(6, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagByteArray : public Tag {
				std::vector<uint8_t> values;
				TagByteArray(std::vector<uint8_t> values, const std::string& name) : Tag(7, name), values(std::move(values)) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			template <int size>
			struct TagStaticByteArray : public Tag {
				std::array<uint8_t, size> values;
				TagStaticByteArray(uint8_t value, const std::string& name) : Tag(7, name) { std::fill(values.begin(), values.end(), value); }
				virtual void writePayload(::Buffer& buffer) override {
					buffer.write<int32_t>((int32_t)values.size());
					for (int i = 0; i < values.size(); i++) {
						buffer.write<int8_t>(values[i]);
					}
				}
			};

			struct TagString : public Tag {
				std::string value;
				TagString(const std::string& value, const std::string& name) : Tag(8, name), value(value) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagList : public Tag, public TagComposite {
				int8_t valueId;
				TagList(std::vector<TagUPtr> children, const std::string& name, int8_t valueId) : Tag(9, name), TagComposite(std::move(children)), valueId(valueId) {}
				TagList(const std::vector<std::string>& values, const std::string& name);
				TagList(const std::set<std::string>& values, const std::string& name);
				TagList(const std::set<int>& values, const std::string& name);

				virtual void writePayload(::Buffer& buffer) override;

				static TagUPtr serialiseivec2(const tf_v0_ivec2& value, const std::string& name);
				tf_v0_ivec2 deserialiseivec2() const;
				static TagUPtr serialisevec2(const tf_v0_vec2& value, const std::string& name);
				tf_v0_vec2 deserialisevec2() const;
				static TagUPtr serialiseivec3(const tf_v0_ivec3& value, const std::string& name);
				tf_v0_ivec3 deserialiseivec3() const;
				static TagUPtr serialisevec3(const tf_v0_vec3& value, const std::string& name);
				tf_v0_vec3 deserialisevec3() const;
				// static TagUPtr serialisevec4(const tf_v0_vec4& value, const std::string& name);
				// tf_v0_vec4 deserialisevec4() const;
				static std::vector<std::string> deserialiseStringList(const TagList& tagList);
				static std::set<std::string> deserialiseStringSet(const TagList& tagList);
				static std::set<int> deserialiseIntSet(const TagList& tagList);
				static int ID;
			};

			StandardTypesStruct(TagCompound);

			struct TagCompound : public Tag, public TagComposite {
				TagCompound() : Tag(10, "root"), TagComposite(std::vector<TagUPtr>{}) {}
				TagCompound(const std::string& tagName) : Tag(10, tagName), TagComposite(std::vector<TagUPtr>{}) {}
				TagCompound(std::vector<TagUPtr> children, const std::string& name) : Tag(10, name), TagComposite(std::move(children)) {}
				virtual void writePayload(::Buffer& buffer) override;

				static int ID;
			};

			struct TagIntArray : public Tag {
				std::vector<int32_t> values;
				TagIntArray(std::vector<int32_t> values, const std::string& name) : Tag(11, name), values(std::move(values)) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};

			struct TagLongArray : public Tag {
				std::vector<int64_t> values;
				TagLongArray(std::vector<int64_t> values, const std::string& name) : Tag(12, name), values(std::move(values)) {}
				virtual void writePayload(::Buffer& buffer) override;
				static int ID;
			};
		}
	}
}