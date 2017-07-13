/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      mpxframe.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-07-28
 *
 */
#ifndef MPXFRAME_H
#define MPXFRAME_H
#include <string>
#include <map>
#include "metadata.h"

#define MPXFRAMEFILE_ASCII    0x1
#define MPXFRAMEFILE_BINARY   0x2
#define MPXFRAMEFILE_SPARSEX  0x4
#define MPXFRAMEFILE_SPARSEXY 0x8
#define MPXFRAMEFILE_APPEND   0x10
#define MPXFRAMEFILE_NODSC    0x20



class MpxFrame
{
public:
    enum FrameType { FRMT_I16 = 2, FRMT_U32 = 5, FRMT_U64 = 7, FRMT_DOUBLE = 9};
public:
    MpxFrame();
    virtual ~MpxFrame();

    int init(size_t width = 256, size_t height = 256, FrameType type = FRMT_DOUBLE);
    size_t frameCount(const char* filePath) const;
    int loadFromFile(const char* filePath, size_t frameIndex);
    void clear();

    size_t size() const { return mSize; }
    int frameType() const { return mDataType; }
    unsigned char* rawData() { return mData; }
    double* doubleData() { return reinterpret_cast<double*>(mData); }
    short* i16Data() { return reinterpret_cast<short*>(mData); }
    unsigned* u32Data() { return reinterpret_cast<unsigned*>(mData); }
    long long* u64Data() { return reinterpret_cast<long long*>(mData); }
    std::map<std::string, MetaData*>& metaData() { return mMetaData; }

    double acqTime();
    double startTime();
    double bias();

private:
    unsigned char* mData;
    int mDataType;
    size_t mWidth;
    size_t mHeight;
    size_t mSize;
    std::map<std::string, MetaData*> mMetaData;
};


#endif /* end of include guard: MPXFRAME_H */


