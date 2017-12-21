#pragma once

#include <json/json.h>
#include <memory>
#include <string>

namespace nabto {

class JsonHelper {
 public:
    static bool parse(const std::string& input, Json::Value& doc) {
        Json::CharReaderBuilder builder;
        std::istringstream is;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

        std::string errors;
        return reader->parse(input.c_str(), input.c_str() + input.size(), &doc, &errors);
    }
    
    static bool parse(const std::string& input, Json::Value& doc, std::string& errors) {
        Json::CharReaderBuilder builder;
        std::istringstream is;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

        return reader->parse(input.c_str(), input.c_str() + input.size(), &doc, &errors);
    }

    static std::string toString(const Json::Value& doc) {
        std::stringstream ss;
        ss << doc;
        return ss.str();
    }
};

} // namespace
