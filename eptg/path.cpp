
#include "eptg/path.hpp"
#include "eptg/constants.hpp"

#include <QDir>

namespace path {

QString append(const QString & path1, const QString & path2)
{
    if (path1.size() == 0)
        return path2;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}
bool is_sub(const QString & path, const QString & maybe_sub_path)
{
     return (QDir(maybe_sub_path).absolutePath().startsWith(QDir(path).absolutePath()));
}
QString up(QString path)
{
    if (path.endsWith("/"))
        path.chop(1);
    int idx = path.lastIndexOf("/");
    if (idx == -1)
        return "";
    path.truncate(idx);
    return path;
}
QString relative(const QString & base, const QString & sub)
{
    if ( ! is_sub(base, sub))
        return "";
    QString base_abs = QDir(base).absolutePath();
    if ( ! base_abs.endsWith('/'))
        base_abs.append('/');
    return QDir(sub).absolutePath().remove(0, base_abs.size());
}

std::string append(std::string path1, const std::string & path2)
{
    if (path1.empty())
        return path2;
    if (path2.empty() || path2.front() == PATH_SEPARATOR_CHAR)
        return path1;
    if (path1.back() != PATH_SEPARATOR_CHAR)
        path1.append(PATH_SEPARATOR_STR);
    return path1.append(path2);
}

bool is_sub(std::string path , std::string maybe_sub_path)
{
    if (path.empty())
        return true;
    if (path.back() != PATH_SEPARATOR_CHAR)
        path.append(PATH_SEPARATOR_STR);
    if (maybe_sub_path.back() != PATH_SEPARATOR_CHAR)
        maybe_sub_path.append(PATH_SEPARATOR_STR);
    if (maybe_sub_path.size() < path.size())
        return false;
    return maybe_sub_path.substr(0, path.size()) == path;
}
std::string up(std::string path)
{
    if (path.empty())
        return "";
    if (path.back() == PATH_SEPARATOR_CHAR)
        path.resize(path.size() - 1);
    auto pos = path.rfind(PATH_SEPARATOR_CHAR);
    if (pos == std::string::npos)
        return "";
    return path.substr(0, pos);
}
std::string relative(std::string base, const std::string & sub)
{
    if (base.empty())
        return sub;
    if (base.back() != PATH_SEPARATOR_CHAR)
        base.append(PATH_SEPARATOR_STR);
    if (sub.size() < base.size())
        return "";
    if (sub.substr(0, base.size()) != base)
        return "";
    return sub.substr(base.size());
}

} // namespace
