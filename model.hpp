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
    template<typename C>
    bool has_tags(const C & tags) const
    {
        for (const std::string & t : tags)
            if (tags.find(t) == tags.end())
                return false;
        return true;
    }
};

using File = taggable;
using Tag  = taggable;

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
    T * add(const std::string & id)        { return &collection.insert(std::make_pair(id,T{})).first->second; }
    void insert(const std::string & id, T && t) { collection.insert(std::make_pair(id, t)); }
    bool has(const std::string & id) const { return collection.find(id) != collection.end(); }
    std::map<std::string,T> get_tagged_with_all(const std::set<std::string> & tags)
    {
        std::map<std::string,T> result;
        for (const auto & elm : collection)
            if (elm.has_tags(tags))
                result.insert(elm);
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
    template<typename C>
    std::set<std::string> get_parent_tags(const C & p_tags) const
    {
        std::set<std::string> result;
        for (const std::string & tag_name : p_tags)
        {
            const Tag *tag = tags.get(tag_name);
            if (tag == nullptr)
                continue;
            for (const std::string & parent_tag_str : tag->tags)
                result.insert(parent_tag_str);
        }
        return result;
    }
    template<typename C>
    std::set<std::string> get_descendent_tags(const C & p_tags) const
    {
        std::set<std::string> result;
        for (const auto & [name,tag] : this->tags.collection)
            for (const std::string & t : p_tags)
                if (tag.has_tag(t))
                    result.insert(name);
        return result;
    }

    std::set<std::string> get_common_tags(const std::set<taggable*> & taggable) const;
    bool inherits(const eptg::taggable & taggable, const std::string & tag) const;
};

std::unique_ptr<Model> Load(const std::string & full_path);
void Save(const std::unique_ptr<Model> & model);

} // namespace

#endif // MODEL_HPP
