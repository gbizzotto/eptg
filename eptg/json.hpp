#ifndef JSON_HPP
#define JSON_HPP

#include <map>
#include <vector>
#include <variant>

namespace eptg {
namespace json {

template<typename STR>
struct dict;
template<typename STR>
struct array;

template<typename STR>
using var = std::variant<dict<STR>,array<STR>,int,float,STR>;

template<typename STR>
struct dict : public std::map<STR,var<STR>>
{
};

template<typename STR>
struct array : public std::vector<var<STR>>
{
};

template<typename STR>
dict<STR> read_dict(const char *& c);
template<typename STR>
array<STR> read_array(const char *& c);
template<typename STR>
STR read_string(const char *& c);

inline int read_int(const char *& c)
{
    return std::atoi(c);
    while ((*c >= 0 && *c <= 9) || *c == '-')
        c++;
}
inline void skip_separators(const char *& c)
{
    while (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r')
        c++;
    if (*c == 0)
        throw;
}

template<typename STR>
var<STR> read_var(const char *& c)
{
    skip_separators(c);
    if (*c == '{')
        return read_dict<STR>(c);
    else if (*c == '[')
        return read_array<STR>(c);
    else if (*c == '"')
        return read_string<STR>(c);
    else if ((*c >= 0 && *c <= 9) || *c == '-')
        return read_int(c);
    else
        throw;
}

template<typename STR>
STR read_string(const char *& c)
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


template<typename STR>
dict<STR> read_dict(const char *& c)
{
    skip_separators(c);

    if (*c != '{')
        throw;

    dict<STR> result;
    for (c++ ; *c != '}' ; )
    {
        skip_separators(c);
        if (*c != '"')
            throw;
        STR key = read_string<STR>(c);
        skip_separators(c);
        if (*c != ':')
            throw;
        c++; // skip ':'
        skip_separators(c);
        result.insert({key, read_var<STR>(c)});
        skip_separators(c);
        if (*c == ',')
            c++;
    }
    c++; // skip '}'
    return result;
}

template<typename STR>
array<STR> read_array(const char *& c)
{
    skip_separators(c);

    if (*c != '[')
        throw;

    array<STR> result;
    for (c++ ; *c != ']' ; )
    {
        skip_separators(c);
        result.push_back(read_var<STR>(c));
        skip_separators(c);
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
STR to_str(const STR & str)
{
    return STR("\"").append(str).append("\"");
}
template<typename STR>
STR to_str(const int v)
{
    return STR(std::to_string(v).c_str());
}
template<typename STR>
STR to_str(const dict<STR> & dict, int indent=0)
{
    STR result;
    result.append("{\n");
    indent++;
    for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
    {
        result.append(make_indent<STR>(indent));
        result.append(to_str(it->first));
        result.append(": ");
        result.append(to_str(it->second, indent+1));
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
STR to_str(const array<STR> & dict, int indent=0)
{
    STR result;
    result.append("[\n");
    indent++;
    for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
    {
        result.append(make_indent<STR>(indent));
        result.append(to_str(*it));
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
STR to_str(const var<STR> & var, int indent=0)
{
    if (std::holds_alternative<int>(var))
        return to_str<STR>(std::get<int>(var));
    else if (std::holds_alternative<STR>(var))
        return to_str<STR>(std::get<STR>(var));
    else if (std::holds_alternative<dict<STR>>(var))
        return to_str(std::get<dict<STR>>(var), indent);
    else if (std::holds_alternative<array<STR>>(var))
        return to_str(std::get<array<STR>>(var), indent);
    else
        return "";
}

}} // namespace

#endif // include guard
