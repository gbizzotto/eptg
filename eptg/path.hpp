#ifndef PATH_HPP
#define PATH_HPP

#include <set>
#include <string>
#include <deque>

#include <QString>
#include <QFileInfo>

#include "eptg/fs.hpp"
#include "eptg/string.hpp"
#include "eptg/in.hpp"

namespace path {

QString append  (const QString & path1, const QString & path2);
bool    is_sub  (const QString & path , const QString & maybe_sub_path);
QString up      (      QString   path);
QString relative(const QString & base, const     QString & sub);
bool is_relative(const QString & path);

std::string append  (const std::string path1, const std::string  & path2);
bool        is_sub  (const std::string path , const std::string  & maybe_sub_path);
std::string up      (      std::string path);
std::string relative(      std::string base, const std::string & sub);
bool     is_relative(const std::string & path);

template<typename STR>
bool parent_has(STR sweep_path, const STR & file_name)
{
    for ( ; sweep_path.size() > 0 ; sweep_path=up(sweep_path))
        if (QFileInfo(append(sweep_path, file_name)).exists())
            return true;
    return false;
}

template<typename STR>
bool are_same(STR left, STR right)
{
    if (left.size() > 0 && left[left.size()-1] != '/')
        left.append("/");
    if (right.size() > 0 && right[right.size()-1] != '/')
        right.append("/");
    return eptg::fs::path(eptg::str::to<std::string>(left)).compare(eptg::fs::path(eptg::str::to<std::string>(right))) == 0;
}

template<typename STR>
std::set<STR> sweep(const STR & full_path, const std::set<STR> & extensions)
{
    std::set<STR> result;

#if has_stdfs
    for (std::deque<eptg::fs::path> directories{{eptg::str::to<std::string>(full_path)}} ; ! directories.empty() ; directories.pop_front())
        try {
            for(eptg::fs::directory_iterator dit(directories.front()) ; dit != eptg::fs::directory_iterator() ; ++dit)
                if(eptg::fs::is_directory(dit->path()))
                    directories.push_back(dit->path());
                else if (eptg::fs::is_regular_file(dit->path()))
                    if (eptg::in(extensions, eptg::str::to_lower(eptg::str::to<STR>(dit->path().extension()))))
                        result.insert(relative(full_path, eptg::str::to<STR>(dit->path().string())));
        }
        catch(...)
        {}
#else
    decltype(extensions) extensions_for_qt;
    std::transform(extensions.begin(), extensions.end(), std::inserter(extensions_for_qt), [](const STR & str){ return STR("*").append(str); });
    QDir base_path(full_path);
    QDirIterator it(full_path, extensions_for_qt, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        result.insert(eptg::str::to<STR>(it.next()));
#endif

    return result;
}

} // namespace

#endif // PATH_HPP
