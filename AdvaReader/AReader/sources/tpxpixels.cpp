/**
 * Copyright (C) 2015 Daniel Turecek
 *
 * @file      tpxpixels.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2015-04-08
 *
 */
#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <iomanip>
#include "tpxpixels.h"
#include "pxerrors.h"
#include "osdepend.h"
#include "fileops.h"
#include "metadata.h"
#include "strutils.h"
#include "datautils.h"

using namespace px;

#define DEFAULT_PIXEL_COUNT  40000000
#define ROWLEN               (10000)

TpxPixels::TpxPixels(DATAID dataID, IPixet* pixet)
    : mPixet(pixet)
    , mDev(NULL)
    , mIDev(NULL)
    , mDataMgr(NULL)
    , mDataID(dataID)
    , mMatrixIndexSize(2)
    , mFrameCount(0)
    , mFilledSize(0)
    , mSavedSize(0)
    , mSavedFrameCount(0)
    , mChipCount(1)
    , mWidth(256)
    , mHeight(256)
    , mRefCount(1)
{
    mDataMgr = mPixet->dataMgr();
}

TpxPixels::~TpxPixels()
{
    removeAllMetaData();
}

int TpxPixels::destroy() {
    if (mRefCount > 1){
        decRefCount();
        return -1;
    }
    if (mDataMgr)
        mDataMgr->removeData(this);
    delete this;
    return 0;
}

int TpxPixels::init(size_t initCapacity)
{
    if (initCapacity == 0)
        initCapacity = DEFAULT_PIXEL_COUNT;
    mPixels.reinit(initCapacity);
    mFrmInfos.reinit(initCapacity / 10);
    return 0;
}

void TpxPixels::setDevice(IDev* dev)
{
    mDev = dynamic_cast<IDevMpx*>(dev);
    mIDev = dev;
    mChipCount = mDev->chipCount();
    mWidth = mDev->width();
    mHeight = mDev->height();
}

int TpxPixels::load(const char* fileName, u32 index)
{
    i64 offset = 0;
    std::string idxFileName = std::string(fileName) + ".idx";

    if (!fileExists(idxFileName.c_str())){
        //return logError(PXERR_FILE_OPEN, "Cannot open index file: %s", idxFileName.c_str());
        i64 fileSize = getFileSize(fileName);
        if (mFrameCount == 0)
            mFrameCount = getCountInFile(fileName);
        File file(fileName, "r");
        if (!file)
            return logError(PXERR_FILE_OPEN, "Cannot open log file: %s", fileName);
        offset = findFrame(file, index + 1, (int)mFrameCount, fileSize);
        if (offset < 0)
            return logError(PXERR_FILE_OPEN, "Cannot find frame in file %s.", idxFileName.c_str());
    }else{
        File idxFile(idxFileName.c_str(), "rb");
        if (!idxFile)
            return logError(PXERR_FILE_OPEN, "Cannot open index file: %s", idxFileName.c_str());
        if (index > 0) {
            if (fseek64(idxFile, index * sizeof(i64), SEEK_SET))
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
            if (fread(&offset, sizeof(i64), 1, idxFile) != 1)
                return logError(PXERR_FILE_OPEN, "Cannot read index file: %s", idxFileName.c_str());
        }
    }

    if (mMetaData.empty())
        loadMetaData(fileName);

    File file(fileName, "r");
    if (!file)
        return logError(PXERR_FILE_OPEN, "Cannot open log file: %s", fileName);

    mPixels.clear();
    mPixels.reinit(mWidth*mHeight*sizeof(i16)*2);
    mFrmInfos.clear();
    mFrmInfos.reinit(1);

    // seek to the position of the frame
    if (offset > 0 && fseek64(file, offset, SEEK_SET))
        return logError(PXERR_FILE_OPEN, "Cannot read log file: %s", fileName);

    // find start of the frame
    char row[ROWLEN];
    char *neof = row;
    int frameNumber = 0;
    TpxPixelFrameInfo* infos = mFrmInfos.data();
    infos[0].frameStart = 0;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] != '[')
            continue;

        int frame = 0;
        int n = sscanf(row, "[Frame %d, %lg, %lg s]", &frame, &infos[0].startTime, &infos[0].acqTime);
        if (n == 3){
            frameNumber = frame;
            if (frame-1 == (int)index)
                break;
        }
    }

    // frame not found
    if (frameNumber-1 != (int)index)
        return logError(PXERR_FILE_READ, "Frame %d not found in file %s", index, fileName);

    // read data
    size_t pixOffset = 0;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;

        // skip too long lines
        if (strlen(row) >= ROWLEN-2)
            continue;

        if (row[0] == '[')
            break;

        int idx;
        int val;
        int n = sscanf(row, "%d\t%d", &idx, &val);
        if (n == 2){
            *((i16*)(mPixels.data() + pixOffset)) = (i16)val;
            *((u16*)(mPixels.data() + pixOffset + 2)) = (u16)idx;
            pixOffset += 4;
        }
    }
    infos[0].frameEnd = pixOffset;
    return 0;
}

