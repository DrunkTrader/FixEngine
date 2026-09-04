#include "parser.hpp"

namespace fix {
    FixMessage Parser::parse(const std::string& raw) {
        FixMessage fixmsg_store;

        // 1. Preprocess the raw message
        std::string processed_msg = preprocess_delimeter(raw);

        // 2. Tokenize the string
        std::vector<std::string> tokens = tokenize(processed_msg);

        // 3. Store the split fields into our map
        for (const auto& token : tokens) {
            FixField field = splitField(token);
            if (field.tag != -1) { 
                // Overwrite behavior for duplicates by default using map index operator
                fixmsg_store[field.tag] = field.value;
            }
        }

        return fixmsg_store;
    }
}
