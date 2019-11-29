#include "model.hpp"
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QDir>

namespace eptg {

QString PathAppend(const QString & path1, const QString & path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

void taggable::insert_tag(const std::string & tag)
{
    tags.insert(tag);
}
void taggable::erase_tag(const std::string & tag)
{
    tags.erase(tag);
}
bool taggable::has_tag(const std::string & tag) const
{
    return tags.find(tag) != tags.end();
}

File::File(const std::string & rel_path)
    : rel_path(rel_path)
{}
Tag::Tag(const std::string & n)
    : name(n)
{}

Model::Model(const std::string & full_path)
    : path(full_path)
{}

File * Model::add_file(const std::string & rel_path)
{
    auto p = files.insert(std::make_pair(rel_path,File{rel_path}));
    return &p.first->second;
}
const File * Model::get_file(const std::string & rel_path) const
{
    auto it = files.find(rel_path);
    if (it == files.end())
        return nullptr;
    return &it->second;
}
File * Model::get_file(const std::string & rel_path)
{
    return const_cast<File*>(const_cast<const Model*>(this)->get_file(rel_path));
}
bool Model::has_file(const std::string & rel_path) const
{
    return files.find(rel_path) != files.end();
}
std::map<std::string,File> Model::get_files(const std::set<std::string> & tags)
{
    std::map<std::string,File> result;

    for (const auto & f : files)
        for (const auto & t : tags)
        {
            if (f.second.tags.find(t) == f.second.tags.end())
                break;
            result.insert(f);
        }

    return result;
}
void Model::insert_file(File && f)
{
    files.insert(std::make_pair(f.rel_path, f));
}

Tag * Model::add_tag(const std::string & name)
{
    auto p = tags.insert(std::make_pair(name,Tag{name}));
    return &p.first->second;
}
const Tag * Model::get_tag(const std::string & name) const
{
    auto it = tags.find(name);
    if (it == tags.end())
        return nullptr;
    return &it->second;
}
Tag * Model::get_tag(const std::string & name)
{
    return const_cast<Tag*>(const_cast<const Model*>(this)->get_tag(name));
}
bool Model::has_tag(const std::string & name) const
{
    return tags.find(name) != tags.end();
}

std::map<std::string,Tag> Model::get_tags(const std::set<std::string> & subtags)
{
    std::map<std::string,Tag> result;

    for (const auto & t : tags)
        for (const auto & st : subtags)
        {
            if (t.second.tags.find(st) == t.second.tags.end())
                break;
            result.insert(t);
        }

    return result;
}
void Model::insert_tag(Tag && t)
{
    tags.insert(std::make_pair(t.name, t));
}
std::vector<std::string> Model::get_parent_tags(const std::vector<std::string> & p_tags) const
{
    std::vector<std::string> result;
    std::set<std::string> tmp_result;
    for (const std::string & t : p_tags)
    {
        if ( ! has_tag(t))
            continue;
        const Tag *tag = get_tag(t);
        for (const std::string & parent_tag_str : tag->tags)
            tmp_result.insert(parent_tag_str);
    }
    for (const std::string & result_tag_str : tmp_result)
        result.push_back(result_tag_str);
    return result;
}

std::vector<std::string> Model::get_descendent_tags(const std::vector<std::string> & p_tags) const
{
    std::vector<std::string> result;
    std::set<std::string> tmp_result;
    for (const auto & [name,tag] : this->tags)
        for (const std::string & t : p_tags)
            if (tag.has_tag(t))
                tmp_result.insert(name);

    for (const std::string & result_tag_str : tmp_result)
        result.push_back(result_tag_str);
    return result;
}

std::unique_ptr<Model> Load(const std::string & full_path)
{
    QFile file(PathAppend(QString(full_path.c_str()), "eptg.json"));
    file.open(QIODevice::ReadOnly);
    QByteArray rawData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(rawData));
    QJsonObject json = doc.object();

    std::unique_ptr<Model> model(new Model(full_path));

    const QJsonObject files = json["files"].toObject();
    for (const auto & filename : files.keys())
    {
        File file{filename.toStdString()};
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
                file.insert_tag(tagname.toString().toStdString());
        if (file.tags.size() > 0)
            model->insert_file(std::move(file));
    }
    const QJsonObject tags = json["tags"].toObject();
    for (const auto & tag_name : tags.keys())
    {
        Tag tag{tag_name.toStdString()};
        const auto & t = tags[tag_name];
        for (const auto subtagname : t.toObject()["tags"].toArray())
            if (subtagname.toString().size() > 0)
                tag.insert_tag(subtagname.toString().toStdString());
        model->insert_tag(std::move(tag));
    }
    return model;
}

void Save(const std::unique_ptr<Model> & model)
{
    QJsonObject document;
    {
        QJsonObject files;
        for (const auto & file_model : model->files)
        {
            if (file_model.second.tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : file_model.second.tags)
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            files.insert(QString(file_model.second.rel_path.c_str()), file_json);
        }
        document.insert("files", files);
    }
    {
        QJsonObject tags_;
        for (const auto & tag_model : model->tags)
        {
            if (tag_model.second.tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : tag_model.second.tags)
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            tags_.insert(QString(tag_model.second.name.c_str()), file_json);
        }
        document.insert("tags", tags_);
    }

    QFile file(PathAppend(QString(model->path.c_str()), "eptg.json"));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(document).toJson());
    file.flush();
    file.close();
}

bool TaggableHasTag(const eptg::Model & model, const eptg::taggable & taggable, const std::string & tag)
{
    if (taggable.has_tag(tag))
        return true;
    for (const std::string & t : taggable.tags)
        if (model.has_tag(t))
            if (TaggableHasTag(model, *model.get_tag(t), tag))
                return true;
    return false;
}

} // namespace
