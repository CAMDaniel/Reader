/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      fileops.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-03-23
 *
 */
#define _CRT_SECURE_NO_WARNINGS
#include "fileops.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#ifndef WIN32
#include <dirent.h>
#endif

int readAsciiMatrix(const char* fileName, DataType type, size_t itemCount, void* buff)
{
    File file(fileName, "r");
    if (!file)
        return FOERR_OPEN_FILE_FAIL;

    const char *format = 0;
    int size = 0;
    switch (type){
        case DT_BYTE: format = "%hhu"; size = sizeof(byte); break;
        case DT_I16: format = "%hd"; size = sizeof(i16); break;
        case DT_U16: format = "%u"; size = sizeof(u16); break;
        case DT_I32: format = "%d"; size = sizeof(i32); break;
        case DT_U32: format = "%u"; size = sizeof(u32); break;
        case DT_DOUBLE: format = "%lf"; size = sizeof(double); break;
        default: return FOERR_UNSUPPORTED;
    }

    size_t i;
    // because of Windows which does not know &hhu and %hd we have to 
    // parse DT_BYTE and DT_I16 manualy
    if (type == DT_BYTE){
        u32 val = 0;
        byte *pitem = (byte *)buff;
        for (i = 0; i < itemCount; i++) {
            if (fscanf(file, "%u", &val) != 1)
                break;
            pitem[i] = (byte)val;
        }
    } else if (type == DT_I16){
        int val = 0;
        i16 *pitem = (i16*)buff;
        for (i = 0; i < itemCount; i++) {
            if (fscanf(file, "%d", &val) != 1)
                break;
            pitem[i] = (i16)val;
        }
    } else {
        byte *pitem = (byte *)buff;
        for (i = 0; i < itemCount; i++) {
            if (fscanf(file, format, pitem) != 1)
                break;
            pitem += size;
        }
    }
    return i != itemCount ? FOERR_READ_FILE_FAIL : 0;
}


#define WRITE_MACRO(type, format1, format2) \
    {type* pbuff = (type*)buff;\
    for (i = 0; i < itemCount; i++)\
        if (fprintf(file, ((i+1) % columns) ? format1 : format2, *(pbuff++)) < 0)\
            break;\
    }\

int writeAsciiMatrix(const char* fileName, DataType type, void* buff, size_t itemCount, size_t columns, bool append)
{
    File file(fileName, append ? "a+": "w");
    if (!file)
        return FOERR_OPEN_FILE_FAIL;

    size_t i = 0;
    switch(type){
        case DT_BYTE: WRITE_MACRO(byte, "%u ", "%u\n"); break;
        case DT_I16: WRITE_MACRO(i16, "%hd ", "%hd\n"); break;
        case DT_U16: WRITE_MACRO(u16, "%u ", "%u\n"); break;
        case DT_I32: WRITE_MACRO(i32, "%d ", "%d\n"); break;
        case DT_U32: WRITE_MACRO(u32, "%u ", "%u\n"); break;
        case DT_DOUBLE: WRITE_MACRO(double, "%lf ", "%lf\n"); break;
        default: fclose(file); return FOERR_UNSUPPORTED;
    }
    if (i != itemCount)
        return FOERR_WRITE_FILE_FAIL;
    return 0;
}

int writeAsciiMatrixHex(const char* fileName, DataType type, void* buff, size_t itemCount, size_t columns, bool append)
{
    File file(fileName, append ? "a+": "w");
    if (!file)
        return FOERR_OPEN_FILE_FAIL;

    size_t i = 0;
    switch(type){
        case DT_BYTE: WRITE_MACRO(byte, "%02X ", "%02X\n"); break;
        case DT_I16: WRITE_MACRO(i16, "%04X ", "%04X\n"); break;
        case DT_U16: WRITE_MACRO(u16, "%04X ", "%04X\n"); break;
        case DT_I32: WRITE_MACRO(i32, "%08X ", "%08X\n"); break;
        case DT_U32: WRITE_MACRO(u32, "%08X ", "%08X\n"); break;
        default: fclose(file); return FOERR_UNSUPPORTED;
    }
    if (i != itemCount)
        return FOERR_WRITE_FILE_FAIL;
    return 0;
}

int readBinary(const char *fileName, byte *buff, size_t size)
{
    File file(fileName, "rb");
    if (!file)
        return FOERR_OPEN_FILE_FAIL;
    if (fread(buff, 1, size, file) < (size_t)size)
        return FOERR_READ_FILE_FAIL;
    return 0;
}

