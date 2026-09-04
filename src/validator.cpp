#include "validator.hpp"
#include "tokenizer.hpp"
#include <unordered_map>
#include <sstream>
#include <algorithm>

namespace fix {
    
    ValidationResult Validator::validate(const std::string &raw_msg, const FixMessage &message) const {
        ValidationResult result;
        validate_header(message, result);
        validate_body(message, result);
        validate_body_length(raw_msg, message, result);
        validate_checksum(raw_msg, message, result);
        result.ok = result.errors.empty();
        return result;
    }

    ParseResult Validator::validateWithParse(const std::string &raw_msg) const {
        ParseResult parse_result;
        
        // Preprocess and tokenize
        std::string processed_msg = preprocess_delimeter(raw_msg);
        std::vector<std::string> tokens = tokenize(processed_msg);
        
        // Parse with duplicate detection
        std::set<int> seen_tags;
        for (const auto& token : tokens) {
            FixField field = splitField(token);
            if (field.tag != -1) {
                if (seen_tags.count(field.tag) > 0) {
                    parse_result.duplicate_tags.insert(field.tag);
                    parse_result.has_duplicates = true;
                } else {
                    seen_tags.insert(field.tag);
                    parse_result.message[field.tag] = field.value;
                }
            }
        }
        
        // Validate
        parse_result.validation = validate(raw_msg, parse_result.message);
        
        // Add duplicate errors
        validate_duplicates(parse_result.duplicate_tags, parse_result.validation);
        
        return parse_result;
    }

    void Validator::validate_header(const FixMessage &message, ValidationResult &result) const {
        const int required_tags[] = {8, 9, 35, 49, 56, 34, 52};
        for (const int tag : required_tags) {
            if (message.find(tag) == message.end()) {
                ValidationError err;
                err.message = "Missing required header tag: " + std::to_string(tag);
                err.tag = tag;
                result.errors.push_back(err);
            }
        }
    }

    void Validator::validate_body(const FixMessage &message, ValidationResult &result) const {
        auto msg_type_it = message.find(35);
        if (msg_type_it == message.end()) {
            ValidationError err;
            err.message = "Missing MsgType (35) for body validation";
            err.tag = 35;
            result.errors.push_back(err);
            return;
        }

        const std::string &msg_type = msg_type_it->second;
        static const std::unordered_map<std::string, std::vector<int>> required_by_type = {
            {"D", {11, 55, 54, 38, 40}},
            {"A", {98, 108}},
            {"8", {37, 17, 39, 150, 55, 54, 38}}
        };

        auto rule_it = required_by_type.find(msg_type);
        if (rule_it == required_by_type.end()) {
            ValidationError err;
            err.message = "No body validation rules for MsgType: " + msg_type;
            err.tag = 35;
            err.field_value = msg_type;
            result.errors.push_back(err);
            return;
        }

        for (const int tag : rule_it->second) {
            if (message.find(tag) == message.end()) {
                ValidationError err;
                err.message = "Missing required body tag for MsgType " + msg_type + ": " + std::to_string(tag);
                err.tag = tag;
                result.errors.push_back(err);
            }
        }
    }

    void Validator::validate_body_length(const std::string &raw_msg, const FixMessage &message, ValidationResult &result) const {
        auto body_length_it = message.find(9);
        if (body_length_it == message.end()) {
            ValidationError err;
            err.message = "Missing BodyLength tag (9)";
            err.tag = 9;
            result.errors.push_back(err);
            return;
        }

        int provided_body_length = -1;
        try {
            provided_body_length = std::stoi(body_length_it->second);
        } catch (const std::exception &) {
            ValidationError err;
            err.message = "Invalid BodyLength value in tag 9: " + body_length_it->second;
            err.tag = 9;
            err.field_value = body_length_it->second;
            result.errors.push_back(err);
            return;
        }

        int calculated_body_length = calculateBodyLength(raw_msg);
        if (calculated_body_length < 0) {
            ValidationError err;
            err.message = "Could not calculate body length from message";
            result.errors.push_back(err);
            return;
        }

        if (provided_body_length != calculated_body_length) {
            ValidationError err;
            std::ostringstream oss;
            oss << "BodyLength mismatch: tag 9 specifies " << provided_body_length 
                << ", but actual body length is " << calculated_body_length;
            err.message = oss.str();
            err.tag = 9;
            err.field_value = body_length_it->second;
            result.errors.push_back(err);
        }
    }

