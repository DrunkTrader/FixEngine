#pragma once

#include <string>
#include "storage.hpp"
#include "tokenizer.hpp"

namespace fix {
    class Parser {
    public:
        Parser() = default;
        ~Parser() = default;

        // Orchestrates tokenization and field extraction
        FixMessage parse(const std::string& raw);
    };
}
