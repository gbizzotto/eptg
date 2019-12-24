#ifndef FS_HPP
#define FS_HPP

#if __has_include(<filesystem>)
#   include <filesystem>
#   define has_stdfs 1
    namespace eptg {
    namespace fs = std::filesystem;
    }
#elif __has_include(<experimental/filesystem>)
#   include <experimental/filesystem>
#   define has_stdfs 1
    namespace eptg {
    namespace fs = std::experimental::filesystem;
    }
#else
#   define has_stdfs 0
#   include <QStringList>
#   include <QDir>
#   include <QDirIterator>
#endif

#endif // FS_HPP
