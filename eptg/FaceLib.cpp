#include "FaceLib.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>

namespace fs = std::filesystem;

FaceLib::FaceLib()
{
    recognizer =
        cv::face::LBPHFaceRecognizer::create();

    nextLabel = 0;
}

bool FaceLib::initialize(
    const std::string& cascadeFile,
    const std::string& dataDir)
{
    databaseDir = dataDir;

    fs::create_directories(dataDir);

    if (!detector.load(cascadeFile))
    {
        return false;
    }

    load();

    return true;
}

cv::Mat FaceLib::preprocessFace(
    const cv::Mat& gray,
    const cv::Rect& rect)
{
    cv::Mat face = gray(rect).clone();

    cv::resize(
        face,
        face,
        cv::Size(200,200));

    cv::equalizeHist(face, face);

    return face;
}

std::vector<FaceInfo>
FaceLib::detectFaces(
    const std::string filename)
{
    cv::Mat img =
        cv::imread(filename);

    std::vector<FaceInfo> result;

    if (img.empty())
    {
        return result;
    }

    cv::Mat gray;

    cv::cvtColor(
        img,
        gray,
        cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;

    detector.detectMultiScale(
        gray,
        faces,
        1.1,
        5,
        0,
        cv::Size(80,80));

    for (const auto& r : faces)
    {
        FaceInfo f;

        f.rect.x = r.x;
        f.rect.y = r.y;
        f.rect.w = r.width;
        f.rect.h = r.height;

        f.name = "unknown";

        f.confidence = -1.0;

        f.ignored = false;

        if (!labelToName.empty())
        {
            cv::Mat face =
                preprocessFace(
                    gray,
                    r);

            int predictedLabel = -1;

            double confidence = 0;

            recognizer->predict(
                face,
                predictedLabel,
                confidence);

            if (predictedLabel >= 0)
            {
                auto it =
                    labelToName.find(
                        predictedLabel);

                if (it !=
                    labelToName.end())
                {
                    f.name =
                        it->second;
                }

                f.confidence =
                    confidence;

                f.is_set = false;
                f.matches_ai = false;
            }
        }

        result.push_back(f);
    }

    auto cacheIt =
        cachedFaces.find(filename);

    if (cacheIt !=
        cachedFaces.end())
    {
        for (const auto& cached :
             cacheIt->second)
        {
            int index =
                findFace(
                    result,
                    cached.centerX,
                    cached.centerY);

            if (index < 0 ||
                index >=
                (int)result.size())
            {
                continue;
            }

            result[index].is_set = true;
            result[index].matches_ai = result[index].name == cached.name;
            result[index].name = cached.name;

            result[index].ignored =
                cached.ignored;
        }
    }

    auto cacheAddedIt =
        cachedAddedFaces.find(filename);

    if (cacheAddedIt !=
        cachedAddedFaces.end())
    {
        for (const auto& cached :
             cacheAddedIt->second)
        {
            result.push_back(cached);
        }
    }

    return result;
}

bool FaceLib::markFace(
    const std::string filename,
    std::vector<FaceInfo>& faces,
    int faceIndex,
    const std::string& name)
{
    if (faceIndex < 0 ||
        faceIndex >= (int)faces.size())
    {
        return false;
    }

    int label;

    auto it =
        nameToLabel.find(name);

    if (it == nameToLabel.end())
    {
        label = nextLabel++;

        nameToLabel[name] = label;
        labelToName[label] = name;
    }
    else
    {
        label = it->second;
    }

    cv::Mat img = cv::imread(filename);

    if (img.empty())
        return false;

    cv::Mat gray;

    cv::cvtColor(
        img,
        gray,
        cv::COLOR_BGR2GRAY);


    cv::Mat face =
        preprocessFace(
            gray,
            cv::Rect(faces[faceIndex].rect.x, faces[faceIndex].rect.y, faces[faceIndex].rect.w, faces[faceIndex].rect.h));

    std::vector<cv::Mat> images;
    std::vector<int> labels;

    images.push_back(face);
    labels.push_back(label);

    recognizer->update(
        images,
        labels);

    faces[faceIndex].name = name;

    const auto& r =
        faces[faceIndex].rect;

    CachedFace c;

    c.centerX =
        r.x + r.w / 2;

    c.centerY =
        r.y + r.h / 2;

    c.name = name;

    c.ignored = false;

    cachedFaces[filename]
        .push_back(c);

    return true;
}
bool FaceLib::markFace(
    const std::string& filename,
    const FaceRect& rect,
    const std::string& name)
{
    cv::Mat img =
        cv::imread(filename);

    if (img.empty())
    {
        return false;
    }

    cv::Mat gray;

    cv::cvtColor(
        img,
        gray,
        cv::COLOR_BGR2GRAY);

    cv::Rect r(
        rect.x,
        rect.y,
        rect.w,
        rect.h);

    if (r.x < 0 ||
        r.y < 0 ||
        r.x + r.width > gray.cols ||
        r.y + r.height > gray.rows)
    {
        return false;
    }

    int label;

    auto it =
        nameToLabel.find(name);

    if (it == nameToLabel.end())
    {
        label =
            nextLabel++;

        nameToLabel[name] =
            label;

        labelToName[label] =
            name;
    }
    else
    {
        label =
            it->second;
    }

    cv::Mat face =
        preprocessFace(
            gray,
            r);

    std::vector<cv::Mat> images;
    std::vector<int> labels;

    images.push_back(face);
    labels.push_back(label);

    recognizer->update(
        images,
        labels);

    FaceInfo cached;

    cached.confidence = 1.0;
    cached.ignored = false;
    cached.is_set = 1;
    cached.name = name;
    cached.rect = rect;
    cachedAddedFaces[filename].push_back(cached);

    return true;
}

bool FaceLib::ignoreFace(
    const std::string filename,
    std::vector<FaceInfo>& faces,
    int faceIndex)
{
    if (faceIndex < 0 ||
        faceIndex >= (int)faces.size())
    {
        return false;
    }

    const auto& r =
        faces[faceIndex].rect;

    CachedFace c;

    c.centerX =
        r.x + r.w / 2;

    c.centerY =
        r.y + r.h / 2;

    c.name = "__ignored__";

    c.ignored = true;

    cachedFaces[filename]
        .push_back(c);

    return true;
}

bool FaceLib::getMarkedFaces(
    const std::string filename,
    std::vector<FaceInfo>& outFaces)
{
    auto cacheIt =
        cachedFaces.find(filename);

    if (cacheIt ==
        cachedFaces.end())
    {
        return false;
    }

    auto detected =
        detectFaces(filename);

    for (const auto& cached :
         cacheIt->second)
    {
        int index =
            findFace(
                detected,
                cached.centerX,
                cached.centerY);

        if (index < 0 ||
            index >=
            (int)detected.size())
        {
            continue;
        }

        detected[index].name =
            cached.name;

        detected[index].ignored =
            cached.ignored;

        outFaces.push_back(
            detected[index]);
    }

    return true;
}

int FaceLib::findFace(
    const std::vector<FaceInfo>& faces,
    int x,
    int y)
{
    int bestIndex = -1;

    double bestDistance =
        999999999.0;

    for (size_t i = 0;
         i < faces.size();
         ++i)
    {
        if (faces[i].ignored)
            continue;

        const auto& r =
            faces[i].rect;

        bool inside =
            x >= r.x &&
            y >= r.y &&
            x < (r.x + r.w) &&
            y < (r.y + r.h);

        if (!inside)
        {
            continue;
        }

        double cx =
            r.x + r.w * 0.5;

        double cy =
            r.y + r.h * 0.5;

        double dx =
            cx - x;

        double dy =
            cy - y;

        double dist =
            dx * dx +
            dy * dy;

        if (dist < bestDistance)
        {
            bestDistance =
                dist;

            bestIndex =
                (int)i;
        }
    }

    return bestIndex;
}

bool FaceLib::saveRecognizer()
{
    recognizer->save(
        databaseDir +
        "/recognizer.yml");

    return true;
}

bool FaceLib::loadRecognizer()
{
    std::string file =
        databaseDir +
        "/recognizer.yml";

    if (!fs::exists(file))
    {
        return true;
    }

    recognizer->read(file);

    return true;
}

bool FaceLib::saveLabels()
{
    std::ofstream out(
        databaseDir +
        "/labels.txt");

    if (!out.is_open())
    {
        return false;
    }

    out << nextLabel
        << std::endl;

    for (const auto& p : labelToName)
    {
        out
            << p.first
            << " "
            << p.second
            << std::endl;
    }

    return true;
}

bool FaceLib::loadLabels()
{
    labelToName.clear();
    nameToLabel.clear();

    std::ifstream in(
        databaseDir +
        "/labels.txt");

    if (!in.is_open())
    {
        return true;
    }

    in >> nextLabel;

    int label;
    std::string name;

    while (in >> label >> name)
    {
        labelToName[label] = name;
        nameToLabel[name] = label;
    }

    return true;
}

bool FaceLib::saveCache()
{
    std::ofstream out(
        databaseDir +
        "/cache.txt");

    if (!out.is_open())
    {
        return false;
    }

    for (const auto& pair :
         cachedFaces)
    {
        out
            << "FILE "
            << pair.first
            << std::endl;

        for (const auto& f :
             pair.second)
        {
            out
                << f.centerX << " "
                << f.centerY << " "
                << f.name << " "
                << f.ignored
                << std::endl;
        }
    }

    return true;
}
bool FaceLib::saveCacheAdded()
{
    std::ofstream out(
        databaseDir +
        "/cache_added.txt");

    if (!out.is_open())
    {
        return false;
    }

    for (const auto& pair :
         cachedAddedFaces)
    {
        out
            << "FILE "
            << pair.first
            << std::endl;

        for (const auto& f :
             pair.second)
        {
            out
                << f.rect.x << " "
                << f.rect.y << " "
                << f.rect.w << " "
                << f.rect.h << " "
                << f.name << " "
                << std::endl;
        }
    }

    return true;
}

bool FaceLib::loadCache()
{
    cachedFaces.clear();

    std::ifstream in(
        databaseDir +
        "/cache.txt");

    if (!in.is_open())
    {
        return true;
    }

    std::string line;

    std::string currentFile;

    while (std::getline(in, line))
    {
        if (line.rfind("FILE ",0)==0)
        {
            currentFile =
                line.substr(5);

            continue;
        }

        std::stringstream ss(line);

        CachedFace c;

        ss
            >> c.centerX
            >> c.centerY
            >> c.name
            >> c.ignored;

        cachedFaces[currentFile]
            .push_back(c);
    }

    return true;
}
bool FaceLib::loadCacheAdded()
{
    cachedAddedFaces.clear();

    std::ifstream in(
        databaseDir +
        "/cache_added.txt");

    if (!in.is_open())
    {
        return true;
    }

    std::string line;

    std::string currentFile;

    while (std::getline(in, line))
    {
        if (line.rfind("FILE ",0)==0)
        {
            currentFile =
                line.substr(5);

            continue;
        }

        std::stringstream ss(line);

        FaceInfo c;

        ss
            >> c.rect.x
            >> c.rect.y
            >> c.rect.w
            >> c.rect.h
            >> c.name;

        c.ignored = false;
        c.is_set = true;
        c.confidence = 1.0;
        c.matches_ai = false;

        cachedAddedFaces[currentFile].push_back(c);
    }

    return true;
}

bool FaceLib::save()
{
    return
        saveRecognizer() &&
        saveLabels() &&
        saveCacheAdded() &&
        saveCache();
}

bool FaceLib::load()
{
    return
        loadRecognizer() &&
        loadLabels() &&
        loadCache() &&
        loadCacheAdded();
}
