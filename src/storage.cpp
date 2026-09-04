#include "storage.hpp"
#include <sstream>

namespace fix {
    FixMessage fix_field_storage(const std::string &raw_str){
        FixMessage fields; // Declare the fields map
        std::stringstream ss(raw_str);
        std::string token;

        while(std::getline(ss, token, '\x01')){
            auto pos = token.find('=');
            if(pos == std::string::npos) continue;

            int tag = std::stoi(token.substr(0,pos));
            std::string value = token.substr(pos + 1);

            fields[tag] = value;
        }
        return fields;
    }
}