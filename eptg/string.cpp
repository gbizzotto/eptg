
#include <QStringList>

#include "eptg/string.hpp"
#include "eptg/in.hpp"

namespace eptg {
namespace str {


std::string substring(const std::string & str, size_t idx)
{
    return str.substr(idx);
}
QString substring(QString str, int idx)
{
    return str.remove(0, idx);
}

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

template<>
QString to<QString>(const std::string & str)
{
    return QString::fromStdString(str);
}
template<>
QString to<QString>(const QString & str)
{
    return str;
}
template<>
std::string to<std::string>(const std::string & str)
{
    return str;
}
template<>
std::string to<std::string>(const QString & str)
{
    return str.toStdString();
}
template<>
std::wstring to<std::wstring>(const std::string & str)
{
	return std::wstring(str.begin(), str.end());
}
template<>
std::wstring to<std::wstring>(const QString & str)
{
	return str.toStdWString();
}

std::set<QString> unique_tokens(const QString & str)
{
    std::set<QString> typed_tags;
    for (const QString & tag : str.split(' '))
    {
        if (tag.size() == 0)
            continue;
        typed_tags.insert(tag);
    }
    return typed_tags;
}


}} // namespaces


template<>
QString & operator<<(QString & out, const std::string & t)
{
    return out.append(QString::fromStdString(t));
}