int TpxPixels::save(const char* fileName, u32 fileType, u32 flags)
{
    if (!str::endsWith(fileName, PX_EXT_PIXEL_LOG))
        return logError(PXERR_FILE_WRITE, "Invalid file type (*.%s). File type has to be pixel log (*.plog)", getFileExt(fileName));

    PXUNUSED(flags);
    PXUNUSED(fileType);
    size_t filledSize = 0;
    size_t frameCount = 0;
    TpxPixelFrameInfo* infos = mFrmInfos.data();
    {
        ThreadLock lock(&mSync);
        frameCount = mFrameCount;
        filledSize = mFilledSize;
    }

    std::string idxFileName = std::string(fileName) + ".idx";
    const char* logmode = fileExists(fileName) ? "a" : "w";
    const char* idxmode = fileExists(idxFileName.c_str()) ? "ab" : "wb";
    if (!fileExists(getFileDir(fileName)))
        createFilePath(fileName);

    if (fileExists(fileName) && !fileExists(idxFileName))
        return logError(PXERR_FILE_WRITE, "Cannot continue writting in to file %s. Missing index file.", fileName);

    i64 logFileSize = getFileSize(fileName);
    if (logFileSize < 0)
        logFileSize = 0;

    File logFile(fileName, logmode);
    if (!logFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", fileName);

    if (logFileSize == 0)
        saveMetas(logFile);

    File idxFile(idxFileName.c_str(), idxmode);
    if (!idxFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", idxFileName.c_str());

    i64 fileSize = getFileSize(idxFileName.c_str());
    size_t frameIndex = fileSize > 0 ? static_cast<size_t>(fileSize)/sizeof(i64) : 0;

    for (size_t i = mSavedFrameCount; i < frameCount; i++) {
        size_t fromIdx = infos[i].frameStart;
        size_t toIdx = infos[i].frameEnd;

        // skip empty frames
        if (fromIdx > toIdx || toIdx - fromIdx < 1)
            continue;

        logFileSize = ftell64(logFile);
        fwrite(&logFileSize, 1, sizeof(i64), idxFile);

        fprintf(logFile, "[Frame %u, %f, %f s]\n", static_cast<u32>(++frameIndex), infos[i].startTime, infos[i].acqTime);
        for (size_t i = fromIdx; i < toIdx; i+=4){
            u16 val = *(u16*)(mPixels.data() + i);
            u16 idx = *(u16*)(mPixels.data() + i + 2);
            fprintf(logFile, "%u\t%u\n",idx , val);
        }
        fprintf(logFile, "\n");
    }

    mSavedFrameCount = frameCount;
    mSavedSize = filledSize;
    return 0;
}

template<typename T> void TpxPixels::saveFrameBufferAscii(FILE* file, T* buffer, size_t size, const char* formatStr)
{
    for (size_t i = 0; i < size; i++) {
        if (buffer[i] != 0)
            fprintf(file, formatStr, i, buffer[i]);
    }
}

int TpxPixels::saveFrameToPixelLog(const char* fileName, void* buffer, DataType dataType, u32 width, u32 height, std::map<std::string, MetaData*>& metaData)
{
    std::string idxFileName = std::string(fileName) + ".idx";
    const char* logmode = fileExists(fileName) ? "a" : "w";
    const char* idxmode = fileExists(idxFileName.c_str()) ? "ab" : "wb";
    if (!fileExists(getFileDir(fileName)))
        createFilePath(fileName);

    if (fileExists(fileName) && !fileExists(idxFileName))
        return logError(PXERR_FILE_WRITE, "Cannot continue writting in to file %s. Missing index file.", fileName);

    i64 logFileSize = getFileSize(fileName);
    if (logFileSize < 0)
        logFileSize = 0;

    File logFile(fileName, logmode);
    if (!logFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", fileName);

    if (logFileSize == 0)
        saveMetas(logFile, metaData);

    File idxFile(idxFileName.c_str(), idxmode);
    if (!idxFile)
        return logError(PXERR_FILE_OPEN, "Cannot open file: %s", idxFileName.c_str());
    i64 fileSize = getFileSize(idxFileName.c_str());
    size_t frameIndex = fileSize > 0 ? static_cast<size_t>(fileSize)/sizeof(i64) : 0;

    double acqTime = 0;
    double startTime = 0;
    if (metaData.find("Acq time") != metaData.end())
        acqTime = *reinterpret_cast<double*>(metaData["Acq time"]->data());
    if (metaData.find("Start time") != metaData.end())
        startTime = *reinterpret_cast<double*>(metaData["Start time"]->data());

    logFileSize = ftell64(logFile);
    fwrite(&logFileSize, 1, sizeof(i64), idxFile);
    fprintf(logFile, "[Frame %u, %f, %f s]\n", static_cast<u32>(++frameIndex), startTime, acqTime);

    size_t size = width*height;
    switch (dataType) {
        case DT_BYTE:    saveFrameBufferAscii(logFile, reinterpret_cast<byte*>(buffer), size, "%u\t%u\n");  break;
        case DT_I16:     saveFrameBufferAscii(logFile, reinterpret_cast<i16*>(buffer), size, "%u\t%d\n"); break;
        case DT_U16:     saveFrameBufferAscii(logFile, reinterpret_cast<u16*>(buffer), size, "%u\t%u\n"); break;
        case DT_I32:     saveFrameBufferAscii(logFile, reinterpret_cast<i32*>(buffer), size, "%u\t%d\n"); break;
        case DT_U32:     saveFrameBufferAscii(logFile, reinterpret_cast<u32*>(buffer), size, "%u\t%u\n"); break;
        case DT_U64:     saveFrameBufferAscii(logFile, reinterpret_cast<u64*>(buffer), size, "%u\t%lu\n"); break;
        case DT_DOUBLE:  saveFrameBufferAscii(logFile, reinterpret_cast<double*>(buffer), size, "%u\t%lg\n"); break;
        default: return -1;
    }
    fprintf(logFile, "\n");
    return 0;
}


int TpxPixels::addPixels(const byte* buffer, size_t size, const TpxPixelFrameInfo* frameInfos, size_t frameInfosCount)
{
    ThreadLock lock(&mSync);
    if (mFilledSize + size >= mPixels.size())
        return logError(PXERR_BUFFER_FULL, "Memory for data from Timepix full.");

    // resize frame indexes if buffer small
    if (frameInfosCount + mFrameCount >= mFrmInfos.size()){
        Buffer<TpxPixelFrameInfo> tmp(mFrmInfos.size());
        tmp.assignData(mFrmInfos.data(), mFrmInfos.size());
        mFrmInfos.reinit(std::max(mFrmInfos.size() * 2, frameInfosCount + mFrameCount));
        memcpy(mFrmInfos.data(), tmp.data(), tmp.size());
    }

    for (size_t i = 0; i < frameInfosCount; i++)
        mFrmInfos.data()[mFrameCount + i] = TpxPixelFrameInfo(frameInfos[i].frameStart + mFilledSize, frameInfos[i].frameEnd + mFilledSize, frameInfos[i].acqTime, frameInfos[i].startTime);

    memcpy(mPixels.data() + mFilledSize, buffer, size);
    mFilledSize += size;
    mFrameCount += frameInfosCount;
    return 0;
}

px::IMpxFrame* TpxPixels::frame(size_t index)
{
    IMpxFrame* frame = mDataMgr->createFrame((u32)mWidth, (u32)mHeight, PX_FRAMETYPE_I16);
    frame->fillWithZeros();
    RefDataBuff<u16> buff;
    frame->data(&buff);
    double startTime = mFrmInfos[index].startTime;
    double acqTime = mFrmInfos[index].acqTime;
    frame->addMetaData("Acq time", "Acquisition time [s]", DT_DOUBLE, &acqTime,  sizeof(double));
    frame->addMetaData("Start time", "Acquisition start time", DT_DOUBLE, &startTime, sizeof(double));

    std::map<std::string, MetaData*>::const_iterator it;
    for (it = mMetaData.begin(); it != mMetaData.end(); ++it) {
        MetaData* meta = it->second;
        if (std::string(meta->name()) != "Acq time" && std::string(meta->name()) != "Start time")
            frame->addMetaData(meta->name(), meta->desc(), meta->type(), meta->data(), meta->size());
    }

    size_t idxFrom = mFrmInfos[index].frameStart;
    size_t idxTo = mFrmInfos[index].frameEnd;
    for (size_t i = idxFrom; i < idxTo; i+=4) {
        byte* pointer = mPixels.data() + i;
        i16 value = *((i16*)pointer);
        u16 index = *((u16*)(pointer + 2));
        buff[index] = value;
    }
    return frame;
}

void TpxPixels::clear()
{
    mFilledSize = 0;
    mSavedFrameCount = 0;
    mSavedSize = 0;
    mFrameCount = 0;

}

size_t TpxPixels::getCountInFile(const char* fileName) const
{
    std::string idxFileName = std::string(fileName) + ".idx";
    if (fileExists(idxFileName.c_str())){
        i64 idxFileSize = getFileSize(idxFileName.c_str());
        return static_cast<size_t>(idxFileSize / sizeof(i64));
    }

    File file(fileName, "r");
    if (!file)
        return -1;

    char row[ROWLEN], *neof = row;
    int frame = -1;
    size_t len = 0;
    i64 startPos = 0;

    // set start position to endOfFile - some offset
    i64 fileSize = getFileSize(fileName);
    startPos = fileSize - 900000;
    if (startPos < 0)
        startPos = 0;

    // seek to the position of the frame
    if (startPos > 0 && fseek64(file, startPos, SEEK_SET))
        return -2;

    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL) row[0] = 0;
        len = strlen(row);
        if (len > 6 && row[0] == '['){
            sscanf(row, "[Frame %d", &frame);
        }
    }
    return frame;
}

std::string doubleToString(double* d, size_t count)
{
    std::stringstream ss;
    for (size_t i = 0; i < count; i++) {
        ss << std::setprecision(15);
        ss << d[i];
        if (i != count-1)
            ss << " ";
    }
    return ss.str();
}

template <typename T> std::string toString(T* buff, size_t count)
{
    std::string output;
    for (size_t i = 0; i < count; i++) {
        output += str::fromNum(buff[i]);
        if (i != count-1)
            output += " ";
    }
    return output;
}


int TpxPixels::loadMetaData(const char* fileName)
{
    File file(fileName, "r");
    if (!file)
        return -1;

    char row[ROWLEN], *neof = row;
    char name[ROWLEN], value[ROWLEN];
    size_t len = 0;
    int n = 0;

    std::map<std::string, std::string> metaData;
    while (neof){
        neof = fgets(row, ROWLEN, file);
        if (neof == NULL)
            row[0] = 0;
        len = strlen(row);

        // The row is too long => skip it.
        if (len >= ROWLEN-2){
            do {
                neof = fgets(row, ROWLEN, file);
                len = strlen(row);
            }while (len >= ROWLEN-2);
            continue;
        }

        // End of the frame => process it.
        if (len < 3)
            continue; // skip empty lines in the beginning

        if (row[0] == '[' && row[2] == 'r')
            break;

        n = sscanf(row, "%[^:]:%[^:]", name, value);
        if (n == 2)
            metaData[name] = value;
    }

    if (metaData.find("Width") != metaData.end())
        mWidth = str::toNumDef(metaData["Width"], 256);
    if (metaData.find("Height") != metaData.end())
        mHeight = str::toNumDef(metaData["Height"], 256);

    if (metaData.find("ChipboardID") != metaData.end()){
        std::string chipID = metaData["ChipboardID"];
        addMetaData("ChipboardID", "Chipboard ID", DT_CHAR, chipID.c_str(), chipID.size());
    }

    if (metaData.find("HV") != metaData.end()){
        double bias = str::toNumDef<double>(metaData["HV"], 0);
        addMetaData("HV", "High voltage [V]", DT_DOUBLE, &bias, sizeof(double));
    }

    if (metaData.find("Pixet version") != metaData.end()){
        std::string ver = metaData["Pixet version"];
        addMetaData("Pixet version", "Pixet version", DT_CHAR, ver.c_str(), ver.size());
    }

    return 0;
}

i64 TpxPixels::findNextFramePosition(FILE* file, int* frame)
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
        if (len > 6 && row[0] == '['){
            sscanf(row, "[Frame %d", &curFrame);
            if (curFrame >= 0 )
                break;
        }
    }
    *frame = curFrame;
    return startPos;
}

