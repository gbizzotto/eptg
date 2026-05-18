#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#include <map>
#include <string>
#include <vector>

struct FaceRect
{
    int x;
    int y;
    int w;
    int h;
};

struct CachedFace
{
    int centerX;
    int centerY;

    std::string name;

    bool ignored;
};

struct FaceInfo
{
    FaceRect rect;

    std::string name;

    double confidence;

    bool ignored;
    bool is_set;
    bool matches_ai;
};

class FaceLib
{
public:

    FaceLib();

    bool initialize(
        const std::string& cascadeFile,
        const std::string& dataDir);

    std::vector<FaceInfo> detectFaces(
        const std::string filename);

    bool markFace(
        const std::string filename,
        std::vector<FaceInfo>& faces,
        int faceIndex,
        const std::string& name);

    bool markFace(
        const std::string& filename,
        const FaceRect& rect,
        const std::string& name);

    bool ignoreFace(
        const std::string filename,
        std::vector<FaceInfo> & faces,
        int faceIndex);

    bool getMarkedFaces(
        const std::string filename,
        std::vector<FaceInfo>& outFaces);

    int findFace(
        const std::vector<FaceInfo>& faces,
        int x,
        int y);

    bool save();
    bool load();

private:

    cv::CascadeClassifier detector;

    cv::Ptr<cv::face::LBPHFaceRecognizer> recognizer;

    std::string databaseDir;

    int nextLabel;

    std::map<int, std::string> labelToName;

    std::map<std::string, int> nameToLabel;

    std::map<
        std::string,
        std::vector<CachedFace>
    > cachedFaces;

    std::map<
        std::string,
        std::vector<FaceInfo>
    > cachedAddedFaces;

private:

    cv::Mat preprocessFace(
        const cv::Mat& gray,
        const cv::Rect& rect);

    bool trainRecognizer();

    bool saveLabels();
    bool loadLabels();

    bool saveCache();
    bool loadCache();
    bool saveCacheAdded();
    bool loadCacheAdded();

    bool saveRecognizer();
    bool loadRecognizer();
};
