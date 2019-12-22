#include "project.hpp"
#include "constants.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <variant>
#include <algorithm>
#include <cctype>

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QDir>
#include <QDirIterator>

namespace eptg {

std::set<std::string> Project::get_common_tags(const std::set<const taggable*> & taggables) const
{
    std::set<std::string> result;
    if (taggables.size() == 0)
        return result;

    auto it = taggables.begin();
    auto end = taggables.end();

    // get tags for 1st taggable as baseline
    if (*it == nullptr)
        return std::set<std::string>();
    result = (*it)->inherited_tags;

    // filter out tags that are not in subsequent taggables
    for ( ++it
        ; it != end && result.size() > 0
        ; ++it)
    {
        if (*it == nullptr)
            return std::set<std::string>();
        decltype(result) tmp;
        std::set_intersection(result.begin(), result.end(),
                              (*it)->inherited_tags.begin(), (*it)->inherited_tags.end(),
                              std::inserter(tmp, tmp.end()));
        result = tmp;
    }

    return result;
}

std::unique_ptr<Project> Load(const std::string & full_path)
{
    std::unique_ptr<Project> project = std::make_unique<Project>(full_path);

    // sweep directory
    QString qfullpath = QString::fromStdString(full_path);
    QDir base_path(qfullpath);
    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.gif"};
    QDirIterator it(QString::fromStdString(full_path), filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        project->files.insert(base_path.relativeFilePath(it.next()).toStdString(), File());

    // read json file
    QFile file(PathAppend(QString(full_path.c_str()), PROJECT_FILE_NAME));
    file.open(QIODevice::ReadOnly);
    QByteArray rawData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(rawData));
    QJsonObject json = doc.object();

    const QJsonObject files = json["files"].toObject();
    for (const auto & filename : files.keys())
    {
        if ( ! project->files.has(filename.toStdString())) // prune deleted files
            continue;
        File & file = project->files.insert(filename.toStdString(), File());
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
            {
                file.insert_tag(tagname.toString().toStdString());
                project->tags.insert(tagname.toString().toStdString(), Tag());
            }
    }
    const QJsonObject tags = json["tags"].toObject();
    for (const auto & tag_name : tags.keys())
    {
        Tag & tag = project->tags.insert(tag_name.toStdString(), Tag());
        const auto & t = tags[tag_name];
        for (const auto subtagname : t.toObject()["tags"].toArray())
            if (subtagname.toString().size() > 0)
                tag.insert_tag(subtagname.toString().toStdString());
    }
    return project;
}

void Save(const std::unique_ptr<Project> & project)
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
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            files_json.insert(QString(id.c_str()), file_json);
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
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            tags_json.insert(QString(id.c_str()), file_json);
        }
        json_document.insert("tags", tags_json);
    }

    QFile file(PathAppend(QString(project->path.c_str()), PROJECT_FILE_NAME));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(json_document).toJson());
    file.flush();
    file.close();
}

bool Project::absorb(const Project & sub_project)
{
    if ( ! PathIsSub(QString::fromStdString(path), QString::fromStdString(sub_project.path)))
        return false;

    QString sub_project_rel_path = QDir(QString::fromStdString(path)).relativeFilePath(QString::fromStdString(sub_project.path));

    std::set<std::string> tags_used;
    for (const auto & [rel_path,file] : sub_project.files.collection)
    {
        const std::string new_rel_path = PathAppend(sub_project_rel_path, QString::fromStdString(rel_path)).toStdString();
        files.insert(new_rel_path, eptg::taggable(file));
        tags_used.insert(file.inherited_tags.begin(), file.inherited_tags.end());
    }

    std::set<std::string> tags_used_done;
    while( ! tags_used.empty())
    {
        const std::string & tag = *tags_used.begin();
        const eptg::Tag * tag_in_sub_project = sub_project.tags.find(tag);
              eptg::Tag & tag_in_new_project =       this->tags.insert(tag, eptg::Tag{});

        if (tag_in_sub_project)
            for (const std::string & inherited_tag : tag_in_sub_project->inherited_tags)
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

bool Project::inherits(const eptg::taggable & taggable, std::set<std::string> tags_to_have) const
{
    for ( std::set<std::string> tags = taggable.inherited_tags
        ; tags.size() > 0 && tags_to_have.size() > 0
        ; tags = get_common_tags(this->tags.get_all_by_name(tags)) )
    {
        for (const std::string & tag : tags)
            tags_to_have.erase(tag);
    }
    return tags_to_have.size() == 0;
}
std::map<std::string,const File*> Project::get_files_tagged_with_all(const std::set<std::string> & tags) const
{
    std::map<std::string,const File*> result;
    for (const auto & [id,f] : files.collection)
        if (inherits(f, tags))
            result.insert(std::make_pair(id,&f));
    return result;
}

std::vector<std::string> Project::search(const SearchNode & search_node) const
{
    std::vector<std::string> matches;

    for (const auto & p : this->files.collection)
        if (search_node.eval(this->get_all_inherited_tags({&p.second})))
            matches.push_back(p.first);

    return matches;
}

std::set<std::string> Project::get_all_inherited_tags(const std::set<const taggable*> & taggables) const
{
    std::set<std::string> result{};
    for (const taggable * taggable : taggables)
        for (const std::string & tag : taggable->inherited_tags)
            result.insert(tag).second;
    for (std::set<std::string> new_tags = result ; ! new_tags.empty() ; )
    {
        const std::string tag = *new_tags.begin();
        new_tags.erase(new_tags.begin());
        for (const std::string & tag : get_all_inherited_tags(this->tags.get_all_by_name({tag})))
            if (result.insert(tag).second)
                new_tags.insert(tag);
    }
    return result;
}

std::vector<std::vector<std::string>> Project::_get_tag_paths(const std::string & tag, const std::string & top_tag) const
{
    std::vector<std::vector<std::string>> result;

    const taggable * t = tags.find(tag);
    if ( ! t)
        return result;

    if (t->has_tag(top_tag))
        return {{}};

    for (const std::string & tag : t->inherited_tags)
        for (std::vector<std::string> & path : _get_tag_paths(tag, top_tag))
        {
            path.insert(path.begin(), tag);
            result.push_back(std::move(path));
        }
    return result;
}

std::vector<std::vector<std::string>> Project::get_tag_paths(const std::string & rel_path, const std::string & top_tag) const
{
    std::vector<std::vector<std::string>> result;

    const taggable * f = files.find(rel_path);
    if ( ! f)
        return result;

    if (f->has_tag(top_tag))
        return {};

    for (const std::string & tag : f->inherited_tags)
        for (std::vector<std::string> & path : _get_tag_paths(tag, top_tag))
        {
            path.insert(path.begin(), tag);
            result.push_back(std::move(path));
        }
    return result;
}

std::vector<std::vector<std::string>> get_similar(const eptg::Project & project, int allowed_difference, std::function<bool(size_t,size_t)> callback)
{
    std::vector<std::vector<std::string>> result;

    size_t count = 0;
    std::vector<std::tuple<std::string, QImage, double, bool>> thumbs;

    // read all images, resize, calculate avg luminosity
    for (auto it=project.files.collection.begin(),end=project.files.collection.end() ; it!=end ; ++it)
    {
        QString full_path = PathAppend(QString::fromStdString(project.path), QString::fromStdString(it->first));
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
        [](const decltype(thumbs)::value_type & left, const decltype(thumbs)::value_type & right)
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