i64 TpxPixels::findFramePosition(FILE* file, int frame)
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
        if (len > 6 && row[0] == '['){
            sscanf(row, "[Frame %d", &curFrame);
            if (curFrame == frame)
                return startPos;
        }
    }
    return -1;
}

i64 TpxPixels::findFrame(FILE* file, int frame, int frameCount, i64 fileSize)
{
    i64 low = 0;
    i64 high = fileSize;
    //int lowFrCount = 0;
    //int highFrCount = frameCount;
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

        if (curFrame > frame){
            //highFrCount = curFrame;
            high = curPos;
        }
        if (curFrame < frame){
            //lowFrCount = curFrame;
            low = curFramePos;
        }
    }

    if (curFrame == frame)
        return curFramePos;

    return -1;
}

int TpxPixels::saveMetas(FILE* file)
{
    if (mMetaData.find("Width") == mMetaData.end()){
        addMetaData("Width", "Width", DT_I32, &mWidth, sizeof(int));
        addMetaData("Height", "Height", DT_I32, &mHeight, sizeof(int));
    }
    return saveMetas(file, mMetaData);
}


int TpxPixels::saveMetas(FILE* file, std::map<std::string, MetaData*> metaData)
{
    fprintf(file, "[File Meta Data]\n");
    std::map<std::string, MetaData*>::const_iterator it;
    for (it = metaData.begin(); it != metaData.end(); ++it) {
        MetaData* meta = it->second;
        if (meta->size() < 1000) {
            std::string data;
            switch (meta->type()) {
                case DT_BYTE: data = toString(reinterpret_cast<byte*>(meta->data()), meta->size()); break;
                case DT_BOOL: data = toString(reinterpret_cast<BOOL*>(meta->data()), meta->size()/sizeof(BOOL)); break;
                case DT_CHAR: data = std::string(reinterpret_cast<char*>(meta->data()), meta->size()); break;
                case DT_STRING: data = reinterpret_cast<char*>(meta->data()); break;
                case DT_I16: data = toString(reinterpret_cast<i16*>(meta->data()), meta->size()/sizeof(i16)); break;
                case DT_U16: data = toString(reinterpret_cast<u16*>(meta->data()), meta->size()/sizeof(u16)); break;
                case DT_I32: data = toString(reinterpret_cast<i32*>(meta->data()), meta->size()/sizeof(i32)); break;
                case DT_U32: data = toString(reinterpret_cast<u32*>(meta->data()), meta->size()/sizeof(u32)); break;
                case DT_I64: data = toString(reinterpret_cast<i64*>(meta->data()), meta->size()/sizeof(i64)); break;
                case DT_U64: data = toString(reinterpret_cast<u64*>(meta->data()), meta->size()/sizeof(u64)); break;
                case DT_FLOAT: data = toString(reinterpret_cast<float*>(meta->data()), meta->size()/sizeof(float)); break;
                case DT_DOUBLE: data = doubleToString(reinterpret_cast<double*>(meta->data()), meta->size()/sizeof(double)); break;
                default: break;
            }
            fprintf(file, "%s:%s\n", meta->name(), data.c_str());
        }
    }
    fprintf(file, "\n");
    return 0;
}


