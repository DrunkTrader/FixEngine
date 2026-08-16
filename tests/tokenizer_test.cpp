#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "tokenizer.hpp"

namespace {

int failures = 0;

void check(
    bool condition,
    const char* expression,
    const char* file,
    int line) {

    if (condition) {
        return;
    }

    std::cerr
        << "FAIL: "
        << file
        << ':'
        << line
        << " -> "
        << expression
        << '\n';

    ++failures;
}

#define CHECK(expr) \
    check((expr), #expr, __FILE__, __LINE__)

void test_basic_message() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 6);

    CHECK(result.message.get(8) == "FIX.4.4");
    CHECK(result.message.get(35) == "D");
    CHECK(result.message.get(11) == "ORDER123");
    CHECK(result.message.get(55) == "AAPL");
    CHECK(result.message.get(54) == "1");
    CHECK(result.message.get(38) == "100");
}

void test_zero_copy() {

    std::string raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "55=AAPL\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());

    const auto* field =
        result.message.find(55);

    CHECK(field != nullptr);

    if (field == nullptr) {
        return;
    }

    /*
     * The parsed value must point into the original
     * input buffer rather than a newly allocated string.
     */
    const char* begin =
        raw.data();

    const char* end =
        raw.data() + raw.size();

    CHECK(field->value.data() >= begin);
    CHECK(field->value.data() < end);

    CHECK(field->value == "AAPL");
}

void test_empty_value() {

    constexpr std::string_view raw =
        "35=\x01"
        "55=AAPL\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 2);

    const auto* field =
        result.message.find(35);

    CHECK(field != nullptr);

    if (field != nullptr) {
        CHECK(field->value.empty());
    }
}

void test_final_field_without_soh() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "55=AAPL";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 3);
    CHECK(result.message.get(55) == "AAPL");
}

void test_missing_equals() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35D\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::MissingDelimiter
    );
}

void test_empty_tag() {

    constexpr std::string_view raw =
        "=FIX.4.4\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::EmptyTag
    );
}

void test_invalid_tag() {

    constexpr std::string_view raw =
        "ABC=value\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::InvalidTag
    );
}

void test_tag_overflow() {

    constexpr std::string_view raw =
        "999999=value\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::InvalidTag
    );
}

void test_missing_value_is_allowed() {

    constexpr std::string_view raw =
        "35=\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 1);
    CHECK(result.message.get(35).empty());
}

void test_pipe_is_not_a_delimiter() {

    constexpr std::string_view raw =
        "8=FIX.4.4|35=D|";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 1);

    const auto* field =
        result.message.find(8);

    CHECK(field != nullptr);

    if (field != nullptr) {

        /*
         * '|' is NOT the FIX wire delimiter.
         *
         * The production parser must not silently
         * transform it into SOH.
         */
        CHECK(
            field->value ==
            "FIX.4.4|35=D|"
        );
    }
}

void test_missing_delimiter_after_previous_field() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35D";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::MissingDelimiter
    );
}

void test_field_iteration() {

    constexpr std::string_view raw =
        "35=D\x01"
        "55=AAPL\x01"
        "54=1\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());

    std::size_t count = 0;

    for (const auto& field : result.message) {

        CHECK(field.tag != 0);

        ++count;
    }

    CHECK(count == 3);
}

void test_has() {

    constexpr std::string_view raw =
        "35=D\x01"
        "55=AAPL\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());

    CHECK(result.message.has(35));
    CHECK(result.message.has(55));
    CHECK(!result.message.has(54));
}

void test_unknown_tag_lookup() {

    constexpr std::string_view raw =
        "35=D\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());

    CHECK(
        result.message.get(999) ==
        std::string_view{}
    );
}

void test_duplicate_tags() {

    constexpr std::string_view raw =
        "55=AAPL\x01"
        "55=MSFT\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 2);

    /*
     * Current storage semantics return the first
     * matching field.
     */
    CHECK(result.message.get(55) == "AAPL");
}

void test_leading_zero_tag() {

    constexpr std::string_view raw =
        "035=D\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 1);
    CHECK(result.message.get(35) == "D");
}

void test_single_field() {

    constexpr std::string_view raw =
        "35=D";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.size() == 1);
    CHECK(result.message.get(35) == "D");
}

void test_empty_input() {

    constexpr std::string_view raw{};

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(result.ok());
    CHECK(result.message.empty());
    CHECK(result.message.size() == 0);
}

void test_capacity() {

    std::string raw;

    /*
     * FixMessage<> currently has a capacity of 64 fields.
     */
    for (int tag = 1; tag <= 65; ++tag) {

        raw += std::to_string(tag);
        raw += "=x";
        raw += '\x01';
    }

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::TooManyFields
    );

    CHECK(result.message.size() == 64);
}

void test_error_position() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35D\x01";

    const auto result =
        fix::Tokenizer::tokenize(raw);

    CHECK(!result.ok());

    /*
     * The malformed field starts immediately after
     * the first field and its SOH.
     */
    CHECK(result.error.position == 10);
}

} // namespace

int main() {

    test_basic_message();
    test_zero_copy();
    test_empty_value();
    test_final_field_without_soh();

    test_missing_equals();
    test_empty_tag();
    test_invalid_tag();
    test_tag_overflow();

    test_missing_value_is_allowed();

    test_pipe_is_not_a_delimiter();

    test_missing_delimiter_after_previous_field();

    test_field_iteration();
    test_has();
    test_unknown_tag_lookup();

    test_duplicate_tags();
    test_leading_zero_tag();

    test_single_field();
    test_empty_input();

    test_capacity();
    test_error_position();

    if (failures != 0) {

        std::cerr
            << '\n'
            << failures
            << " test(s) failed.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "All tokenizer tests passed.\n";

    return EXIT_SUCCESS;
}