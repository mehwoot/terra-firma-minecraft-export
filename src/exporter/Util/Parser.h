#pragma once
#include "Tags.h"
#include "Buffer.h"

namespace Util {
	namespace Serialisation {
		namespace NBT {
			class Parser {
			public:
				struct Options {
					std::optional<std::string> endTag;

					Options() = default;
					Options& endOnTag(const std::string& tagName) {
						endTag = tagName;
						return *this;
					}
				};

			protected:
				int indent = 0;
				bool debugOutput = false;
				bool finished = false;

				TagUPtr parsePayload(::Util::Serialisation::ReadBuffer& buffer, int8_t tagType, const std::string& name, Options options);
				void printIndent();

			public:
				std::vector<TagUPtr> parse(::Util::Serialisation::ReadBuffer& buffer, Options options = Options());
				Parser& setDebugOutput(bool debugOutput) { this->debugOutput = debugOutput; return *this; }
			};
		}

		void saveTagToFile(Util::Serialisation::NBT::Tag& tag, const std::filesystem::path& path);
		Util::Serialisation::NBT::TagUPtr loadTagFromFile(const std::filesystem::path& path);
	}
}