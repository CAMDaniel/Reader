/**
 * Copyright (C) 2015 Daniel Turecek
 *
 * @file      tpxpixels.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2015-04-08
 *
 */
#ifndef TPXPIXELS_H
#define TPXPIXELS_H
#include <map>
#include <vector>
#include <string>
#include "common.h"
#include "ipixet.h"
#include "osdepend.h"
#include "buffer.h"

class MetaData;

struct TpxPixelFrameInfo
{
    double acqTime;
    double startTime;
    size_t frameStart;
    size_t frameEnd;
    TpxPixelFrameInfo() : acqTime(0), startTime(0), frameStart(0), frameEnd(0) {}
    TpxPixelFrameInfo(size_t _frameStart, size_t _frameEnd, double _acqTime, double _startTime) : acqTime(_acqTime), startTime(_startTime), frameStart(_frameStart), frameEnd(_frameEnd) {}
};


class TpxPixels : public px::ITpxPixels
{
public:
    TpxPixels(DATAID dataID, px::IPixet* pixet);
    virtual ~TpxPixels();
    int init(size_t initCapacity = 0);
    int saveFrameToPixelLog(const char* fileName, void* buffer, DataType dataType, u32 width, u32 height, std::map<std::string, MetaData*> &metaData);

public:
    // IData interface
    virtual int FCALL dataFormat() const { return PX_DATAFORMAT_TPX_PIXELS; }
    virtual int FCALL lock(u32 timeout = PX_DEFAULT_LOCK_TIMEOUT) { PXUNUSED(timeout); return 0;}
    virtual int FCALL unlock() { return 0; }
    virtual bool FCALL isLocked() const { return false; }
    virtual void FCALL incRefCount() { ThreadLock lock(&mDataLock); mRefCount++;}
    virtual void FCALL decRefCount() { mDataLock.lock(); mRefCount--; mDataLock.unlock(); if (mRefCount <= 0) destroy(); }
    virtual int FCALL refCount() const { return mRefCount; }
    virtual DATAID FCALL dataID() const  { return mDataID; }
    virtual int FCALL setDataID(DATAID dataid) { PXUNUSED(dataid); return 0; }
    virtual int FCALL load(const char* fileName, u32 index=0);
    virtual int FCALL save(const char* fileName, u32 fileType, u32 flags=0);
    virtual int FCALL metaDataCount() const;
    virtual int FCALL metaDataNames(px::IStrList* metaDataNames) const;
    virtual px::IMetaData* FCALL metaData(const char* metaDataName) const;
    virtual int FCALL addMetaData(const char* metaDataName, const char* description, u8 type, const void* data, size_t byteSize);
    virtual int FCALL removeMetaData(const char* metaDataName);
    virtual int FCALL removeAllMetaData();
    virtual int FCALL metaDataRaw(const char* metaDataName, void* data, size_t byteSize) const;
    virtual int FCALL setMetaDataRaw(const char* metaDataName, const void* data, size_t byteSize);
    virtual size_t FCALL frameCount() const { return mFrameCount; }
    virtual px::IMpxFrame* FCALL asIMpxFrame(u32 index);
    virtual px::IDev* FCALL device() const { return mIDev; }
    virtual void FCALL setDevice(px::IDev* dev);
    virtual void FCALL onEvent(int dataEventType) { PXUNUSED(dataEventType); }
    virtual int FCALL destroy();

public:
    // ITpxPixels interface
    virtual px::IMpxFrame* FCALL frame(size_t index);
    virtual void FCALL clear();

 public:
    virtual int FCALL addPixels(const byte* buffer, size_t size, const TpxPixelFrameInfo* frameInfos, size_t frameInfosCount);
    virtual size_t FCALL getCountInFile(const char* fileName) const;
    virtual int FCALL setMatrixIndexSize(size_t size);
    virtual size_t FCALL matrixIndexSize() const { return mMatrixIndexSize; }
    virtual size_t FCALL pixelsSize() const { return mPixels.size(); }

private:
    int saveMetas(FILE* file);
    int saveMetas(FILE* file, std::map<std::string, MetaData*> metaData);
    int loadMetaData(const char* fileName);
    i64 findNextFramePosition(FILE* file, int* frame);
    i64 findFramePosition(FILE* file, int frame);
    i64 findFrame(FILE* file, int frame, int frameCount, i64 fileSize);
    template<typename T> void saveFrameBufferAscii(FILE* file, T* buffer, size_t size, const char* formatStr);
    int logError(int err, const char* text, ...) const;

private:
    ThreadSyncObject mDataLock;
    ThreadSyncObject mSync;
    px::IPixet* mPixet;
    px::IDevMpx* mDev;
    px::IDev* mIDev;
    px::IDataMgr* mDataMgr;
    DATAID mDataID;
    Buffer<TpxPixelFrameInfo> mFrmInfos;
    Buffer<byte> mPixels;
    mutable std::map<std::string, MetaData*> mMetaData;
    size_t mMatrixIndexSize;
    size_t mFrameCount;
    size_t mFilledSize;
    size_t mSavedSize;
    size_t mSavedFrameCount;
    size_t mChipCount;
    size_t mWidth;
    size_t mHeight;
    int mRefCount;
};


#endif /* !TPXPIXELS_H */

