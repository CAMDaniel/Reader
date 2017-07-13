/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      clusterlogfile.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-10-02
 *
 */
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <algorithm>
#include "clusterlogfile.h"
#include "osdepend.h"
#include "fileops.h"
#include "clusterfinder.h"

#define ROWLEN (65536*2)



template<typename T> int ClusterLogFile::saveFrameBufferAscii(FILE* file, T* buffer, u32 width, u32 height, const char* format)
{
    ClusterFinder<T> clf;
    clf.searchFrame(buffer, width, height);
    for (u32 i = 0; i < clf.clusters.size(); i++) {
        Cluster* cluster = clf.clusters[i];
        for (u32 pix = 0; pix < cluster->pixels.size(); pix++) {
            Pixel pixel = cluster->pixels[pix];
            fprintf(file, format, pixel.x, pixel.y, static_cast<T>(pixel.val));
        }
        fprintf(file, "\n");
    }
    return 0;
}

int ClusterLogFile::save(const char* fileName, void* buffer, DataType dataType, u32 dataFormat, u32 width, u32 height, std::map<std::string, MetaData*> &metaData)
{
    i64 idxFileSize = 0;
    size_t frameCount = 0;
    std::string idxFileName = std::string(fileName) + ".idx";
    const char* logmode = fileExists(fileName) ? "a" : "w";
    const char* idxmode = fileExists(idxFileName.c_str()) ? "ab" : "wb";
    if (!fileExists(getFileDir(fileName)))
        createFilePath(fileName);

    if (fileExists(idxFileName.c_str())){
        idxFileSize = getFileSize(idxFileName.c_str());
        if (idxFileSize < 0)
            return -1;
        frameCount = static_cast<size_t>(idxFileSize / sizeof(i64));
    }

    double acqTime = 0;
    double startTime = 0;
    if (metaData.find("Acq time") != metaData.end())
        acqTime = *reinterpret_cast<double*>(metaData["Acq time"]->data());
    if (metaData.find("Start time") != metaData.end())
        startTime = *reinterpret_cast<double*>(metaData["Start time"]->data());

    i64 logFileSize = getFileSize(fileName);
    if (logFileSize < 0)
        logFileSize = 0;

    File logFile(fileName, logmode);
    if (!logFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", fileName);

    File idxFile(idxFileName.c_str(), idxmode);
    if (!idxFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", idxFileName.c_str());

    fprintf(logFile, "Frame %u (%f, %f s)\n", static_cast<u32>(frameCount + 1), startTime, acqTime);
    fwrite(&logFileSize, 1, sizeof(i64), idxFile);

    switch (dataType) {
        case DT_BYTE:    return saveFrameBufferAscii(logFile, reinterpret_cast<byte*>(buffer), width, height, "[%u, %u, %u] ");  break;
        case DT_I16:     return saveFrameBufferAscii(logFile, reinterpret_cast<i16*>(buffer), width, height, "[%u, %u, %d] ");break;
        case DT_U16:     return saveFrameBufferAscii(logFile, reinterpret_cast<u16*>(buffer), width, height, "[%u, %u, %u] ");break;
        case DT_I32:     return saveFrameBufferAscii(logFile, reinterpret_cast<i32*>(buffer), width, height, "[%u, %u, %d] ");break;
        case DT_U32:     return saveFrameBufferAscii(logFile, reinterpret_cast<u32*>(buffer), width, height, "[%u, %u, %u] ");break;
        case DT_U64:     return saveFrameBufferAscii(logFile, reinterpret_cast<u64*>(buffer), width, height, "[%u, %u, %lu] ");break;
        case DT_DOUBLE:  return saveFrameBufferAscii(logFile, reinterpret_cast<double*>(buffer), width, height, "[%u, %u, %g] ");break;
        default: return -1;
    }
    return 0;
}

int ClusterLogFile::frameCountInFile(const char* fileName)
{
    File file(fileName, "r");
    if (!file)
        return -1;

    char row[ROWLEN], *neof = row;
    int frame = -1;
    int len = 0;
    i64 fileSize = 0, startPos = 0;

    // set start position to endOfFile - some offset
    fileSize = getFileSize(fileName);
    startPos = fileSize - 900000;
    if (startPos < 0)
        startPos = 0;

    // seek to the position of the frame
    if (startPos > 0 && fseek64(file, startPos, SEEK_SET))
        return -2;

    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL) row[0] = 0;
        len = (int)strlen(row);
        if (len > 6 && row[0] == 'F')
            sscanf(row, "Frame %d", &frame);
    }

    return frame;
}

