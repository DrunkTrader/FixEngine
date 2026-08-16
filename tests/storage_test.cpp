#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "storage.hpp"

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


void test_empty_message() {

    fix::FixMessage<> message;

    CHECK(message.empty());
    CHECK(message.size() == 0);
    CHECK(message.begin() == message.end());
}


void test_push_and_size() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(54, "1"));

    CHECK(!message.empty());
    CHECK(message.size() == 3);
}


void test_field_order() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(54, "1"));

    CHECK(message[0].tag == 35);
    CHECK(message[1].tag == 55);
    CHECK(message[2].tag == 54);

    CHECK(message[0].value == "D");
    CHECK(message[1].value == "AAPL");
    CHECK(message[2].value == "1");
}


void test_iteration() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(54, "1"));

    std::size_t count = 0;

    for (const auto& field : message) {

        CHECK(field.tag != 0);

        ++count;
    }

    CHECK(count == 3);
}


void test_find() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(54, "1"));

    const auto* field =
        message.find(55);

    CHECK(field != nullptr);

    if (field != nullptr) {
        CHECK(field->tag == 55);
        CHECK(field->value == "AAPL");
    }
}


void test_find_missing() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));

    CHECK(message.find(55) == nullptr);
}


void test_get() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));

    CHECK(message.get(35) == "D");
    CHECK(message.get(55) == "AAPL");
}


void test_get_fallback() {

    fix::FixMessage<> message;

    constexpr std::string_view fallback =
        "NOT_FOUND";

    CHECK(
        message.get(999, fallback) ==
        fallback
    );
}


void test_has() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));

    CHECK(message.has(35));
    CHECK(message.has(55));

    CHECK(!message.has(54));
}


void test_empty_field() {

    fix::FixMessage<> message;

    CHECK(message.push(35, ""));

    const auto* field =
        message.find(35);

    CHECK(field != nullptr);

    if (field != nullptr) {
        CHECK(field->empty());
        CHECK(field->value.empty());
    }
}


void test_zero_copy_storage() {

    std::string raw =
        "AAPL";

    fix::FixMessage<> message;

    CHECK(message.push(55, raw));

    const auto* field =
        message.find(55);

    CHECK(field != nullptr);

    if (field == nullptr) {
        return;
    }

    /*
     * FixMessage stores std::string_view.
     *
     * The field must point directly into the original
     * input buffer rather than allocating/copying it.
     */

    CHECK(field->value.data() == raw.data());
    CHECK(field->value.size() == raw.size());

    CHECK(field->value == "AAPL");
}


void test_zero_copy_multiple_fields() {

    std::string first = "AAPL";
    std::string second = "100";
    std::string third = "BUY";

    fix::FixMessage<> message;

    CHECK(message.push(55, first));
    CHECK(message.push(38, second));
    CHECK(message.push(54, third));

    const auto* symbol =
        message.find(55);

    const auto* quantity =
        message.find(38);

    const auto* side =
        message.find(54);

    CHECK(symbol != nullptr);
    CHECK(quantity != nullptr);
    CHECK(side != nullptr);

    if (symbol == nullptr ||
        quantity == nullptr ||
        side == nullptr) {
        return;
    }

    CHECK(symbol->value.data() == first.data());
    CHECK(quantity->value.data() == second.data());
    CHECK(side->value.data() == third.data());
}


void test_contiguous_field_storage() {

    fix::FixMessage<> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(54, "1"));
    CHECK(message.push(38, "100"));

    const auto* first =
        message.begin();

    const auto* second =
        first + 1;

    const auto* third =
        first + 2;

    const auto* fourth =
        first + 3;

    CHECK(second == first + 1);
    CHECK(third == first + 2);
    CHECK(fourth == first + 3);

    CHECK(
        message.end() ==
        message.begin() + message.size()
    );
}


void test_capacity() {

    constexpr std::size_t capacity = 4;

    fix::FixMessage<capacity> message;

    CHECK(message.size() == 0);

    CHECK(message.push(1, "one"));
    CHECK(message.push(2, "two"));
    CHECK(message.push(3, "three"));
    CHECK(message.push(4, "four"));

    CHECK(message.size() == capacity);

    CHECK(!message.push(5, "five"));

    CHECK(message.size() == capacity);

    CHECK(message[0].tag == 1);
    CHECK(message[1].tag == 2);
    CHECK(message[2].tag == 3);
    CHECK(message[3].tag == 4);
}


void test_capacity_does_not_overwrite_existing_fields() {

    fix::FixMessage<2> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));

    CHECK(!message.push(54, "1"));

    CHECK(message.size() == 2);

    CHECK(message[0].tag == 35);
    CHECK(message[0].value == "D");

    CHECK(message[1].tag == 55);
    CHECK(message[1].value == "AAPL");
}


void test_duplicate_tags_preserve_order() {

    fix::FixMessage<> message;

    CHECK(message.push(55, "AAPL"));
    CHECK(message.push(55, "MSFT"));

    CHECK(message.size() == 2);

    CHECK(message[0].tag == 55);
    CHECK(message[1].tag == 55);

    CHECK(message[0].value == "AAPL");
    CHECK(message[1].value == "MSFT");

    /*
     * find()/get() currently return the first occurrence.
     */
    CHECK(message.get(55) == "AAPL");
}


