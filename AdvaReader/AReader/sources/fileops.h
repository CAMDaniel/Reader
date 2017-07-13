/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      fileops.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-07-29
 *
 */
#ifndef FILEOPS_H
#define FILEOPS_H
#include "common.h"
#include "osdepend.h"
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#ifndef WIN32
#include <sys/stat.h>
#define _stricmp  strcasecmp
#endif


#define FOERR_OPEN_FILE_FAIL     -1
#define FOERR_READ_FILE_FAIL     -2
#define FOERR_WRITE_FILE_FAIL    -3
#define FOERR_UNSUPPORTED        -4
#define FOERR_INVALID_ARGUMENT   -5


int readAsciiMatrix(const char* fileName, DataType type, size_t itemCount, void *buff);
int writeAsciiMatrix(const char* fileName, DataType type, void* buff, size_t itemCount, size_t columns, bool append);
int writeAsciiMatrixHex(const char* fileName, DataType type, void* buff, size_t itemCount, size_t columns, bool append);
int readBinary(const char* fileName, byte* buff, size_t size);
int writeBinary(const char* fileName, byte* buff, size_t size, bool append);
int getline(FILE* stream, char** buf, size_t* len);
int listFilesInDir(const char* path, const char* mask, bool onlyFiles, bool fullPath, std::vector<std::string>& files);
int copyFile(const char* sourceFilePath, const char* destFilePath);

inline const char* getBaseName(const char* fileName)
{
    const char *baseName = strrchr(fileName, PATH_SEPAR);
    return baseName ? baseName + 1 : fileName;
}

inline const char* getFileExt(const char* fileName)
{
    if (!fileName) return "";
    const char *slash = strrchr(fileName, PATH_SEPAR);
    const char *dot = strrchr(fileName, '.');
    return dot > slash ? dot+1 : "";
}

inline bool hasFileExt(const char* fileName, const char* ext)
{
    if (!fileName) return false;
    const char *dot = strrchr(fileName, '.');
    const char *slash = strrchr(fileName, PATH_SEPAR);
    const char *fext = 0;
    fext = (dot > slash) ? dot+1 : "";
    return _stricmp(fext, ext) == 0;
}

inline bool hasFileExt(std::string fileName, std::string ext)
{
    return hasFileExt(fileName.c_str(), ext.c_str());
}

inline char* getFileNameWithoutExt(const char* fileName, char* name, unsigned size)
{
    if (!fileName) return (char*)"";
    const char *dot = strrchr(fileName, '.');
    strncpy(name, fileName, (unsigned)(dot-fileName) > size ? size : dot-fileName);
    return name;
}

inline std::string getFileNameWithoutExt(const char* fileName)
{
    if (!fileName) return std::string("");
    const char *dot = strrchr(fileName, '.');
    return std::string(fileName).substr(0, (unsigned)(dot-fileName));
}

inline std::string getFileNameWithoutIdx(const char* fileName)
{
    if (!fileName) return std::string("");
    const char *dot = strrchr(fileName, '.');
    const char *slash = strrchr(fileName, PATH_SEPAR);
    const char *start = (dot == 0 || dot <= slash) ? fileName + strlen(fileName)-1 : dot-1;
    if (start < fileName)
        return std::string(fileName);
    const char* pos = start;
    while (((*pos >= '0' && *pos <= '9') || *pos == '.') && pos >= fileName)
        --pos;
    if (pos == start)
        return std::string(fileName);
    return std::string(fileName).substr(0, (unsigned)(pos+1-fileName)) + dot;
}

inline std::string changeFileExt(const char* fileName, const char* ext)
{
    return getFileNameWithoutExt(fileName) + (ext ?  std::string(".") + ext : "");
}

inline std::string changeFileExt(std::string fileName, std::string ext)
{
    return getFileNameWithoutExt(fileName.c_str()) + (ext.empty() ? "" : std::string(".") + ext);
}

inline std::string getFileDir(const char *fileName)
{
    const char *sep = strrchr(fileName, PATH_SEPAR);
    if (sep)
        return std::string(fileName).substr(0, sep-fileName);
    else
        return std::string("");
}

inline std::string getFileDir(std::string fileName)
{
    const char *sep = strrchr(fileName.c_str(), PATH_SEPAR);
    if (sep)
        return fileName.substr(0, sep-fileName.c_str());
    else
        return std::string("");
}

inline std::string normalizePathSepar(std::string fileName)
{
    std::string result = "";
    for (size_t i = 0; i < fileName.size(); i++){
        if (fileName[i] == '\\' || fileName[i] == '/')
            result += PATH_SEPAR;
        else
            result += fileName[i];
    }
    return result;
}

inline char* getFileNameIdx(const char* baseName, int index, int digits, const char* prefixDig, const char* sufixDig, char* name, unsigned size)
{
    const char *dot = strrchr(baseName, '.');
    const char *slash = strrchr(baseName, PATH_SEPAR);
    if (slash > dot) dot = 0;
    name[size-1] = '\0';
    char format[64];
    snprintf(format, 64, "%%.%ds%%s%%0%dd%%s%%s", dot ? (int)(dot - baseName) : size, digits);
    if (dot == NULL) dot = "";
    snprintf(name, size, format, baseName, !prefixDig?"":prefixDig, index, !sufixDig?"":sufixDig, dot);
    return name;
}

