#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "parser.hpp"
#include "validator.hpp"

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

[[nodiscard]] bool has_error(
    const fix::ValidationResult<fix::Validator::max_errors>& result,
    fix::ValidationErrorCode code) {

    for (const auto& error : result) {
        if (error.code == code) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool has_error_for_tag(
    const fix::ValidationResult<fix::Validator::max_errors>& result,
    fix::ValidationErrorCode code,
    fix::Tag tag) {

    for (const auto& error : result) {

        if (error.code == code &&
            error.tag == tag) {
            return true;
        }
    }

    return false;
}

void test_valid_logon() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=A\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "98=0\x01"
        "108=30\x01"
        "10=139\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(result.ok());
    CHECK(result.size() == 0);
}

void test_valid_new_order() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=211\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(result.ok());
    CHECK(result.size() == 0);
}

void test_missing_begin_string() {

    constexpr std::string_view raw =
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error(
            result,
            fix::ValidationErrorCode::MissingBeginString)
    );

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingBeginString,
            8)
    );
}

void test_missing_body_length() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingBodyLength,
            9)
    );
}

void test_missing_msg_type() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingMsgType,
            35)
    );
}

void test_missing_sender_comp_id() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingSenderCompId,
            49)
    );
}

void test_missing_target_comp_id() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingTargetCompId,
            56)
    );
}

void test_missing_sequence_number() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingMsgSeqNum,
            34)
    );
}

void test_missing_sending_time() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingSendingTime,
            52)
    );
}

void test_unknown_message_type() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=Z\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::UnknownMessageType,
            35)
    );
}

void test_missing_logon_body_tag() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=A\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "98=0\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingBodyTag,
            108)
    );
}

void test_missing_new_order_body_tag() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingBodyTag,
            40)
    );
}

void test_missing_checksum() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::MissingChecksum,
            10)
    );
}

void test_invalid_checksum_value() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=ABC\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::InvalidChecksum,
            10)
    );
}

void test_checksum_out_of_range() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=999\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::InvalidChecksum,
            10)
    );
}

void test_checksum_mismatch() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=000\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(
        has_error_for_tag(
            result,
            fix::ValidationErrorCode::ChecksumMismatch,
            10)
    );
}

void test_three_digit_checksum() {

    constexpr std::string_view raw =
        "8=FIX.4.4\x01"
        "9=0\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=1\x01"
        "52=20260816-09:20:52\x01"
        "11=ORDER123\x01"
        "55=AAPL\x01"
        "54=1\x01"
        "38=100\x01"
        "40=2\x01"
        "10=211\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(result.ok());
}

void test_error_count_is_bounded() {

    constexpr std::string_view raw =
        "35=D\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());

    CHECK(result.size() <= fix::Validator::max_errors);
}

void test_result_contains_no_dynamic_error_strings() {

    constexpr std::string_view raw =
        "35=Z\x01";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    CHECK(parsed.ok());

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());
    CHECK(result.size() > 0);

    for (const auto& error : result) {

        CHECK(error.code !=
              fix::ValidationErrorCode::None);
    }
}

void test_pipe_message_is_not_valid_wire_message() {

    constexpr std::string_view raw =
        "8=FIX.4.4|"
        "35=D|"
        "49=SENDER|"
        "56=TARGET|"
        "34=1|"
        "52=20260816-09:20:52|"
        "11=ORDER123|"
        "55=AAPL|"
        "54=1|"
        "38=100|"
        "40=2|"
        "10=000|";

    const fix::Parser parser;
    const fix::Validator validator;

    const auto parsed =
        parser.parse(raw);

    /*
     * Because '|' isn't SOH, the tokenizer sees this
     * as one field. This test documents that behavior.
     */
    CHECK(parsed.ok());
    CHECK(parsed.message.size() == 1);

    if (!parsed.ok()) {
        return;
    }

    const auto result =
        validator.validate(
            raw,
            parsed.message);

    CHECK(!result.ok());
}

} // namespace

int main() {

    test_valid_logon();
    test_valid_new_order();

    test_missing_begin_string();
    test_missing_body_length();
    test_missing_msg_type();
    test_missing_sender_comp_id();
    test_missing_target_comp_id();
    test_missing_sequence_number();
    test_missing_sending_time();

    test_unknown_message_type();

    test_missing_logon_body_tag();
    test_missing_new_order_body_tag();

    test_missing_checksum();
    test_invalid_checksum_value();
    test_checksum_out_of_range();
    test_checksum_mismatch();
    test_three_digit_checksum();

    test_error_count_is_bounded();
    test_result_contains_no_dynamic_error_strings();

    test_pipe_message_is_not_valid_wire_message();

    if (failures != 0) {

        std::cerr
            << '\n'
            << failures
            << " test(s) failed.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "All validator tests passed.\n";

    return EXIT_SUCCESS;
}