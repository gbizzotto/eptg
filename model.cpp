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

File::File(const std::string & rel_path)
    : rel_path(rel_path)
{}

File & Model::get_file(const std::string & rel_path)
{
    auto it = files.lower_bound(rel_path);
    if (it == files.end() || it->second.rel_path != rel_path)
        it = files.insert(it, std::make_pair(rel_path, File(rel_path)));
    return it->second;
}

void File::insert_tag(const std::string & tag)
{
    tags.insert(tag);
}
void File::erase_tag(const std::string & tag)
{
    tags.erase(tag);
}
bool File::has_tag(const std::string & tag) const
{
    return tags.find(tag) != tags.end();
}

Model::Model(const std::string & full_path)
    : path(full_path)
{}

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
        File & file = model->get_file(filename.toStdString());
        const auto & f = files[filename];
        for (const auto tagname : f.toObject()["tags"].toArray())
            if (tagname.toString().size() > 0)
                file.insert_tag(tagname.toString().toStdString());
    }
    return model;
}

void Save(const std::unique_ptr<Model> & model)
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
    QJsonObject document;
    document.insert("files", files);

    QFile file(PathAppend(QString(model->path.c_str()), "eptg.json"));
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(document).toJson());
    file.flush();
    file.close();
}

} // namespace
