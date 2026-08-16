#include "validator.hpp"

#include <array>
#include <limits>
#include <span>

namespace fix {

namespace {

struct MessageRule {
    std::string_view type;
    std::span<const Tag> required_tags;
};

inline constexpr std::array<Tag, 2> logon_tags{
    98,
    108
};

inline constexpr std::array<Tag, 5> new_order_tags{
    11,
    55,
    54,
    38,
    40
};

inline constexpr std::array<Tag, 7> execution_report_tags{
    37,
    17,
    39,
    150,
    55,
    54,
    38
};

inline constexpr std::array<MessageRule, 3> message_rules{{
    MessageRule{
        "A",
        std::span<const Tag>{
            logon_tags.data(),
            logon_tags.size()
        }
    },

    MessageRule{
        "D",
        std::span<const Tag>{
            new_order_tags.data(),
            new_order_tags.size()
        }
    },

    MessageRule{
        "8",
        std::span<const Tag>{
            execution_report_tags.data(),
            execution_report_tags.size()
        }
    }
}};

inline constexpr std::array<Tag, 7> required_header_tags{
    8,
    9,
    35,
    49,
    56,
    34,
    52
};

[[nodiscard]] constexpr const MessageRule* find_rule(
    std::string_view msg_type) noexcept {

    for (const auto& rule : message_rules) {
        if (rule.type == msg_type) {
            return &rule;
        }
    }

    return nullptr;
}

[[nodiscard]] constexpr ValidationErrorCode
missing_header_error(Tag tag) noexcept {

    switch (tag) {
        case 8:
            return ValidationErrorCode::MissingBeginString;

        case 9:
            return ValidationErrorCode::MissingBodyLength;

        case 35:
            return ValidationErrorCode::MissingMsgType;

        case 49:
            return ValidationErrorCode::MissingSenderCompId;

        case 56:
            return ValidationErrorCode::MissingTargetCompId;

        case 34:
            return ValidationErrorCode::MissingMsgSeqNum;

        case 52:
            return ValidationErrorCode::MissingSendingTime;

        default:
            return ValidationErrorCode::None;
    }
}

} // namespace

ValidationResult<Validator::max_errors> Validator::validate(
    std::string_view raw_message,
    const FixMessage<>& message) const noexcept {

    ValidationResult<max_errors> result{};

    validate_header(message, result);
    validate_body(message, result);
    validate_checksum(raw_message, message, result);

    return result;
}

void Validator::validate_header(
    const FixMessage<>& message,
    ValidationResult<max_errors>& result) noexcept {

    for (const Tag tag : required_header_tags) {

        if (message.has(tag)) {
            continue;
        }

        if (!result.add(
                missing_header_error(tag),
                tag)) {

            return;
        }
    }
}

void Validator::validate_body(
    const FixMessage<>& message,
    ValidationResult<max_errors>& result) noexcept {

    const auto* msg_type_field =
        message.find(35);

    if (msg_type_field == nullptr) {
        return;
    }

    const MessageRule* rule =
        find_rule(msg_type_field->value);

    if (rule == nullptr) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::UnknownMessageType,
                35));

        return;
    }

    for (const Tag tag : rule->required_tags) {

        if (message.has(tag)) {
            continue;
        }

        if (!result.add(
                ValidationErrorCode::MissingBodyTag,
                tag)) {

            return;
        }
    }
}

void Validator::validate_checksum(
    std::string_view raw_message,
    const FixMessage<>& message,
    ValidationResult<max_errors>& result) noexcept {

    const auto* checksum_field =
        message.find(10);

    if (checksum_field == nullptr) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::MissingChecksum,
                10));

        return;
    }

    std::uint32_t provided_checksum = 0;

    if (!parse_uint(
            checksum_field->value,
            provided_checksum)) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::InvalidChecksum,
                10));

        return;
    }

    if (provided_checksum > 255U) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::InvalidChecksum,
                10));

        return;
    }

    const std::uint32_t expected_checksum =
        compute_checksum(raw_message);

    if (expected_checksum ==
        std::numeric_limits<std::uint32_t>::max()) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::MissingChecksum,
                10));

        return;
    }

    if (expected_checksum != provided_checksum) {

        static_cast<void>(
            result.add(
                ValidationErrorCode::ChecksumMismatch,
                10));
    }
}

std::uint32_t Validator::compute_checksum(
    std::string_view raw_message) noexcept {

    std::size_t checksum_position =
        std::string_view::npos;

    /*
     * CheckSum must be the final FIX field.
     *
     * Search for a real field boundary:
     *
     *   SOH + "10="
     *
     * rather than searching for "10=" anywhere
     * inside the message.
     */

    if (raw_message.starts_with("10=")) {
        checksum_position = 0;
    } else {

        std::size_t position = 0;

        while (position < raw_message.size()) {

            const std::size_t soh =
                raw_message.find(soh_del, position);

            if (soh == std::string_view::npos) {
                break;
            }

            const std::size_t next =
                soh + 1;

            if (next + 3 <= raw_message.size() &&
                raw_message[next] == '1' &&
                raw_message[next + 1] == '0' &&
                raw_message[next + 2] == '=') {

                checksum_position = next;
                break;
            }

            position = next;
        }
    }

    if (checksum_position ==
        std::string_view::npos) {

        return std::numeric_limits<std::uint32_t>::max();
    }

    std::uint32_t sum = 0;

    for (std::size_t i = 0;
         i < checksum_position;
         ++i) {

        sum += static_cast<unsigned char>(
            raw_message[i]);
    }

    return sum % 256U;
}

bool Validator::parse_uint(
    std::string_view value,
    std::uint32_t& result) noexcept {

    if (value.empty()) {
        return false;
    }

    std::uint32_t parsed = 0;

    for (const char ch : value) {

        if (ch < '0' || ch > '9') {
            return false;
        }

        const std::uint32_t digit =
            static_cast<std::uint32_t>(ch - '0');

        if (parsed >
            (std::numeric_limits<std::uint32_t>::max()
             - digit) / 10U) {

            return false;
        }

        parsed =
            parsed * 10U + digit;
    }

    result = parsed;

    return true;
}

} // namespace fix