#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <algorithm>
#include <variant>

#if __has_include(<filesystem>)
#   include <filesystem>
#   define has_stdfs 1
    namespace stdfs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#   include <experimental/filesystem>
#   define has_stdfs 1
    namespace stdfs = std::experimental::filesystem;
#else
#   define has_stdfs 0
#endif

#include <QImage>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QDir>
#include <QDirIterator>

#include "eptg/search.hpp"
#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"
#include "eptg/path.hpp"

namespace eptg {

template<typename STR>
struct taggable
{
    std::set<STR> inherited_tags;

    inline void  insert_tag(const STR & tag)       {        inherited_tags.insert(tag); }
    inline size_t erase_tag(const STR & tag)       { return inherited_tags.erase (tag); }
    inline bool     has_tag(const STR & tag) const { return inherited_tags.find  (tag) != inherited_tags.end(); }
    template<typename C>
    bool has_all_of(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const STR & t){ return ! in(inherited_tags, t); }) == tags.end();
    }
    template<typename C>
    bool has_one_of(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const STR & t){ return in(inherited_tags, t); }) != tags.end();
    }
};

template<typename T> using File = taggable<T>;
template<typename T> using Tag  = taggable<T>;

template<typename STR, typename T>
struct taggable_collection
{
    std::map<STR,T> collection;

    inline size_t size() const { return collection.size(); }
    inline size_t count_tagged() const
    {
        return std::count_if(collection.begin(), collection.end(), [](const auto & p) { return p.second.inherited_tags.size() > 0; });
    }

    const T * find(const STR & id) const
    {
        auto it = collection.find(id);
        if (it == collection.end())
            return nullptr;
        return &it->second;
    }
    T * find  (const STR & id)         { return const_cast<T*>(const_cast<const taggable_collection<STR,T>*>(this)->find(id)); }
    T & insert(const STR & id, T && t) { return collection.insert({id, t}).first->second; }
    bool has  (const STR & id) const   { return collection. find(id) != collection.end(); }
    bool erase(const STR & id)         { return collection.erase(id)>0; }
    std::map<STR,T> get_tagged_with_all(const std::set<STR> & tags) const
    {
        std::map<STR,T> result;
        for (const auto & p : collection)
        {
            if ( ! p.second.has_all_of(tags))
                continue;
            result.insert(p);
        }
        return result;
    }
    std::set<const taggable<STR>*> get_all_by_name(const std::set<STR> & ids) const
    {
        std::set<const taggable<STR>*> result;
        for (const STR & id : ids)
        {
            const T * t = find(id);
            if (t != nullptr)
                result.insert(t);
        }
        return result;
    }
};

template<typename STR>
struct Project
{
    STR path;
    taggable_collection<STR,File<STR>> files;
    taggable_collection<STR,Tag <STR>> tags ;

    inline Project(const STR & full_path)
        : path(full_path)
    {}

    bool absorb(const Project & sub_project)
    {
        if ( ! Path::is_sub(path, sub_project.path))
            return false;

        STR sub_project_rel_path = QDir(path).relativeFilePath(sub_project.path);

        std::set<STR> tags_used;
        for (const auto & [rel_path,file] : sub_project.files.collection)
        {
            const STR new_rel_path = Path::append(sub_project_rel_path, rel_path);
            files.insert(new_rel_path, eptg::taggable(file));
            tags_used.insert(file.inherited_tags.begin(), file.inherited_tags.end());
        }

        std::set<STR> tags_used_done;
        while( ! tags_used.empty())
        {
            const STR & tag = *tags_used.begin();
            const eptg::Tag<STR> * tag_in_sub_project = sub_project.tags.find(tag);
                  eptg::Tag<STR> & tag_in_new_project =       this->tags.insert(tag, eptg::Tag<STR>{});

            if (tag_in_sub_project)
                for (const STR & inherited_tag : tag_in_sub_project->inherited_tags)
                {
                    tag_in_new_project.insert_tag(inherited_tag);
                    if (tags_used_done.find(inherited_tag) == tags_used_done.end())
                        tags_used.insert(inherited_tag);
                }

            tags_used_done.insert(tag);
            tags_used.erase(tag);
        }

        return true;
    }

