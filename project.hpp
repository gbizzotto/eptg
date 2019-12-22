#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <vector>
#include <string>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <algorithm>
#include <variant>

#include <QImage>

#include "search.hpp"
#include "helpers.hpp"

namespace eptg {

struct taggable
{
    std::set<std::string> inherited_tags;

    inline void insert_tag(const std::string & tag) { inherited_tags.insert(tag); }
    inline size_t erase_tag (const std::string & tag) { return inherited_tags.erase (tag); }
    inline bool has_tag   (const std::string & tag) const { return inherited_tags.find(tag) != inherited_tags.end(); }
    template<typename C>
    bool has_tags(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const std::string & t){ return ! in(inherited_tags, t); }) == tags.end();
    }
    template<typename C>
    bool has_one(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const std::string & t){ return in(inherited_tags, t); }) != tags.end();
    }
};

using File = taggable;
using Tag  = taggable;

template<typename T>
struct taggable_collection
{
    std::map<std::string,T> collection;

    inline size_t size() const { return collection.size(); }
    inline size_t count_tagged() const
    {
        return std::count_if(collection.begin(), collection.end(), [](const auto & p) { return p.second.inherited_tags.size() > 0; });
    }

    const T * find(const std::string & id) const
    {
        auto it = collection.find(id);
        if (it == collection.end())
            return nullptr;
        return &it->second;
    }
    T * find(const std::string & id) { return const_cast<T*>(const_cast<const taggable_collection<T>*>(this)->find(id)); }
    T & insert(const std::string & id, T && t) { return collection.insert(std::make_pair(id, t)).first->second; }
    bool has(const std::string & id) const { return collection.find(id) != collection.end(); }
    bool erase(const std::string & it) { return collection.erase(it)>0; }
    std::map<std::string,T> get_tagged_with_all(const std::set<std::string> & tags) const
    {
        std::map<std::string,T> result;
        for (const auto & p : collection)
        {
            if ( ! p.second.has_tags(tags))
                continue;
            result.insert(p);
        }
        return result;
    }
    std::set<const taggable*> get_all_by_name(const std::set<std::string> & ids) const
    {
        std::set<const taggable*> result;
        for (const std::string & id : ids)
        {
            const T * t = find(id);
            if (t != nullptr)
                result.insert(t);
        }
        return result;
    }
};

struct Project
{
    std::string path;
    taggable_collection<File> files;
    taggable_collection<Tag > tags ;

    inline Project(const std::string & full_path)
        : path(full_path)
    {}

    bool absorb(const Project & sub_project);
    template<typename C>
    std::set<std::string> get_descendent_tags(const C & p_tags) const
    {
        std::set<std::string> result;
        for (const auto & [name,tag] : this->tags.collection)
            if (tag.has_one(p_tags))
                result.insert(name);
        return result;
    }
    std::map<std::string,const File*> get_files_tagged_with_all(const std::set<std::string> & tags) const;
    std::set<std::string> get_common_tags(const std::set<const taggable*> & taggables) const;
    bool inherits(const eptg::taggable & taggable, std::set<std::string> tags_to_have) const;
    std::vector<std::string> search(const SearchNode & search_node) const;
    std::set<std::string> get_all_inherited_tags(const std::set<const taggable*> & taggables) const;
    std::vector<std::vector<std::string>> _get_tag_paths(const std::string & tag, const std::string & top_tag) const;
    std::vector<std::vector<std::string>> get_tag_paths(const std::string & rel_path, const std::string & top_tag) const;
};

std::unique_ptr<Project> Load(const std::string & full_path);
void Save(const std::unique_ptr<Project> & project);

std::vector<std::vector<std::string>> get_similar(const eptg::Project & project, int allowed_difference, std::function<bool(size_t,size_t)> callback);

} // namespace

#endif // include guard