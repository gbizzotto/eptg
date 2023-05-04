#ifndef STRING_HPP
#define STRING_HPP

#include <string>
#include <vector>
#include <utility>

#include <set>
#include <QString>
#include <QStringList>

namespace eptg {
namespace str {


std::string substring(const std::string & str, size_t idx);
    QString substring(          QString   str,    int idx);
std::vector<std::string> split(const std::string & s, const char * separators = " \t", const char * ignore = "");
std::vector<    QString> split(const     QString & s, const char * separators = " \t", const char * ignore = "");
std::string to_lower(const std::string & str);
    QString to_lower(const     QString & str);

std::set<QString> unique_tokens(const QString & str);

template<typename STR>
STR to(const QString & str)
{
    return str.toStdString().c_str();
}
template<typename STR>
STR to(const std::string & str)
{
    return str.c_str();
}
template<typename STR>
STR to(const std::wstring & str)
{
    return str.c_str();
}

template<>     QString to<    QString>(const std:: string & str);
template<>     QString to<    QString>(const std::wstring & str);
template<>     QString to<    QString>(const      QString & str);
template<> std::string to<std::string>(const std:: string & str);
template<> std::string to<std::string>(const std::wstring & str);
template<> std::string to<std::string>(const      QString & str);
template<> std::wstring to<std::wstring>(const std:: string & str);
template<> std::wstring to<std::wstring>(const std::wstring & str);
template<> std::wstring to<std::wstring>(const      QString & str);

inline int value(const QChar   c) { return c.digitValue(); }
inline int value(const  char   c) { return (int)c; }
inline int value(const wchar_t c) { return (int)c; }

}} // namespaces


template<typename STR>
QStringList & operator+(QStringList & list, const STR & str)
{
    list.append(eptg::str::to<QString>(str));
    return list;
}

template<typename T>
QString & operator<<(QString & out, const T & t)
{
    return out.append(t);
}
template<>
QString & operator<<(QString & out, const std::string & t);


#endif // include_guard
