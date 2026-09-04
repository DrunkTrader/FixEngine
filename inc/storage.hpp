#pragma once

#include <unordered_map>
#include <string>

namespace fix {

	using FixMessage = std::unordered_map<int, std::string>;

	struct FixField{
		int tag;
		std::string value;
	};

	FixMessage fix_field_storage(const std::string &raw_str);
}