inline std::string getFileNameWithSuffix(const char* baseName, const char* suffix)
{
    char name[4096];
    size_t size = 4096;
    const char *dot = strrchr(baseName, '.');
    const char *slash = strrchr(baseName, PATH_SEPAR);
    if (slash > dot) dot = 0;
    name[size-1] = '\0';
    if (dot == NULL) dot = "";
    snprintf(name, size, "%s%s%s", baseName, !suffix?"":suffix, dot);
    return name;
}

inline std::string getFileNameIdx(const char* baseName, int index, int digits, const char* prefixDig, const char* sufixDig)
{
    char name[4096];
    return std::string(getFileNameIdx(baseName, index, digits, prefixDig, sufixDig, name, 4096));
}

inline std::string getFileNameIdxStr(const char* fileName)
{
    if (!fileName) return std::string("");
    const char *dot = strrchr(fileName, '.');
    const char *slash = strrchr(fileName, PATH_SEPAR);
    const char *start = (dot == 0 || dot <= slash) ? fileName + strlen(fileName)-1 : dot-1;
    if (start < fileName)
        return std::string("");
    const char* pos = start;
    while (*pos >= '0' && *pos <= '9' && pos >= fileName)
        --pos;
    if (pos == start)
        return std::string("");
    return std::string(fileName).substr(pos+1-fileName, start-pos);
}


inline int getFileNameIdx(const char* fileName)
{
    std::string idx = getFileNameIdxStr(fileName);
    return idx.empty() ? -1 : atoi(idx.c_str());
}

inline std::string getRepeatAcqIndexFileName(const std::string filePath, int repeatIndex, int repDigits, bool directory, int acqIndex, int acqDigits, int acqCount)
{
    std::string result = filePath;
    if (repDigits > 0){
        std::string fileDir = getFileDir(filePath.c_str());
        std::string fileName = getBaseName(filePath.c_str());
        if (directory)
            result = getFileNameIdx(fileDir.c_str(), repeatIndex, repDigits, fileDir.size() ? PATH_SEPAR_STR : NULL, PATH_SEPAR_STR) + fileName;
        else
            result = getFileNameIdx(filePath.c_str(), repeatIndex, repDigits, "_r", NULL);
    }
    if (acqDigits && acqCount > 1)
        return getFileNameIdx(result.c_str(), acqIndex, acqDigits, "_", NULL);
    return result;
}

inline std::string getValidFileName(std::string filePath)
{
    std::string result;
    std::transform(filePath.begin(), filePath.end(), filePath.begin(), tolower);
    for (size_t i = 0; i < filePath.size(); i++) {
        char c = filePath[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' || c <='z') || c == '-' || c == '_')
            result += c;
    }
    return result;
}

// create directories for supplied filePath
inline int createFilePath(const char* filePath)
{
    if (!filePath)
        return -1;

    char dir[1024] = "";
    const char *sep = strchr(filePath, PATH_SEPAR);
    while (sep) {
        strncpy(dir, filePath, sep - filePath + 1);
        createDir(dir);
        sep = strchr(sep + 1, PATH_SEPAR);
    }
    return 0;
}

// create directories tree
inline int createDirs(const char* dirPath)
{
    if (!dirPath)
        return -1;
    char dir[1024] = "";
    const char *sep = strchr(dirPath, PATH_SEPAR);
    while (sep) {
        strncpy(dir, dirPath, sep - dirPath + 1);
        if (!fileExists(dir))
            createDir(dir);
        sep = strchr(sep + 1, PATH_SEPAR);
    }
    createDir(dirPath);
    return 0;
}

inline std::string getAbsolutePath(const char* path)
{
#ifdef WIN32
    char outPath[4096];
    memset(outPath, 0, 4096);
    if (_fullpath(outPath, path, 4096) != NULL)
        return std::string(outPath);
    return path;
#else
    char outPath[4096];
    realpath(path, outPath);
    return std::string(outPath);
#endif
}

inline bool isAbsolutePath(const char* path)
{
    if (strlen(path) < 3)
        return false;
    if (path[0] == '/' || path[0] == '\\' || (path[1] == ':' && path[2] == '\\'))
        return true;
    return false;
}

inline int copyFile(const char* sourceFilePath, const char* destFilePath)
{
    try {
        std::ifstream src(sourceFilePath, std::ios::binary);
        std::ofstream dst(destFilePath, std::ios::binary);
        dst << src.rdbuf();
    }catch(...) {
        return -1;
    }
    return 0;
}


inline void wofstream(std::ofstream& file, std::string fileName, std::ios_base::openmode mode = std::ios_base::out)
{
#ifdef _MSC_VER
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(fileName.c_str());
    file.open(wfile.c_str(), mode);
#else
    file.open(fileName.c_str(), mode);
#endif
#else
    file.open(fileName.c_str(), mode);
#endif
}

inline void wifstream(std::ifstream& file, std::string fileName, std::ios_base::openmode mode = std::ios_base::out)
{
#ifdef _MSC_VER
#ifdef WIN32_UTF8
    std::wstring wfile = utf8ToWString(fileName.c_str());
    file.open(wfile.c_str(), mode);
#else
    file.open(fileName.c_str(), mode);
#endif
#else
    file.open(fileName.c_str(), mode);
#endif
}


#endif /* end of include guard: FILEOPS_H */




