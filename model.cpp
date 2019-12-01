#include "model.hpp"
#include <algorithm>
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

std::set<std::string> Model::get_common_tags(const std::set<taggable*> & taggables) const
{
    std::set<std::string> result;
    if (taggables.size() == 0)
        return result;

    // get tags for 1st taggable as baseline
    auto it = taggables.begin();
    auto end = taggables.end();
    for ( ; it != end ; ++it)
    {
        if (*it == nullptr)
            return std::set<std::string>();
        result = (*it)->tags;
        break;
    }
    for ( ++it
        ; it != end && result.size() > 0
        ; ++it)
    {
        if (*it == nullptr)
            return std::set<std::string>();
        decltype(result) tmp;
        std::set_intersection(result.begin(), result.end(),
                              (*it)->tags.begin(), (*it)->tags.end(),
                              std::inserter(tmp, tmp.end()));
        result = tmp;
    }
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
        File file;
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
                file.insert_tag(tagname.toString().toStdString());
        if (file.tags.size() > 0)
            model->files.insert(filename.toStdString(), std::move(file));
    }
    const QJsonObject tags = json["tags"].toObject();
    for (const auto & tag_name : tags.keys())
    {
        Tag tag{};
        const auto & t = tags[tag_name];
        for (const auto subtagname : t.toObject()["tags"].toArray())
            if (subtagname.toString().size() > 0)
                tag.insert_tag(subtagname.toString().toStdString());
        model->tags.insert(tag_name.toStdString(), std::move(tag));
    }
    return model;
}

void Save(const std::unique_ptr<Model> & model)
{
    QJsonObject json_document;
    {
        QJsonObject files_json;
        for (const auto & [id,file] : model->files.collection)
        {
            if (file.tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : file.tags)
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            files_json.insert(QString(id.c_str()), file_json);
        }
        json_document.insert("files", files_json);
    }
    {
        QJsonObject tags_json;
        for (const auto & [id,tag] : model->tags.collection)
        {
            if (tag.tags.size() == 0)
                continue;
            QJsonArray tags;
            int i=0;
            for (const auto & tag : tag.tags)
                tags.insert(i++, QJsonValue(QString(tag.c_str())));
            QJsonObject file_json;
            file_json.insert("tags", tags);
            tags_json.insert(QString(id.c_str()), file_json);
        }
        json_document.insert("tags", tags_json);
    }

    QFile file(PathAppend(QString(model->path.c_str()), "eptg.json"));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(json_document).toJson());
    file.flush();
    file.close();
}

bool Model::inherits(const eptg::taggable & taggable, const std::string & tag) const
{
    if (taggable.has_tag(tag))
        return true;
    for (const std::string & t : taggable.tags)
        if (this->tags.has(t))
            if (this->inherits(*this->tags.get(t), tag))
                return true;
    return false;
}

} // namespace
