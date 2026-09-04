#pragma once

#include <vector>
#include <string>
#include "storage.hpp"

namespace fix {
	constexpr char soh_del = '\x01';

	std::string preprocess_delimeter(const std::string &input);
	std::vector<std::string> tokenize(const std::string &input);
	FixField splitField(const std::string &token);
}

// this stores the preprocess deliemter to normalize the use of '|' and soh delimeter = '\x01'
// also tokenize the input string to tags and value.
