#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <set>
#include <map>
#include <memory>

namespace eptg {

struct taggable
{
    const std::string id_;
    std::set<std::string> tags;

    inline taggable(const std::string & id)
        : id_(id)
    {}

    inline const std::string & id() { return id_; }

    void insert_tag(const std::string & tag);
    void erase_tag(const std::string & tag);
    bool has_tag(const std::string & tag) const;
};

struct File : public taggable
{
    inline File(const std::string & rel_path)
        : taggable(rel_path)
    {}
    inline const std::string & rel_path() { return id(); }
};

struct Tag : taggable
{
    inline Tag(const std::string & name)
        : taggable(name)
    {}
    inline const std::string & name() { return id(); }
};

template<typename T>
struct taggable_collection
{
    std::map<std::string,T> collection;

    inline size_t size() const { return collection.size(); }

    const T * get(const std::string & rel_path) const
    {
        auto it = collection.find(rel_path);
        if (it == collection.end())
            return nullptr;
        return &it->second;
    }
    T * get(const std::string & id)        { return const_cast<T*>(const_cast<const taggable_collection<T>*>(this)->get(id)); }
    T * add(const std::string & id)        { return &collection.insert(std::make_pair(id,T{id})).first->second; }
    void insert(T && t)                    { collection.insert(std::make_pair(t.id(), t)); }
    bool has(const std::string & id) const { return collection.find(id) != collection.end(); }
    std::map<std::string,T> get_tagged_with_all(const std::set<std::string> & tags)
    {
        std::map<std::string,T> result;
        for (const auto & elm : collection)
            for (const auto & t : tags)
            {
                if (elm.second.tags.find(t) == elm.second.tags.end())
                    break;
                result.insert(elm);
            }
        return result;
    }
    std::set<taggable*> get_all_by_name(const std::set<std::string> & ids)
    {
        std::set<taggable*> result;
        for (const std::string & id : ids)
        {
            auto it = collection.find(id);
            if (it != collection.end())
                result.insert(&it->second);
        }
        return result;
    }
};

struct Model
{
    std::string path;
    taggable_collection<File> files;
    taggable_collection<Tag > tags ;

    inline Model(const std::string & full_path)
        : path(full_path)
    {}
    // files
//    File * add_file(const std::string & rel_path);
//    File * get_file(const std::string & rel_path);
//    const File * get_file(const std::string & rel_path) const;
//    std::map<std::string,File> get_files(const std::set<std::string> & tags);
//    void insert_file(File && f);
//    bool has_file(const std::string & rel_path) const;
//    // tags
//    Tag * add_tag(const std::string & rel_path);
//    Tag * get_tag(const std::string & rel_path);
//    const Tag * get_tag(const std::string & rel_path) const;
//    std::map<std::string,Tag> get_tags(const std::set<std::string> & tags);
//    void insert_tag(Tag && f);
//    bool has_tag(const std::string & rel_path) const;

    std::vector<std::string> get_parent_tags(const std::vector<std::string> & tags) const;
    std::vector<std::string> get_descendent_tags(const std::vector<std::string> & tags) const;

    std::set<std::string> get_common_tags(std::set<taggable*> & taggable) const;
};

std::unique_ptr<Model> Load(const std::string & full_path);
void Save(const std::unique_ptr<Model> & model);

bool TaggableHasTag(const eptg::Model & model, const eptg::taggable & taggable, const std::string & tag);

} // namespace

#endif // MODEL_HPP