bool ClusterLogFile::isTpx3ClusterLog(const char* fileName)
{
    File file(fileName, "r");
    if (!file)
        return false;

    char row[ROWLEN], *neof = row;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;
        if (row[0] != '[')
            continue;

        int x = 0, y = 0;
        double vtot = 0, vtoa = 0;
        if (sscanf(row,"[%d, %d, %lg, %lg]", &x, &y, &vtot, &vtoa) == 4)
            return true;
        return false;
    }
    return false;
}

i64 ClusterLogFile::findNextFramePosition(FILE* file, int* frame)
{
    char row[ROWLEN], *neof = row;
    size_t len = 0;
    i64 startPos = 0;
    int curFrame = -1;

    while (neof){
        startPos = ftell64(file);
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;
        len = strlen(row);
        if (len > 6 && row[0] == 'F'){
            sscanf(row, "Frame %d", &curFrame);
            if (curFrame >= 0 )
                break;
        }
    }
    *frame = curFrame;
    return startPos;
}

i64 ClusterLogFile::findFramePosition(FILE* file, int frame)
{
    char row[ROWLEN], *neof = row;
    size_t len = 0;
    i64 startPos = 0;
    int curFrame = -1;

    while (neof){
        startPos = ftell64(file);
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;
        len = strlen(row);
        if (len > 6 && row[0] == 'F'){
            sscanf(row, "Frame %d", &curFrame);
            if (curFrame == frame)
                return startPos;
        }
    }
    return -1;
}

i64 ClusterLogFile::findFrame(FILE* file, int frame, int frameCount, i64 fileSize)
{
    i64 low = 0;
    i64 high = fileSize;
    int curFrame = -1;
    i64 curPos = 0;
    i64 curFramePos = 0;

    while (low < high && curFrame != frame){
        curPos = low + (i64)((high - low)/2);
        if (abs(curFrame-frame) < 2){
            fseek64(file, low, SEEK_SET);
            curFramePos = findFramePosition(file, frame);
            curFrame = curFramePos < 0 ? -1 : frame;
        }else{
            fseek64(file, curPos, SEEK_SET);
            curFramePos = findNextFramePosition(file, &curFrame);
        }

        if (curFrame < 0)
            return -1;

        if (curFrame > frame)
            high = curPos;

        if (curFrame < frame)
            low = curFramePos;
    }

    if (curFrame == frame)
        return curFramePos;
    return -1;
}


int ClusterLogFile::getFrameDimensions(const char* fileName, size_t frameIndex, u32* width, u32* height)
{
    i64 offset = 0;
    std::string idxFileName = std::string(fileName) + ".idx";
    size_t frameCount = 0;

    // get frame position from index file or find in the file if idx does not exists:
    if (!fileExists(idxFileName.c_str())){
        i64 fileSize = getFileSize(fileName);
        frameCount = frameCountInFile(fileName);
        File file(fileName, "r");
        if (!file)
            return logError(PXERR_FILE_OPEN, "Cannot open log file: %s", fileName);
        offset = findFrame(file, (int)frameIndex + 1, (int)frameCount, fileSize);
        if (offset < 0)
            return logError(PXERR_FILE_OPEN, "Cannot find frame in file %s.", idxFileName.c_str());
    }else{
        File idxFile(idxFileName.c_str(), "rb");
        if (!idxFile)
            return logError(PXERR_FILE_OPEN, "Cannot open index file: %s", idxFileName.c_str());
        if (frameIndex > 0) {
            if (fseek64(idxFile, frameIndex * sizeof(i64), SEEK_SET))
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
            if (fread(&offset, sizeof(i64), 1, idxFile) != 1)
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
        }
    }

    File file(fileName, "r");
    if (!file)
        return -1;

    // seek to the position of the frame
    if (offset > 0 && fseek64(file, offset, SEEK_SET))
        return logError(PXERR_FILE_OPEN, "Cannot read log file: %s", fileName);

    // find start of the frame
    char row[ROWLEN];
    char *neof = row;
    int frameNum = 0;
    double startTime = 0, acqTime = 0;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] != 'F')
            continue;

        int frame = 0;
        int n = sscanf(row, "Frame %d (%lg, %lg s)", &frame, &startTime, &acqTime);
        if (n == 3){
            frameNum = frame;
            if (frameNum-1 == (int)frameIndex)
                break;
        }
    }

    // frame not found
    if (frameNum-1 != (int)frameIndex)
        return logError(PXERR_FILE_READ, "Frame %d not found in file %s", frameIndex, fileName);

    // load frame data:
    char *ptr;
    int w = 256;
    int h = 256;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] == 'F')
            break;

        ptr = row;
        double vtot = 0;
        int x = 0, y = 0;

        // load timepix cluster log:
        while ((ptr = strchr(ptr,'[')) != NULL){
            if (sscanf(ptr,"[%d, %d, %lg", &x, &y, &vtot) != 3)
                break; // error
            if (x > w) w = x;
            if (y > h) h = y;
            ptr++; // skip one space
        }
    }

    *width = std::max((u32)256, (u32)ceil((double)(w) / 256.0)*256);
    *height = std::max((u32)256, (u32)ceil((double)(h) / 256.0)*256);
    return 0;
}