int TpxPixels::setMatrixIndexSize(size_t size)
{
    if (size != 2)
        return logError(PXERR_INVALID_ARGUMENT, "Matrix index size must be 2");
    mMatrixIndexSize = size;
    return 0;
}

int TpxPixels::metaDataCount() const
{
    return static_cast<int>(mMetaData.size());
}

int TpxPixels::metaDataNames(px::IStrList* metaDataNames) const
{
    if (!metaDataNames)
        return logError(PXERR_INVALID_ARGUMENT, "metaDataNames cannot be NULL");
    std::map<std::string, MetaData*>::const_iterator it;
    for (it = mMetaData.begin(); it != mMetaData.end(); ++it)
        metaDataNames->add(it->second->name());
    return 0;
}

px::IMetaData* TpxPixels::metaData(const char* metaDataName) const
{
    if (mMetaData.find(metaDataName) == mMetaData.end()){
        logError(PXERR_ITEM_NOT_FOUND, "Frame metaData with name %s not found.", metaDataName);
        return NULL;
    }
    return mMetaData[metaDataName];
}

int TpxPixels::addMetaData(const char* metaDataName, const char* description, u8 type, const void* data, size_t byteSize)
{
    if (type >= DT_LAST)
        return logError(PXERR_INVALID_ARGUMENT, "Invalid data type (%u)", type);
    if (mMetaData.find(metaDataName) != mMetaData.end())
        return logError(PXERR_EXISTS, "Frame metaData with name %s already exists.", metaDataName);
    MetaData* metaData = new MetaData(metaDataName, description, static_cast<DataType>(type), data, byteSize, false);
    mMetaData[metaDataName] = metaData;
    return 0;
}

