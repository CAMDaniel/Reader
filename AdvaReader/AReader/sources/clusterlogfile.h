/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      clusterlogfile.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-10-02
 *
 */
#ifndef CLUSTERLOGFILE_H
#define CLUSTERLOGFILE_H
#include <map>
#include "common.h"
#include "metadata.h"

class ClusterLogFile
{
public:
    enum ClusterLogType { ASCII = 0, BINARY = 1};
    static int save(const char* fileName, void* buffer, DataType dataType, u32 dataFormat, u32 width, u32 height, std::map<std::string, MetaData*> &metaData);
    static int frameCountInFile(const char* fileName);
    static bool isTpx3ClusterLog(const char* fileName);
    // for now toa=NULL (for older detectors)
    static int load(const char* fileName, size_t frameIndex, double* tot, double* toa, u32 width, u32 height, std::map<std::string, MetaData*>& metaData);
    static int getFrameDimensions(const char* fileName, size_t frameIndex, u32* width, u32* height);

private:
    template<typename T> static int saveFrameBufferAscii(FILE* file, T* buffer, u32 width, u32 height, const char* format);
    static int logError(int err, const char* text, ...);

    static i64 findNextFramePosition(FILE* file, int* frame);
    static i64 findFramePosition(FILE* file, int frame);
    static i64 findFrame(FILE* file, int frame, int frameCount, i64 fileSize);

private:

};


#endif /* !CLUSTERLOGFILE_H */

