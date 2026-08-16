#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "storage.hpp"

namespace fix {

enum class ValidationErrorCode : std::uint8_t {
    None,

    MissingBeginString,
    MissingBodyLength,
    MissingMsgType,
    MissingSenderCompId,
    MissingTargetCompId,
    MissingMsgSeqNum,
    MissingSendingTime,

    MissingBodyTag,
    UnknownMessageType,

    MissingChecksum,
    InvalidChecksum,
    ChecksumMismatch
};

struct ValidationError {
    ValidationErrorCode code{ValidationErrorCode::None};
    Tag tag{0};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return code != ValidationErrorCode::None;
    }
};

template <std::size_t MaxErrors = 16>
struct ValidationResult {
    std::array<ValidationError, MaxErrors> errors{};
    std::uint8_t count{0};

    [[nodiscard]] constexpr bool ok() const noexcept {
        return count == 0;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return count;
    }

    constexpr bool add(
        ValidationErrorCode code,
        Tag tag = 0) noexcept {

        if (count >= MaxErrors) {
            return false;
        }

        errors[count++] = ValidationError{
            .code = code,
            .tag = tag
        };

        return true;
    }

    [[nodiscard]] constexpr const ValidationError* begin() const noexcept {
        return errors.data();
    }

    [[nodiscard]] constexpr const ValidationError* end() const noexcept {
        return errors.data() + count;
    }
};

class Validator {
public:
    static constexpr std::size_t max_errors = 16;

    [[nodiscard]] ValidationResult<max_errors> validate(
        std::string_view raw_message,
        const FixMessage<>& message) const noexcept;

private:
    static void validate_header(
        const FixMessage<>& message,
        ValidationResult<max_errors>& result) noexcept;

    static void validate_body(
        const FixMessage<>& message,
        ValidationResult<max_errors>& result) noexcept;

    static void validate_checksum(
        std::string_view raw_message,
        const FixMessage<>& message,
        ValidationResult<max_errors>& result) noexcept;

    [[nodiscard]] static std::uint32_t compute_checksum(
        std::string_view raw_message) noexcept;

    [[nodiscard]] static bool parse_uint(
        std::string_view value,
        std::uint32_t& result) noexcept;
};

} // namespace fix