int ClusterLogFile::load(const char* fileName, size_t frameIndex, double* tot, double* toa, u32 width, u32 height, std::map<std::string, MetaData*>& metaData)
{
    i64 offset = 0;
    std::string idxFileName = std::string(fileName) + ".idx";
    size_t frameCount = 0;

    // get frame position from index file or find in the file if idx does not exists:
    if (!fileExists(idxFileName.c_str())){
        i64 fileSize = getFileSize(fileName);
        frameCount = frameCountInFile(fileName);
        File file(fileName, "r");
        if (!file)
            return logError(PXERR_FILE_OPEN, "Cannot open log file: %s", fileName);
        offset = findFrame(file, (int)frameIndex + 1, (int)frameCount, fileSize);
        if (offset < 0)
            return logError(PXERR_FILE_OPEN, "Cannot find frame in file %s.", idxFileName.c_str());
    }else{
        File idxFile(idxFileName.c_str(), "rb");
        if (!idxFile)
            return logError(PXERR_FILE_OPEN, "Cannot open index file: %s", idxFileName.c_str());
        if (frameIndex > 0) {
            if (fseek64(idxFile, frameIndex * sizeof(i64), SEEK_SET))
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
            if (fread(&offset, sizeof(i64), 1, idxFile) != 1)
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
        }
    }


    File file(fileName, "r");
    if (!file)
        return -1;

    // seek to the position of the frame
    if (offset > 0 && fseek64(file, offset, SEEK_SET))
        return logError(PXERR_FILE_OPEN, "Cannot read log file: %s", fileName);

    // find start of the frame
    char row[ROWLEN];
    char *neof = row;
    int frameNum = 0;
    double startTime = 0, acqTime = 0;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] != 'F')
            continue;

        int frame = 0;
        int n = sscanf(row, "Frame %d (%lg, %lg s)", &frame, &startTime, &acqTime);
        if (n == 3){
            frameNum = frame;
            if (frameNum-1 == (int)frameIndex)
                break;
        }
    }

    // frame not found
    if (frameNum-1 != (int)frameIndex)
        return logError(PXERR_FILE_READ, "Frame %d not found in file %s", frameIndex, fileName);


    // load frame data:
    char *ptr;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] == 'F')
            break;

        ptr = row;
        double vtot = 0, vtoa = 0;
        int x = 0, y = 0;

        if (toa){
            // load tpx3 cluster log:
            while ((ptr = strchr(ptr,'[')) != NULL){
                if (sscanf(ptr,"[%d, %d, %lg, %lg]", &x, &y, &vtot, &vtoa) != 4)
                    break; // error
                tot[width*y + x] = vtot;
                toa[width*y + x] = vtoa;
                ptr++; // skip one space
            }

        }else{
            // load timepix cluster log:
            while ((ptr = strchr(ptr,'[')) != NULL){
                if (sscanf(ptr,"[%d, %d, %lg]", &x, &y, &vtot) != 3)
                    break; // error
                tot[width*y + x] = vtot;
                ptr++; // skip one space
            }
        }
    }

    metaData["Acq time"] =  new MetaData("Acq time", "Acquisition time [s]", DT_DOUBLE, &acqTime, sizeof(double));
    metaData["Start time"] =  new MetaData("Start time", "Acquisition start time", DT_DOUBLE, &startTime, sizeof(double));
    return 0;
}

int ClusterLogFile::logError(int err, const char* text, ...)
{
    va_list args;
    va_start(args, text);
//#ifdef TESTING
    int size = 512;
    std::string str;
    while (1) {
        str.resize(size);
        int n = vsnprintf((char *)str.c_str(), size, text, args);
        if (n > -1 && n < size) break;
        size = n > -1 ? n+1 : 2*size;
    }
    printf("ERROR: %s\n", str.c_str());
//#else
    //pxLogMsgV(0, text, args); // call to Pixet logging function
//#endif
    va_end(args);
    return err;
}
