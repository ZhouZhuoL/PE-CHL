#ifndef QT_UTILS_H
#define QT_UTILS_H
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <string>
#include <fstream>
#include <iostream>
// 文件夹操作
#include <vector>
#include "groundRemove/include/cloud.h"
#include <QImage>

namespace utils
{
   /**
   * @brief Initialize from 3d points.
   *
   * @param[in]  points  The points
   */
    void ReadKittiBinCloudByPath(const std::string & path, Cloud & cloud);
    Cloud::Ptr ReadKittiBinCloudByPath(const std::string & path);

    void ReadKittiImageByPath(const std::string & path, cv::Mat & img);
    
    void ReadKittiFileByDir(const std::string & dir, std::vector<std::string> &fileNames);

    QImage MatToQImage(const cv::Mat &image);

}
#endif