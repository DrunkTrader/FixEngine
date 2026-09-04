#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include "storage.hpp"
#include "validator.hpp"

namespace fix {

    struct FormattedMessage {
        std::string header;
        std::string body;
        std::string trailer;
        std::string summary;
    };

    class Formatter {
    public:
        Formatter() = default;
        ~Formatter() = default;

        // Format a parsed FIX message for display/debugging
        FormattedMessage format(const FixMessage& message) const;

        // Format validation results with enhanced error reporting
        std::string formatValidationResult(const ValidationResult& result, 
                                           const std::string& raw_message = "") const;

        // Format a message with field details (tag, name, value, type)
        std::string formatDetailed(const FixMessage& message) const;

        // Convert message to human-readable string
        std::string toReadableString(const FixMessage& message) const;

    private:
        std::string getTagName(int tag) const;
        std::string getTagType(int tag) const;
        bool isHeaderTag(int tag) const;
        bool isTrailerTag(int tag) const;
    };

}
