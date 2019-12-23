#ifndef JSON_HPP
#define JSON_HPP

#include <map>
#include <vector>
#include <variant>

namespace eptg
{

template<typename STR>
struct json_dict;
template<typename STR>
struct json_array;

template<typename STR>
using json_var = std::variant<json_dict<STR>,json_array<STR>,int,float,STR>;

template<typename STR>
struct json_dict : public std::map<STR,json_var<STR>>
{
};

template<typename STR>
struct json_array : public std::vector<json_var<STR>>
{
};

template<typename STR>
json_dict<STR> json_read_dict(const char *& c);
template<typename STR>
json_array<STR> json_read_array(const char *& c);
template<typename STR>
STR json_read_string(const char *& c);

inline int json_read_int(const char *& c)
{
    return std::atoi(c);
    while ((*c >= 0 && *c <= 9) || *c == '-')
        c++;
}
inline void json_skip_separators(const char *& c)
{
    while (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r')
        c++;
    if (*c == 0)
        throw;
}

template<typename STR>
json_var<STR> json_read_var(const char *& c)
{
    json_skip_separators(c);
    if (*c == '{')
        return json_read_dict<STR>(c);
    else if (*c == '[')
        return json_read_array<STR>(c);
    else if (*c == '"')
        return json_read_string<STR>(c);
    else if ((*c >= 0 && *c <= 9) || *c == '-')
        return json_read_int(c);
    else
        throw;
}

template<typename STR>
STR json_read_string(const char *& c)
{
    STR str;
    if (*c != '"')
        throw;
    for (c++ ; *c != '"' ; c++)
    {
        if (*c == 0)
            throw;
        str.append(*c);
    }
    c++; // skip last '"'
    return str;
}

int json_read_int(const char *& c);

template<typename STR>
json_dict<STR> json_read_dict(const char *& c)
{
    json_skip_separators(c);

    if (*c != '{')
        throw;

    json_dict<STR> result;
    for (c++ ; *c != '}' ; )
    {
        json_skip_separators(c);
        if (*c != '"')
            throw;
        STR key = json_read_string<STR>(c);
        json_skip_separators(c);
        if (*c != ':')
            throw;
        c++; // skip ':'
        json_skip_separators(c);
        result.insert({key, json_read_var<STR>(c)});
        json_skip_separators(c);
        if (*c == ',')
            c++;
    }
    c++; // skip '}'
    return result;
}

template<typename STR>
json_array<STR> json_read_array(const char *& c)
{
    json_skip_separators(c);

    if (*c != '[')
        throw;

    json_array<STR> result;
    for (c++ ; *c != ']' ; )
    {
        json_skip_separators(c);
        result.push_back(json_read_var<STR>(c));
        json_skip_separators(c);
        if (*c == ',')
            c++;
    }
    c++; // skip '}'
    return result;
}

template<typename STR>
STR make_indent(int indent)
{
    STR result;
    for (int i=0 ; i<indent ; i++)
        result.append("\t");
    return result;
}

template<typename STR>
STR json_to_str(const STR & str)
{
    return STR("\"").append(str).append("\"");
}
template<typename STR>
STR json_to_str(const int v)
{
    return STR(std::to_string(v).c_str());
}
template<typename STR>
STR json_to_str(const json_dict<STR> & dict, int indent=0)
{
    STR result;
    result.append("{\n");
    indent++;
    for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
    {
        result.append(make_indent<STR>(indent));
        result.append(json_to_str(it->first));
        result.append(": ");
        result.append(json_to_str(it->second, indent+1));
        ++it;
        if (it != end)
            result.append(",");
        result.append("\n");
    }
    indent--;
    result.append(make_indent<STR>(indent));
    result.append("}");
    return result;
}
template<typename STR>
STR json_to_str(const json_array<STR> & dict, int indent=0)
{
    STR result;
    result.append("[\n");
    indent++;
    for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
    {
        result.append(make_indent<STR>(indent));
        result.append(json_to_str(*it));
        ++it;
        if (it != end)
            result.append(",");
        result.append("\n");
    }
    indent--;
    result.append(make_indent<STR>(indent));
    result.append("]");
    return result;
}

template<typename STR>
STR json_to_str(const json_var<STR> & var, int indent=0)
{
    if (std::holds_alternative<int>(var))
        return json_to_str<STR>(std::get<int>(var));
    else if (std::holds_alternative<STR>(var))
        return json_to_str<STR>(std::get<STR>(var));
    else if (std::holds_alternative<json_dict<STR>>(var))
        return json_to_str(std::get<json_dict<STR>>(var), indent);
    else if (std::holds_alternative<json_array<STR>>(var))
        return json_to_str(std::get<json_array<STR>>(var), indent);
    else
        return "";
}

} // namespace

#endif // include guard