int writeBinary(const char* fileName, byte* buff, size_t size, bool append)
{
    File file(fileName, append? "a+b" : "wb");
    if (!file)
        return FOERR_OPEN_FILE_FAIL;
    if (fwrite(buff, 1, size, file) < (size_t)size)
        return FOERR_WRITE_FILE_FAIL;
    return 0;
}

int getline(FILE* stream, char** buf, size_t* len)
{
    int i = 0, newlen, mymalloc = 0;
    char* nl, *newbuf;

    if (buf == NULL || len == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*buf == NULL || *len == 0) {
        *buf = NULL; *len = 0;
        mymalloc = 1;
    }

    if (*len <= 60) goto alloc;

    while (1) {
        if (fgets(*buf + i, static_cast<int>(*len - i), stream) == NULL) {
            if (!feof(stream) || i == 0)
                goto error;
             else
                return i; // final line has no newline
        }

        if (feof(stream))
            return static_cast<int>(i + strlen(*buf + i));

        if ((nl = (char*)memchr(*buf + i, '\n', *len - i - 1)) == NULL) {
            i = static_cast<int>(*len) - 1;
alloc:      newlen = static_cast<int>(*len) < 60 ? 120 : static_cast<int>(*len) * 2;
            if ((newbuf = (char*)realloc(*buf, newlen)) == NULL) goto error;
            *buf = newbuf;
            *len = newlen;
        } else {
            return static_cast<int>(nl - *buf + 1); // new line, done
        }
    }
error:
    if (mymalloc) {
        free(*buf);
        *buf = NULL; *len = 0;
    }
    return -1;
}

bool matchMask(const char* fileName, const char* maskToMatch)
{
    char *str = (char*)fileName;
    char *mask = (char*)maskToMatch;
    bool star = false;
    char *s, *m;
    while (true) {
        for (s = str, m = mask; *s; ++s, ++m) {
            if (*m == '?') {
                if (*s == '.') {
                    if (!star)
                        return false;
                    str++;
                    break;
                }
            } else if (*m == '*') {
                star = true;
                str = s;
                mask = m;
                do {++mask;} while (*mask == '*');
                if (*mask == '\0')
                    return true;
                break;
            } else if (*m != *s) {
                if (!star)
                    return false;
                str++;
                break;
            }
        }
        if (*s == '\0') {
            while (*m == '*')
                ++m;
            return *m == '\0';
        }
    }
}

int listFilesInDir(const char* path, const char* mask, bool onlyFiles, bool fullPath, std::vector<std::string>& files)
{
#ifdef WIN32
#ifdef WIN32_UTF8
    WIN32_FIND_DATAW fd;
    std::wstring wmaskName = utf8ToWString(path) + PATH_SEPAR_STRW + utf8ToWString(mask);
    HANDLE hFind = FindFirstFileW(wmaskName.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return FOERR_INVALID_ARGUMENT;
    do {
        if (onlyFiles && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        // ignore . and ..
        if ((fd.cFileName[0] == '.' && fd.cFileName[1] == '\0') || (fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'))
            continue;

        std::wstring wFileName = fd.cFileName;
        if (fullPath){
            std::string fullFilePath = std::string(path) + PATH_SEPAR_STR + wstringToUtf8(wFileName);
            files.push_back(fullFilePath);
        }else
            files.push_back(wstringToUtf8(wFileName));
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
#else
    WIN32_FIND_DATA fd;
    std::string maskName = std::string(path) + PATH_SEPAR_STR + mask;
    HANDLE hFind = FindFirstFile(maskName.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return FOERR_INVALID_ARGUMENT;

    do {
        if (onlyFiles && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            continue;

        // ignore . and ..
        if ((fd.cFileName[0] == '.' && fd.cFileName[1] == '\0') || (fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'))
            continue;

        if (fullPath){
            std::string fullFilePath = std::string(path) + PATH_SEPAR + fd.cFileName;
            files.push_back(fullFilePath);
        }else
            files.push_back(fd.cFileName);
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
#endif
#else
    DIR* dir = opendir(path);
    if (!dir)
        return FOERR_INVALID_ARGUMENT;

    dirent* dent = 0;
    while ((dent = readdir(dir))) {
        if (!matchMask(dent->d_name, mask))
            continue;

        if (onlyFiles && dent->d_type == DT_DIR)
            continue;

        // ignore . and ..
        if ((dent->d_name[0] == '.' && dent->d_name[1] == '\0') || (dent->d_name[0] == '.' && dent->d_name[1] == '.' && dent->d_name[2] == '\0'))
            continue;

        if (fullPath){
            std::string fullFilePath = std::string(path) + PATH_SEPAR + dent->d_name;
            files.push_back(fullFilePath);
        }else
            files.push_back(dent->d_name);
    }

    closedir(dir);
#endif
    return 0;
}
