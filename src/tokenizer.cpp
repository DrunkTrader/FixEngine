#include "tokenizer.hpp"
#include <sstream>

namespace fix {
    std::string preprocess_delimeter(const std::string &input){
        std::string good_str = input;
        
        for(auto &ch : good_str){
            if(ch == '|') ch = soh_del;
        }
        return good_str;
    }

    std::vector<std::string> tokenize(const std::string &input){
        std::vector<std::string> tokens;
        std::size_t start = 0;

        while (start <= input.size()) {
            const std::size_t end = input.find(soh_del, start);
            if (end == std::string::npos) {
                if (start < input.size()) {
                    tokens.push_back(input.substr(start));
                }
                break;
            }
            if (end > start) {
                tokens.push_back(input.substr(start, end - start));
            }
            start = end + 1;
        }

        return tokens;
    }

    FixField splitField(const std::string &token){
        FixField field{-1, ""};
        auto pos = token.find('=');
        if (pos == std::string::npos) {
            return field;
        }

        try {
            field.tag = std::stoi(token.substr(0, pos));
            field.value = token.substr(pos + 1);
        } catch(const std::exception& e) {
            // Invalid conversion (e.g., tag is not a number)
            field.tag = -1;
            field.value = "";
        }
        return field;
    }
}