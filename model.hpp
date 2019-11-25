#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <set>
#include <map>
#include <memory>

namespace eptg {

struct File
{
    const std::string rel_path;
    std::set<std::string> tags;

    File(const std::string & rel_path);
    void insert_tag(const std::string & tag);
    void erase_tag(const std::string & tag);
};

struct Model
{
    std::string path;
    std::map<std::string,File> files;

    Model(const std::string & full_path);
    File & get_file(const std::string & rel_path);
    std::map<std::string,File> get_files(const std::set<std::string> & tags);
};

std::unique_ptr<Model> Load(const std::string & full_path);
void Save(const std::unique_ptr<Model> & model);

} // namespace

#endif // MODEL_HPP
