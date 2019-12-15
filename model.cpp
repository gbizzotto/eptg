#include "model.hpp"
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

QString PathAppend(const QString & path1, const QString & path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

std::set<std::string> Model::get_common_tags(const std::set<const taggable*> & taggables) const
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

std::unique_ptr<Model> Load(const std::string & full_path)
{
    std::unique_ptr<Model> model = std::make_unique<Model>(full_path);

    // sweep directory
    QString qfullpath = QString::fromStdString(full_path);
    QDir base_path(qfullpath);
    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.gif"};
    QDirIterator it(QString::fromStdString(full_path), filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        model->files.insert(base_path.relativeFilePath(it.next()).toStdString(), File());

    // read json file
    QFile file(PathAppend(QString(full_path.c_str()), "eptg.json"));
    file.open(QIODevice::ReadOnly);
    QByteArray rawData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(rawData));
    QJsonObject json = doc.object();

    const QJsonObject files = json["files"].toObject();
    for (const auto & filename : files.keys())
    {
        File & file = model->files.insert(filename.toStdString(), File());
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
            {
                file.insert_tag(tagname.toString().toStdString());
                model->tags.insert(tagname.toString().toStdString(), Tag());
            }
    }
    const QJsonObject tags = json["tags"].toObject();
    for (const auto & tag_name : tags.keys())
    {
        Tag & tag = model->tags.insert(tag_name.toStdString(), Tag());
        const auto & t = tags[tag_name];
        for (const auto subtagname : t.toObject()["tags"].toArray())
            if (subtagname.toString().size() > 0)
                tag.insert_tag(subtagname.toString().toStdString());
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
        for (const auto & [id,tag] : model->tags.collection)
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

    QFile file(PathAppend(QString(model->path.c_str()), "eptg.json"));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(json_document).toJson());
    file.flush();
    file.close();
}

bool Model::inherits(const eptg::taggable & taggable, std::set<std::string> tags_to_have) const
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
std::map<std::string,const File*> Model::get_files_tagged_with_all(const std::set<std::string> & tags) const
{
    std::map<std::string,const File*> result;
    for (const auto & [id,f] : files.collection)
        if (inherits(f, tags))
            result.insert(std::make_pair(id,&f));
    return result;
}

std::vector<std::string> Model::search(const SearchNode & search_node) const
{
    std::vector<std::string> matches;

    for (const auto & p : this->files.collection)
        if (search_node.eval(this->get_all_inherited_tags({&p.second})))
            matches.push_back(p.first);

    return matches;
}

std::set<std::string> Model::get_all_inherited_tags(const std::set<const taggable*> & taggables) const
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

//bool MyImage::close(const MyImage & other) const
//{
//    for (int x=0 ; x<10 ; x++)
//        for (int y=0 ; y<10 ; y++)
//                 if (std::get<0>(quadrant_colors[x][y]) - std::get<0>(other.quadrant_colors[x][y]) > 10)
//                return false;
//            else if (std::get<0>(quadrant_colors[x][y]) - std::get<0>(other.quadrant_colors[x][y]) < -10)
//                return false;
//             else if (std::get<1>(quadrant_colors[x][y]) - std::get<1>(other.quadrant_colors[x][y]) > 10)
//                 return false;
//             else if (std::get<1>(quadrant_colors[x][y]) - std::get<1>(other.quadrant_colors[x][y]) < -10)
//                 return false;
//             else if (std::get<2>(quadrant_colors[x][y]) - std::get<2>(other.quadrant_colors[x][y]) > 10)
//                 return false;
//             else if (std::get<2>(quadrant_colors[x][y]) - std::get<2>(other.quadrant_colors[x][y]) < -10)
//                 return false;
//    return true;
//}

} // namespace
