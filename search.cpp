
#include "search.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <assert.h>
#include <QString>

std::vector<std::string> split(const std::string & s, const char * separators, const char * ignore)
{
    const char *str = s.c_str();

    std::vector<std::string> result;
    while (*str)
    {
        while(*str && in(ignore,*str))
            str++;
        if ( ! *str)
            break;
        if (in(separators,*str))
        {
            result.push_back(std::string(str, str+1));
            str++;
            continue;
        }
        const char *begin = str;
        while(*str && ! in(separators, *str) && ! in(ignore, *str))
            str++;
        result.push_back(std::string(begin, str));
    }
    return result;
}

std::vector<QString> split(const QString & s, const char * separators, const char * ignore)
{
    std::vector<QString> result;
    for (const std::string & s : split(s.toStdString(), separators, ignore))
        result.push_back(QString::fromStdString(s));
    return result;
}

std::string to_lower(const std::string & str)
{
    std::string istr;
    std::transform(str.begin(), str.end(), std::back_inserter(istr), [](const char c){ return std::tolower(c); });
    return istr;
}

QString to_lower(const QString & str)
{
    return str.toLower();
}
