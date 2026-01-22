/**
 * @file FileIO.cpp
 * @brief File I/O utility implementation
 */

#include "util/FileIO.h"
#include "util/Logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace FileIO
{
    std::string ReadTextFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR("[FileIO] Failed to open file: {}", path);
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool WriteTextFile(const std::string &path, const std::string &content)
    {
        try
        {
            // Create parent directories if they don't exist
            std::filesystem::path filePath(path);
            std::filesystem::path parentDir = filePath.parent_path();
            if (!parentDir.empty() && !std::filesystem::exists(parentDir))
            {
                std::filesystem::create_directories(parentDir);
            }

            std::ofstream file(path);
            if (!file.is_open())
            {
                LOG_ERROR("[FileIO] Failed to open file for writing: {}", path);
                return false;
            }

            file << content;
            file.close();

            if (file.fail())
            {
                LOG_ERROR("[FileIO] Failed to write to file: {}", path);
                return false;
            }

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("[FileIO] Exception while writing file {}: {}", path, e.what());
            return false;
        }
    }

    bool FileExists(const std::string &path)
    {
        return std::filesystem::exists(path);
    }

    std::vector<std::string> ListFiles(const std::string &dirPath, const std::string &extension)
    {
        std::vector<std::string> files;

        try
        {
            if (!std::filesystem::exists(dirPath))
            {
                LOG_WARNING("[FileIO] Directory does not exist: {}", dirPath);
                return files;
            }

            if (!std::filesystem::is_directory(dirPath))
            {
                LOG_WARNING("[FileIO] Path is not a directory: {}", dirPath);
                return files;
            }

            for (const auto &entry : std::filesystem::directory_iterator(dirPath))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    // Check if file has the requested extension
                    if (extension.empty() || filename.size() >= extension.size())
                    {
                        std::string fileExt = filename.substr(filename.size() - extension.size());
                        if (fileExt == extension)
                        {
                            files.push_back(filename);
                        }
                    }
                }
            }

            // Sort filenames for consistent ordering
            std::sort(files.begin(), files.end());
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("[FileIO] Exception while listing files in {}: {}", dirPath, e.what());
        }

        return files;
    }

    std::string GetBaseName(const std::string &path)
    {
        std::filesystem::path filePath(path);
        std::string stem = filePath.stem().string();
        return stem;
    }

    std::string GetDirectory(const std::string &path)
    {
        std::filesystem::path filePath(path);
        std::filesystem::path parentDir = filePath.parent_path();
        std::string dirStr = parentDir.string();
        
        // Ensure trailing slash
        if (!dirStr.empty() && dirStr.back() != '/' && dirStr.back() != '\\')
        {
            dirStr += "/";
        }
        
        return dirStr;
    }

    std::string JoinPath(const std::string &base, const std::string &component)
    {
        std::filesystem::path basePath(base);
        std::filesystem::path componentPath(component);
        return (basePath / componentPath).string();
    }
}
