#pragma once

#include <string_view>

#include "storage.hpp"
#include "tokenizer.hpp"

namespace fix {

class Parser {
public:
    Parser() = default;
    ~Parser() = default;

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(Parser&&) = delete;
    Parser& operator=(Parser&&) = delete;

    [[nodiscard]] TokenizerResult parse(
        std::string_view raw) const noexcept {
        return Tokenizer::tokenize(raw);
    }
};

} // namespace fix