int TpxPixels::removeMetaData(const char* metaDataName)
{
    std::map<std::string, MetaData*>::iterator it = mMetaData.find(metaDataName);
    if (it == mMetaData.end())
        return logError(PXERR_ITEM_NOT_FOUND, "Frame metaData with name %s not found.", metaDataName);
    MetaData* metaData = it->second;
    delete metaData;
    mMetaData.erase(it);
    return 0;
}

int TpxPixels::removeAllMetaData()
{
    std::map<std::string, MetaData*>::iterator it;
    for (it = mMetaData.begin(); it != mMetaData.end(); ++it) {
        MetaData* meta = it->second;
        delete meta;
    }
    mMetaData.clear();
    return 0;
}

int TpxPixels::metaDataRaw(const char* metaDataName, void* data, size_t byteSize) const
{
    if (!data)
        return logError(PXERR_INVALID_ARGUMENT, "data cannot be NULL");
    if (mMetaData.find(metaDataName) == mMetaData.end())
        return logError(PXERR_ITEM_NOT_FOUND, "Frame metaData with name %s not found.", metaDataName);
    MetaData* metaData = mMetaData[metaDataName];
    if (byteSize < metaData->size())
        return logError(PXERR_BUFFER_SMALL, "Buffer size is small %u < %u", byteSize, metaData->size());
    memcpy(data, metaData->data(), metaData->size());
    return 0;
}

int TpxPixels::setMetaDataRaw(const char* metaDataName, const void* data, size_t byteSize)
{
    if (!data)
        return logError(PXERR_INVALID_ARGUMENT, "data cannot be NULL");
    if (mMetaData.find(metaDataName) == mMetaData.end())
        return logError(PXERR_ITEM_NOT_FOUND, "Frame metaData with name %s not found.", metaDataName);
    MetaData* metaData = mMetaData[metaDataName];
    metaData->replaceData(data, byteSize);
    return 0;
}

px::IMpxFrame* TpxPixels::asIMpxFrame(u32 index)
{
    if (!mDataMgr)
        return NULL;
    if (mFrmInfos.empty()){
        logError(PXERR_NO_ITEMS, "No pixels to convert to frame");
        return NULL;
    }

    return frame(index);
}

int TpxPixels::logError(int err, const char* text, ...) const
{
    va_list args;
    va_start(args, text);
    std::string msg = str::formatv(text, args);
    mPixet->logMsg(NULL, 0, msg.c_str());
    va_end(args);
    return err;
}


