#include "tokenizer.hpp"

#include <limits>

namespace fix {

namespace {

[[nodiscard]] constexpr bool is_digit(char ch) noexcept {
    return ch >= '0' && ch <= '9';
}

[[nodiscard]] constexpr bool parse_tag(
    std::string_view text,
    Tag& tag) noexcept {

    if (text.empty()) {
        return false;
    }

    std::uint32_t value = 0;

    for (const char ch : text) {

        if (!is_digit(ch)) {
            return false;
        }

        const std::uint32_t digit =
            static_cast<std::uint32_t>(ch - '0');

        if (value >
            (std::numeric_limits<Tag>::max() - digit) / 10U) {
            return false;
        }

        value = value * 10U + digit;
    }

    tag = static_cast<Tag>(value);

    return true;
}

} // namespace

TokenizerResult Tokenizer::tokenize(
    std::string_view input) noexcept {

    TokenizerResult result{};

    std::size_t cursor = 0;

    while (cursor < input.size()) {

        if (result.message.size() >= max_fields) {

            result.error = {
                ParseError::Code::TooManyFields,
                cursor
            };

            return result;
        }

        const std::size_t field_start = cursor;

        const std::size_t delimiter =
            input.find('=', cursor);

        if (delimiter == std::string_view::npos) {

            result.error = {
                ParseError::Code::MissingDelimiter,
                cursor
            };

            return result;
        }

        if (delimiter == field_start) {

            result.error = {
                ParseError::Code::EmptyTag,
                cursor
            };

            return result;
        }

        const std::string_view tag_text =
            input.substr(
                field_start,
                delimiter - field_start);

        Tag tag{};

        if (!parse_tag(tag_text, tag)) {

            result.error = {
                ParseError::Code::InvalidTag,
                field_start
            };

            return result;
        }

        const std::size_t value_start =
            delimiter + 1;

        const std::size_t end =
            input.find(soh_del, value_start);

        const std::size_t value_end =
            end == std::string_view::npos
                ? input.size()
                : end;

        const std::string_view value =
            input.substr(
                value_start,
                value_end - value_start);

        if (!result.message.push(tag, value)) {

            result.error = {
                ParseError::Code::TooManyFields,
                field_start
            };

            return result;
        }

        if (end == std::string_view::npos) {
            break;
        }

        cursor = end + 1;
    }

    return result;
}

} // namespace fix