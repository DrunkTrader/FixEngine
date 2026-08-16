#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "parser.hpp"

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

void test_basic_parse() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    CHECK(result.message.size() == 6);

    CHECK(result.message.get(8) == "FIX.4.4");
    CHECK(result.message.get(35) == "D");
    CHECK(result.message.get(11) == "ORDER123");
    CHECK(result.message.get(55) == "AAPL");
    CHECK(result.message.get(54) == "1");
    CHECK(result.message.get(38) == "100");
}

void test_string_input() {

    const std::string raw =
        "8=FIX.4.4\x01"
        "35=A\x01"
        "98=0\x01"
        "108=30\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    CHECK(result.message.size() == 4);

    CHECK(result.message.get(35) == "A");
    CHECK(result.message.get(98) == "0");
    CHECK(result.message.get(108) == "30");
}

void test_zero_copy() {

    std::string raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "55=AAPL\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    const auto* field =
        result.message.find(55);

    CHECK(field != nullptr);

    if (field == nullptr) {
        return;
    }

    const char* begin =
        raw.data();

    const char* end =
        raw.data() + raw.size();

    CHECK(field->value.data() >= begin);
    CHECK(field->value.data() < end);

    CHECK(field->value == "AAPL");
}

void test_missing_delimiter() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35D\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::MissingDelimiter
    );

    CHECK(result.error.position == 10);
}

void test_invalid_tag() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "ABC=D\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::InvalidTag
    );

    CHECK(result.error.position == 10);
}

void test_empty_tag() {

    constexpr std::string_view raw =
        "=FIX.4.4\x01";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(!result.ok());

    CHECK(
        result.error.code ==
        fix::ParseError::Code::EmptyTag
    );

    CHECK(result.error.position == 0);
}

void test_empty_input() {

    constexpr std::string_view raw{};

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    CHECK(result.message.empty());
    CHECK(result.message.size() == 0);
}

void test_final_field_without_soh() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "55=AAPL";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    CHECK(result.message.size() == 3);
    CHECK(result.message.get(55) == "AAPL");
}

void test_pipe_is_not_wire_delimiter() {

    constexpr std::string_view raw =
        "8=FIX.4.4|35=D|";

    const fix::Parser parser;

    const auto result =
        parser.parse(raw);

    CHECK(result.ok());

    CHECK(result.message.size() == 1);

    CHECK(
        result.message.get(8) ==
        "FIX.4.4|35=D|"
    );
}

void test_multiple_parser_calls() {

    const fix::Parser parser;

    constexpr std::string_view first =
        "35=D\x01"
        "55=AAPL\x01";

    constexpr std::string_view second =
        "35=A\x01"
        "98=0\x01"
        "108=30\x01";

    const auto first_result =
        parser.parse(first);

    const auto second_result =
        parser.parse(second);

    CHECK(first_result.ok());
    CHECK(second_result.ok());

    CHECK(first_result.message.size() == 2);
    CHECK(second_result.message.size() == 3);

    CHECK(first_result.message.get(35) == "D");
    CHECK(second_result.message.get(35) == "A");
}

void test_result_is_independent() {

    const fix::Parser parser;

    std::string first =
        "35=D\x01"
        "55=AAPL\x01";

    std::string second =
        "35=A\x01"
        "55=MSFT\x01";

    const auto first_result =
        parser.parse(first);

    const auto second_result =
        parser.parse(second);

    CHECK(first_result.ok());
    CHECK(second_result.ok());

    CHECK(first_result.message.get(55) == "AAPL");
    CHECK(second_result.message.get(55) == "MSFT");

    /*
     * Each FixMessage owns only views.
     *
     * The backing buffers are different, so the views
     * must point into their respective input strings.
     */

    const auto* first_field =
        first_result.message.find(55);

    const auto* second_field =
        second_result.message.find(55);

    CHECK(first_field != nullptr);
    CHECK(second_field != nullptr);

    if (first_field == nullptr ||
        second_field == nullptr) {
        return;
    }

    CHECK(
        first_field->value.data() >= first.data());

    CHECK(
        first_field->value.data() <
        first.data() + first.size());

    CHECK(
        second_field->value.data() >=
        second.data());

    CHECK(
        second_field->value.data() <
        second.data() + second.size());
}

} // namespace

int main() {

    test_basic_parse();
    test_string_input();
    test_zero_copy();

    test_missing_delimiter();
    test_invalid_tag();
    test_empty_tag();

    test_empty_input();
    test_final_field_without_soh();

    test_pipe_is_not_wire_delimiter();

    test_multiple_parser_calls();
    test_result_is_independent();

    if (failures != 0) {

        std::cerr
            << '\n'
            << failures
            << " test(s) failed.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "All parser tests passed.\n";

    return EXIT_SUCCESS;
}