void test_custom_capacity() {

    fix::FixMessage<8> message;

    for (std::uint16_t tag = 1; tag <= 8; ++tag) {

        CHECK(
            message.push(
                static_cast<fix::Tag>(tag),
                "x")
        );
    }

    CHECK(message.size() == 8);
    CHECK(!message.push(9, "x"));
}


void test_tag_range() {

    fix::FixMessage<> message;

    constexpr fix::Tag max_tag =
        UINT16_MAX;

    CHECK(message.push(max_tag, "MAX"));

    CHECK(message.has(max_tag));
    CHECK(message.get(max_tag) == "MAX");
}


void test_fix_field_properties() {

    static_assert(
        std::is_trivially_copyable_v<fix::FixField>
    );

    static_assert(
        std::is_trivially_destructible_v<fix::FixField>
    );

    static_assert(
        std::is_trivially_copyable_v<fix::FixMessage<>>
    );

    static_assert(
        std::is_trivially_destructible_v<fix::FixMessage<>>
    );

    fix::FixField field{
        .tag = 55,
        .value = "AAPL"
    };

    CHECK(field.tag == 55);
    CHECK(field.value == "AAPL");
    CHECK(!field.empty());
}


void test_storage_has_fixed_capacity() {

    /*
     * FixMessage<> must contain its field storage directly.
     *
     * There should be no separately allocated backing
     * container such as std::vector.
     */
    constexpr std::size_t expected_fields =
        64;

    fix::FixMessage<expected_fields> message;

    CHECK(message.size() == 0);

    for (std::size_t i = 0; i < expected_fields; ++i) {

        CHECK(
            message.push(
                static_cast<fix::Tag>(i + 1),
                "x")
        );
    }

    CHECK(message.size() == expected_fields);
}


void test_copy_is_shallow_for_string_views() {

    std::string raw =
        "AAPL";

    fix::FixMessage<> first;

    CHECK(first.push(55, raw));

    const auto second =
        first;

    CHECK(second.size() == 1);
    CHECK(second.get(55) == "AAPL");

    const auto* first_field =
        first.find(55);

    const auto* second_field =
        second.find(55);

    CHECK(first_field != nullptr);
    CHECK(second_field != nullptr);

    if (first_field == nullptr ||
        second_field == nullptr) {
        return;
    }

    /*
     * Copying the message copies the string_view,
     * not the underlying character buffer.
     */
    CHECK(
        first_field->value.data() ==
        second_field->value.data()
    );
}


void test_move_is_trivial_for_storage() {

    std::string raw =
        "AAPL";

    fix::FixMessage<> first;

    CHECK(first.push(55, raw));

    auto second =
        std::move(first);

    CHECK(second.size() == 1);
    CHECK(second.get(55) == "AAPL");

    const auto* field =
        second.find(55);

    CHECK(field != nullptr);

    if (field != nullptr) {
        CHECK(field->value.data() == raw.data());
    }
}


void test_begin_end_only_expose_active_fields() {

    fix::FixMessage<8> message;

    CHECK(message.push(35, "D"));
    CHECK(message.push(55, "AAPL"));

    CHECK(
        static_cast<std::size_t>(
            message.end() - message.begin()
        ) == 2
    );

    CHECK(message.begin()[0].tag == 35);
    CHECK(message.begin()[1].tag == 55);
}


void test_parse_error_default_state() {

    fix::ParseError error;

    CHECK(error.ok());
    CHECK(
        error.code ==
        fix::ParseError::Code::None
    );

    CHECK(error.position == 0);
}


void test_parse_error_state() {

    fix::ParseError error{
        .code =
            fix::ParseError::Code::InvalidTag,
        .position = 42
    };

    CHECK(!error.ok());

    CHECK(
        error.code ==
        fix::ParseError::Code::InvalidTag
    );

    CHECK(error.position == 42);
}


} // namespace


int main() {

    test_empty_message();

    test_push_and_size();
    test_field_order();
    test_iteration();

    test_find();
    test_find_missing();

    test_get();
    test_get_fallback();
    test_has();

    test_empty_field();

    test_zero_copy_storage();
    test_zero_copy_multiple_fields();

    test_contiguous_field_storage();

    test_capacity();
    test_capacity_does_not_overwrite_existing_fields();

    test_duplicate_tags_preserve_order();

    test_custom_capacity();
    test_tag_range();

    test_fix_field_properties();
    test_storage_has_fixed_capacity();

    test_copy_is_shallow_for_string_views();
    test_move_is_trivial_for_storage();

    test_begin_end_only_expose_active_fields();

    test_parse_error_default_state();
    test_parse_error_state();

    if (failures != 0) {

        std::cerr
            << '\n'
            << failures
            << " test(s) failed.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "All storage tests passed.\n";

    return EXIT_SUCCESS;
}