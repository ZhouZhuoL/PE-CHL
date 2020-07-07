#include "utils.h"
#include <boost/filesystem.hpp>
#include <vector>

namespace fs = boost::filesystem;
namespace utils
{

    void ReadKittiBinCloudByPath(const std::string & path, Cloud & cloud)
    {
        std::fstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (file.good())
        {
            file.seekg(0, std::ios::beg);
            float intensity = 0;
            for (int i = 0; file.good() && !file.eof(); ++i)
            {
                point pt;
                file.read(reinterpret_cast<char*>(&pt.x()), sizeof(float));
                file.read(reinterpret_cast<char*>(&pt.y()), sizeof(float));
                file.read(reinterpret_cast<char*>(&pt.z()), sizeof(float));
                file.read(reinterpret_cast<char*>(&intensity), sizeof(float));
                cloud.emplace_back(pt);
            }
            file.close();
        }
    }
    Cloud::Ptr ReadKittiBinCloudByPath(const std::string & path)
    {
        Cloud::Ptr cloud(new Cloud);
        std::fstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (file.good())
        {
            file.seekg(0, std::ios::beg);
            float intensity = 0;
            for (int i = 0; file.good() && !file.eof(); ++i)
            {
                point pt;
                file.read(reinterpret_cast<char*>(&pt.x()), sizeof(float));
                file.read(reinterpret_cast<char*>(&pt.y()), sizeof(float));
                file.read(reinterpret_cast<char*>(&pt.z()), sizeof(float));
                file.read(reinterpret_cast<char*>(&intensity), sizeof(float));
                cloud->emplace_back(pt);
            }
            file.close();
        }
        return cloud;
    }

    void ReadKittiImageByPath(const std::string & path, cv::Mat & img)
    {
        // fprintf(stderr, "path: %s\n", path.c_str());
        img = cv::imread(path);
        if (img.empty())
        {
            printf("no kitti image was readed!\n");
            return;
        }
    }

    void ReadKittiFileByDir(const std::string & dir, std::vector<std::string> &fileNames)
    {
        // printf("kitti velodyne path :%s\n", dir.c_str());
        fs::path velo_path(dir);
        fs::directory_iterator end;
        for (fs::directory_iterator fileIt(velo_path); fileIt != end; ++fileIt)
        {
            std::string filename =  fileIt->path().string();
            fileNames.emplace_back(filename);         
        }
    }

    QImage MatToQImage(const cv::Mat &image) 
    {
        auto qimage = QImage(image.cols, image.rows, QImage::Format_RGB888);
        if (image.type() == CV_32F) 
        {
            for (int r = 0; r < image.rows; ++r) 
            {
                for (int c = 0; c < image.cols; ++c) 
                {
                    if (image.at<float>(r, c) == 666) 
                    {
                        auto color = qRgb(0, 200, 0);
                        qimage.setPixel(c, r, color);
                        continue;
                    }
                    const float &val = image.at<float>(r, c) * 10;
                    auto color = qRgb(val, val, val);
                    qimage.setPixel(c, r, color);
                }
            }
        } 
        else 
        {
            for (int r = 0; r < image.rows; ++r) 
            {
                for (int c = 0; c < image.cols; ++c) 
                {
                    auto val = image.at<cv::Vec3b>(r, c);
                    auto color = qRgb(val[0], val[1], val[2]);
                    qimage.setPixel(c, r, color);
                }
            }
        }
        return qimage;
    }
}