    void Validator::validate_checksum(const std::string &raw_msg, const FixMessage &message, ValidationResult &result) const {
        auto checksum_it = message.find(10);
        if (checksum_it == message.end()) {
            ValidationError err;
            err.message = "Missing checksum tag (10)";
            err.tag = 10;
            result.errors.push_back(err);
            return;
        }

        std::string processed = preprocess_delimeter(raw_msg);
        int expected_checksum = compute_checksum(processed);
        if (expected_checksum < 0) {
            ValidationError err;
            err.message = "Could not compute checksum (missing tag 10 in raw message)";
            err.tag = 10;
            result.errors.push_back(err);
            return;
        }

        int provided_checksum = -1;
        try {
            provided_checksum = std::stoi(checksum_it->second);
        } catch (const std::exception &) {
            ValidationError err;
            err.message = "Invalid checksum value in tag 10: " + checksum_it->second;
            err.tag = 10;
            err.field_value = checksum_it->second;
            result.errors.push_back(err);
            return;
        }

        if (expected_checksum != provided_checksum) {
            ValidationError err;
            std::ostringstream oss;
            oss << "Checksum mismatch: expected " << expected_checksum 
                << ", got " << provided_checksum;
            err.message = oss.str();
            err.tag = 10;
            err.field_value = checksum_it->second;
            result.errors.push_back(err);
        }
    }

    void Validator::validate_duplicates(const std::set<int>& duplicates, ValidationResult &result) const {
        for (int tag : duplicates) {
            ValidationError err;
            std::ostringstream oss;
            oss << "Duplicate tag detected: " << tag << " (FIX does not allow duplicate tags)";
            err.message = oss.str();
            err.tag = tag;
            result.errors.push_back(err);
        }
    }

    int Validator::compute_checksum(const std::string &processed_msg) const {
        const std::string checksum_tag = "10=";
        const std::size_t tag_pos = processed_msg.find(checksum_tag);
        if (tag_pos == std::string::npos) {
            return -1;
        }

        int sum = 0;
        for (std::size_t i = 0; i < tag_pos; ++i) {
            sum += static_cast<unsigned char>(processed_msg[i]);
        }
        return sum % 256;
    }

    int Validator::calculateBodyLength(const std::string &raw_msg) const {
        std::string processed = preprocess_delimeter(raw_msg);
        
        // Find the position of tag 9= (BodyLength)
        const std::string body_length_tag = "9=";
        std::size_t tag9_pos = processed.find(body_length_tag);
        if (tag9_pos == std::string::npos) {
            return -1;
        }
        
        // Find the start of the body (after tag 9's value and its delimiter)
        std::size_t value_start = tag9_pos + body_length_tag.length();
        std::size_t value_end = processed.find(soh_del, value_start);
        if (value_end == std::string::npos) {
            return -1;
        }
        
        // The body starts after the delimiter following tag 9's value
        std::size_t body_start = value_end + 1;
        
        // Find tag 10= (CheckSum)
        const std::string checksum_tag = "10=";
        std::size_t tag10_pos = processed.find(checksum_tag, body_start);
        if (tag10_pos == std::string::npos) {
            return -1;
        }
        
        // Body length is from body_start to just before tag 10=
        // This includes everything from tag 35 to tag 10 (excluding the final delimiter before 10)
        std::size_t body_end = tag10_pos;
        if (body_end > body_start && processed[body_end - 1] == soh_del) {
            body_end -= 1;
        }
        
        return static_cast<int>(body_end - body_start);
    }
}
