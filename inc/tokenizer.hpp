#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "storage.hpp"

namespace fix {

struct TokenizerResult {
    FixMessage<> message{};
    ParseError error{};

    [[nodiscard]] constexpr bool ok() const noexcept {
        return error.ok();
    }
};

class Tokenizer {
public:
    static constexpr std::size_t max_fields = 64;

    [[nodiscard]] static TokenizerResult tokenize(
        std::string_view input) noexcept;
};

} // namespace fix