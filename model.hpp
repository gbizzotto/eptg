#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <set>
#include <map>
#include <memory>

namespace eptg {

struct taggable
{
    std::set<std::string> tags;
    void insert_tag(const std::string & tag);
    void erase_tag(const std::string & tag);
    bool has_tag(const std::string & tag) const;
};

struct File : public taggable
{
    const std::string rel_path;
    File(const std::string & rel_path);
};

struct Tag : taggable
{
    const std::string name;
    Tag(const std::string & name);
};

struct Model
{
    std::string path;
    std::map<std::string,File> files;
    std::map<std::string,Tag> tags;

    Model(const std::string & full_path);
    // files
    File * add_file(const std::string & rel_path);
    File * get_file(const std::string & rel_path);
    const File * get_file(const std::string & rel_path) const;
    std::map<std::string,File> get_files(const std::set<std::string> & tags);
    void insert_file(File && f);
    bool has_file(const std::string & rel_path) const;
    // tags
    Tag * add_tag(const std::string & rel_path);
    Tag * get_tag(const std::string & rel_path);
    const Tag * get_tag(const std::string & rel_path) const;
    std::map<std::string,Tag> get_tags(const std::set<std::string> & tags);
    void insert_tag(Tag && f);
    bool has_tag(const std::string & rel_path) const;
    std::vector<std::string> get_parent_tags(const std::vector<std::string> & tags) const;
    std::vector<std::string> get_descendent_tags(const std::vector<std::string> & tags) const;
};

std::unique_ptr<Model> Load(const std::string & full_path);
void Save(const std::unique_ptr<Model> & model);

bool TaggableHasTag(const eptg::Model & model, const eptg::taggable & taggable, const std::string & tag);

} // namespace

#endif // MODEL_HPP
