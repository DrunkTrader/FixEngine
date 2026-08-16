#include <iostream>
#include <string_view>

#include "inc/parser.hpp"
#include "inc/validator.hpp"

namespace {

[[nodiscard]] constexpr std::string_view
error_name(fix::ValidationErrorCode code) noexcept {

    using Code = fix::ValidationErrorCode;

    switch (code) {
        case Code::None:
            return "None";

        case Code::MissingBeginString:
            return "MissingBeginString";

        case Code::MissingBodyLength:
            return "MissingBodyLength";

        case Code::MissingMsgType:
            return "MissingMsgType";

        case Code::MissingSenderCompId:
            return "MissingSenderCompId";

        case Code::MissingTargetCompId:
            return "MissingTargetCompId";

        case Code::MissingMsgSeqNum:
            return "MissingMsgSeqNum";

        case Code::MissingSendingTime:
            return "MissingSendingTime";

        case Code::MissingBodyTag:
            return "MissingBodyTag";

        case Code::UnknownMessageType:
            return "UnknownMessageType";

        case Code::MissingChecksum:
            return "MissingChecksum";

        case Code::InvalidChecksum:
            return "InvalidChecksum";

        case Code::ChecksumMismatch:
            return "ChecksumMismatch";
    }

    return "Unknown";
}

} // namespace

int main() {

    constexpr std::string_view raw_msg =
        "8=FIX.4.2\x01"
        "9=118\x01"
        "35=D\x01"
        "49=SENDER\x01"
        "56=TARGET\x01"
        "34=2\x01"
        "52=20240528-09:20:52\x01"
        "11=ORDERID\x01"
        "55=MSFT\x01"
        "54=1\x01"
        "38=1000\x01"
        "40=2\x01"
        "44=150.5\x01"
        "10=000\x01";

    fix::Parser parser;

    const auto parsed = parser.parse(raw_msg);

    if (!parsed.ok()) {
        std::cerr
            << "Parse failed at byte "
            << parsed.error.position
            << '\n';

        return 1;
    }

    const fix::Validator validator;

    const auto validation =
        validator.validate(raw_msg, parsed.message);

    std::cout << "Fields: "
              << parsed.message.size()
              << '\n';

    for (const auto& field : parsed.message) {
        std::cout
            << "Tag: "
            << field.tag
            << ", Value: "
            << field.value
            << '\n';
    }

    if (validation.ok()) {
        std::cout << "Validation: OK\n";
        return 0;
    }

    std::cout << "Validation errors:\n";

    for (const auto& error : validation) {
        std::cout
            << " - "
            << error_name(error.code);

        if (error.tag != 0) {
            std::cout
                << " (tag "
                << error.tag
                << ')';
        }

        std::cout << '\n';
    }

    return 1;
}