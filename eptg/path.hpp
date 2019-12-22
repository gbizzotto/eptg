#ifndef PATH_HPP
#define PATH_HPP

#include <string>
#include <QString>
#include <QFileInfo>

namespace Path {

QString append(const QString & path1, const QString & path2);
bool    is_sub(const QString & path , const QString & maybe_sub_path);
QString up    (QString path);

std::string append(const std::string path1, const std::string  & path2);
bool        is_sub(const std::string path , const std::string  & maybe_sub_path);
std::string up    (std::string  path);

template<typename STR>
bool parent_has(STR sweep_path, const STR & file_name)
{
    for ( ; sweep_path.size() > 0 ; sweep_path=up(sweep_path))
        if (QFileInfo(Path::append(sweep_path, file_name)).exists())
            return true;
    return false;
}

} // namespace

#endif // PATH_HPP