    template<typename C>
    std::set<STR> get_descendent_tags(const C & p_tags) const
    {
        std::set<STR> result;
        for (const auto & [name,tag] : this->tags.collection)
            if (tag.has_one_of(p_tags))
                result.insert(name);
        return result;
    }
    std::map<STR,const File<STR>*> get_files_tagged_with_all(const std::set<STR> & tags) const
    {
        std::map<STR,const File<STR>*> result;
        for (const auto & [id,f] : files.collection)
            if (inherits(f, tags))
                result.insert({id, &f});
        return result;
    }
    std::set<STR> get_common_tags(const std::set<const taggable<STR>*> & taggables) const
    {
        std::set<STR> result;
        if (taggables.size() == 0)
            return result;

        auto it = taggables.begin();
        auto end = taggables.end();

        // get tags for 1st taggable as baseline
        if (*it == nullptr)
            return std::set<STR>();
        result = (*it)->inherited_tags;

        // filter out tags that are not in subsequent taggables
        for ( ++it
            ; it != end && result.size() > 0
            ; ++it)
        {
            if (*it == nullptr)
                return std::set<STR>();
            decltype(result) tmp;
            std::set_intersection(result.begin(), result.end(),
                                  (*it)->inherited_tags.begin(), (*it)->inherited_tags.end(),
                                  std::inserter(tmp, tmp.end()));
            result = tmp;
        }

        return result;
    }
    bool inherits(const taggable<STR> & taggable, std::set<STR> tags_to_have) const
    {
        for ( std::set<STR> tags = taggable.inherited_tags
            ; tags.size() > 0 && tags_to_have.size() > 0
            ; tags = get_common_tags(this->tags.get_all_by_name(tags)) )
        {
            for (const STR & tag : tags)
                tags_to_have.erase(tag);
        }
        return tags_to_have.size() == 0;
    }

    std::vector<STR> search(const SearchNode<STR> & search_node) const
    {
        std::vector<STR> matches;

        for (const auto & p : this->files.collection)
            if (search_node.eval(this->get_all_inherited_tags({&p.second})))
                matches.push_back(p.first);

        return matches;
    }
    std::set<STR> get_all_inherited_tags(const std::set<const taggable<STR>*> & taggables) const
    {
        std::set<STR> result{};
        for (const taggable<STR> * taggable : taggables)
            for (const STR & tag : taggable->inherited_tags)
                result.insert(tag).second;
        for (std::set<STR> new_tags = result ; ! new_tags.empty() ; )
        {
            const STR tag = *new_tags.begin();
            new_tags.erase(new_tags.begin());
            for (const STR & tag : get_all_inherited_tags(this->tags.get_all_by_name({tag})))
                if (result.insert(tag).second)
                    new_tags.insert(tag);
        }
        return result;
    }
    std::vector<std::vector<STR>> _get_tag_paths(const STR & tag, const STR & top_tag) const
    {
        std::vector<std::vector<STR>> result;

        const taggable<STR> * t = tags.find(tag);
        if ( ! t)
            return result;

        if (t->has_tag(top_tag))
            return {{}};

        for (const STR & tag : t->inherited_tags)
            for (std::vector<STR> & path : _get_tag_paths(tag, top_tag))
            {
                path.insert(path.begin(), tag);
                result.push_back(std::move(path));
            }
        return result;
    }
    std::vector<std::vector<STR>> get_tag_paths(const STR & rel_path, const STR & top_tag) const
    {
        std::vector<std::vector<STR>> result;

        const taggable<STR> * f = files.find(rel_path);
        if ( ! f)
            return result;

        if (f->has_tag(top_tag))
            return {};

        for (const STR & tag : f->inherited_tags)
            for (std::vector<STR> & path : _get_tag_paths(tag, top_tag))
            {
                path.insert(path.begin(), tag);
                result.push_back(std::move(path));
            }
        return result;
    }
};

template<typename STR>
std::unique_ptr<Project<STR>> load(const STR & full_path)
{
    std::unique_ptr<Project<STR>> project = std::make_unique<Project<STR>>(full_path);

    // sweep directory
    QDir base_path(full_path);
    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.gif"};
    QDirIterator it(full_path, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        project->files.insert(base_path.relativeFilePath(it.next()), File<STR>());

    // read json file
    QFile file(Path::append(full_path, PROJECT_FILE_NAME));
    file.open(QIODevice::ReadOnly);
    QByteArray rawData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(rawData));
    QJsonObject json = doc.object();

    const QJsonObject files = json["files"].toObject();
    for (const auto & filename : files.keys())
    {
        if ( ! project->files.has(filename)) // prune deleted files
            continue;
        File<STR> & file = project->files.insert(filename, File<STR>());
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
            {
                file.insert_tag(tagname.toString());
                project->tags.insert(tagname.toString(), Tag<STR>());
            }
    }
    const QJsonObject tags = json["tags"].toObject();
    for (const auto & tag_name : tags.keys())
    {
        Tag<STR> & tag = project->tags.insert(tag_name, Tag<STR>());
        const auto & t = tags[tag_name];
        for (const auto subtagname : t.toObject()["tags"].toArray())
            if (subtagname.toString().size() > 0)
                tag.insert_tag(subtagname.toString());
    }
    return project;
}

