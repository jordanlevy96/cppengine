/**
 * @file FileIO.h
 * @brief File I/O utility functions for reading/writing text files and directory operations
 */

#pragma once

#include <string>
#include <vector>

/**
 * @brief File I/O utility functions
 *
 * Provides helper functions for reading/writing text files and scanning directories.
 * All paths are relative to the executable location (typically build/ directory).
 */
namespace FileIO
{
    /**
     * @brief Read entire text file into string
     * @param path File path (relative to executable, e.g., "../res/ui/templates/game.html")
     * @return File contents, or empty string on error
     * @note Logs error if file cannot be opened
     */
    std::string ReadTextFile(const std::string &path);

    /**
     * @brief Write string to text file
     * @param path File path (relative to executable)
     * @param content Content to write
     * @return true if write succeeded, false on error
     * @note Creates parent directories if they don't exist
     * @note Logs error if write fails
     */
    bool WriteTextFile(const std::string &path, const std::string &content);

    /**
     * @brief Check if file exists
     * @param path File path (relative to executable)
     * @return true if file exists, false otherwise
     */
    bool FileExists(const std::string &path);

    /**
     * @brief List all files in directory with given extension
     * @param dirPath Directory path (relative to executable, e.g., "../res/ui/templates/")
     * @param extension File extension to filter (e.g., ".html", ".css", ".lua") - include the dot
     * @return Vector of filenames (without path, just basename)
     * @note Returns empty vector if directory doesn't exist or can't be read
     */
    std::vector<std::string> ListFiles(const std::string &dirPath, const std::string &extension);

    /**
     * @brief Get filename without extension
     * @param path Full file path or just filename
     * @return Filename without extension
     * @example GetBaseName("game.html") returns "game"
     * @example GetBaseName("../res/ui/templates/game.html") returns "game"
     */
    std::string GetBaseName(const std::string &path);

    /**
     * @brief Get directory path from full file path
     * @param path Full file path
     * @return Directory path (with trailing slash)
     * @example GetDirectory("../res/ui/templates/game.html") returns "../res/ui/templates/"
     */
    std::string GetDirectory(const std::string &path);

    /**
     * @brief Join path components
     * @param base Base path
     * @param component Path component to append
     * @return Joined path
     * @example JoinPath("../res/ui", "templates") returns "../res/ui/templates"
     */
    std::string JoinPath(const std::string &base, const std::string &component);
}
