#pragma once

#include <string>
#include <vector>
#include <set>
#include "storage.hpp"

namespace fix {
    struct ValidationError {
        std::string message;
        int tag = -1;           // Associated tag if applicable
        std::size_t position = 0; // Position in raw message if applicable
        std::string field_value; // The problematic value if applicable
    };

    struct ValidationResult {
        bool ok = true;
        std::vector<ValidationError> errors;
        
        // Helper to get simple error messages (backward compatibility)
        std::vector<std::string> getErrorMessages() const {
            std::vector<std::string> messages;
            for (const auto& err : errors) {
                messages.push_back(err.message);
            }
            return messages;
        }
    };

    struct ParseResult {
        FixMessage message;
        ValidationResult validation;
        std::set<int> duplicate_tags; // Tags that appeared multiple times
        bool has_duplicates = false;
    };

    class Validator {
    public:
        ValidationResult validate(const std::string &raw_msg, const FixMessage &message) const;
        
        // Enhanced validation with duplicate detection during parsing
        ParseResult validateWithParse(const std::string &raw_msg) const;

    private:
        void validate_header(const FixMessage &message, ValidationResult &result) const;
        void validate_body(const FixMessage &message, ValidationResult &result) const;
        void validate_body_length(const std::string &raw_msg, const FixMessage &message, ValidationResult &result) const;
        void validate_checksum(const std::string &raw_msg, const FixMessage &message, ValidationResult &result) const;
        void validate_duplicates(const std::set<int>& duplicates, ValidationResult &result) const;
        int compute_checksum(const std::string &processed_msg) const;
        int calculateBodyLength(const std::string &raw_msg) const;
    };
}