template<typename STR>
void save(const std::unique_ptr<Project<STR>> & project)
{
    QJsonObject json_document;
    {
        QJsonObject files_json;
        for (const auto & [id,file] : project->files.collection)
        {
            if (file.inherited_tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : file.inherited_tags)
                tags.insert(i++, QJsonValue(tag));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            files_json.insert(id, file_json);
        }
        json_document.insert("files", files_json);
    }
    {
        QJsonObject tags_json;
        for (const auto & [id,tag] : project->tags.collection)
        {
            if (tag.inherited_tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : tag.inherited_tags)
                tags.insert(i++, QJsonValue(tag));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            tags_json.insert(id, file_json);
        }
        json_document.insert("tags", tags_json);
    }

    QFile file(Path::append(project->path, PROJECT_FILE_NAME));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(json_document).toJson());
    file.flush();
    file.close();
}

template<typename STR>
std::vector<std::vector<STR>> get_similar(const Project<STR> & project, int allowed_difference, std::function<bool(size_t,size_t)> callback)
{
    std::vector<std::vector<STR>> result;

    size_t count = 0;
    std::vector<std::tuple<STR, QImage, double, bool>> thumbs;

    // read all images, resize, calculate avg luminosity
    for (auto it=project.files.collection.begin(),end=project.files.collection.end() ; it!=end ; ++it)
    {
        QString full_path = Path::append(project.path, it->first);
        QImage thumb = QImage(full_path).scaled(8, 8);
        double grad = 0;
        for (int y=0 ; y<8 ; y++)
            for (int x=0 ; x<8 ; x++)
            {
                QRgb rgb = thumb.pixel(x, y);
                grad += qRed  (rgb);
                grad += qGreen(rgb);
                grad += qBlue (rgb);
            }
        grad /= 3*8*8;
        thumbs.emplace_back(it->first, std::move(thumb), grad, false);

        count++;
        if ( ! callback(count, project.files.size()))
            return result;
    }

    // sort by
    std::sort(thumbs.begin(), thumbs.end(),
        [](const typename decltype(thumbs)::value_type & left, const typename decltype(thumbs)::value_type & right)
            {
                return std::get<2>(left) < std::get<2>(right);
            }
        );

    for (auto it=thumbs.begin(),end=thumbs.end() ; it!=end ; ++it)
    {
        if (std::get<3>(*it)) // already in a set
            continue;
        bool found_similar = false;
        auto grad = std::get<2>(*it);
        auto rel_path = std::get<0>(*it);
        for (auto it2=std::make_reverse_iterator(it),end2=thumbs.rend() ; it2!=end2 ; it2++)
        {
            if (std::get<3>(*it2)) // already in a set
                continue;
            if (std::get<2>(*it2) < grad-3)
                break;
            if (images_close(std::get<1>(*it), std::get<1>(*it2), allowed_difference))
            {
                if ( ! found_similar)
                {
                    result.push_back({});
                    result.back().push_back(rel_path);
                    found_similar = true;
                    std::get<3>(*it) = true;
                }
                std::get<3>(*it2) = true;
                result.back().push_back(std::get<0>(*it2));
            }
        }
        for (auto it2=std::next(it),end2=thumbs.end() ; it2!=end2 ; it2++)
        {
            if (std::get<3>(*it2)) // already in a set
                continue;
            if (std::get<2>(*it2) > grad+3)
                break;
            if (images_close(std::get<1>(*it), std::get<1>(*it2), allowed_difference))
            {
                if ( ! found_similar)
                {
                    result.push_back({});
                    result.back().push_back(rel_path);
                    found_similar = true;
                    std::get<3>(*it) = true;
                }
                std::get<3>(*it2) = true;
                result.back().push_back(std::get<0>(*it2));
            }
        }
    }
    return result;
}

} // namespace

#endif // include guard
