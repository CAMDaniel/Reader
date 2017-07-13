/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      mpxframefile.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-08-06
 *
 */
#define _CRT_SECURE_NO_WARNINGS
#include <cmath>
#include <limits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <ctime>
#include <cmath>
#include <ctime>
#include <map>
#include <memory>
#include <algorithm>
#include <memory.h>
#include <cassert>
#include <vector>
#include "mpxframe.h"
#include "osdepend.h"

#ifndef NO_HDF5
#include "hdf5.h"
#include "hdf5_hl.h"
#endif

#define NOMINMAX

#define NAME_LENGTH         32
#define DESC_LENGTH         128
#define MAX_METADATA_STRSIZE  (16*1024)
#define DESC_FILE_EXT       "dsc"
#define IDX_FILE_EXT        "idx"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif

#ifndef WIN32
#define strnicmp strncasecmp
#else
#define strnicmp _strnicmp
#ifdef _MSC_VER
    #define snprintf(a,b,...) _snprintf_s(a,b,b,__VA_ARGS__)
    #define strncpy(a,b,c) strncpy_s(a,c,b,((c)-1))
#endif
#endif


#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
    #define THREAD_HANDLE  HANDLE
    #define WIN32_UTF8
    #ifdef _MSC_VER
        #ifdef snprintf
        #undef snprintf
        #endif
        #ifdef strncpy
        #undef strncpy
        #endif
        #define snprintf(a,b,...) _snprintf_s(a,b,b,__VA_ARGS__)
        #define strncpy(a,b,c) strncpy_s(a,c,b,((c)-1))
    #endif
#else
    #include <unistd.h>
    #include <dlfcn.h>
    #include <pthread.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
    #include <semaphore.h>
    #include <sys/time.h>
    #include <cstdarg>

    #define INVALID_HANDLE_VALUE   0
    #define INFINITE               0xFFFFFFFF
    #define THREAD_HANDLE          pthread_t
    #define _strnicmp               strncasecmp
    #define _stricmp                strcasecmp
#endif

#ifdef __APPLE__
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#define __useconds_t useconds_t
#endif

#define TIMEOUT_INF             0xffffffff
#define INVALID_THREAD_ID       0


#define GET_STR_VALS(dttype, func)\
    {\
        dttype* values = (dttype*)itemData;\
        char* begin = (char*)dataString;\
        char* end;\
        for (size_t i = 0; i < itemCount; i++) {\
            *values++ = (dttype)func(begin, &end, 10);\
            if (begin == end)\
                return -1;\
            begin = end;\
        }\
    }

#define GET_STR_VALSF(dttype, func)\
    {\
        dttype* values = (dttype*)itemData;\
        char* begin = (char*)dataString;\
        char* end;\
        for (size_t i = 0; i < itemCount; i++) {\
            *values++ = (dttype)func(begin, &end);\
            if (begin == end)\
                return -1;\
            begin = end;\
        }\
    }

#define GET_VALS(dttype, format, _values) {\
    const dttype * values = (const dttype *)itemData;\
    for (size_t i = 0; i < itemCount; i++) {\
        if (buffSize <= 0) break;\
        int rc = snprintf(buff, buffSize, format, _values);\
        if (rc < 0)\
            break;\
        buff += rc;\
        buffSize -= rc;\
    };\
    }


// basic data types
typedef int BOOL;
typedef unsigned char byte;
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
typedef u32 THREADID;
#define PATH_SEPAR          '\\'
#define PATH_SEPAR_STR      "\\"
#define PATH_SEPAR_STRW      L"\\"
#else
#include <stdint.h>
typedef long THREADID;
typedef void* HMODULE;
typedef void* HINSTANCE;
#define MAXDWORD            0xffffffff
#define PATH_SEPAR          '/'
#define PATH_SEPAR_STR      "/"
#endif


struct IndexItem;

struct IndexItem {
    i64 dscPos;
    i64 dataPos;
    i64 sfPos;
};

static const char * const typeNames[] =
{
    "char", "byte", "i16", "u16", "i32", "u32", "i64",
    "u64", "float", "double", "bool", "string", "uknown"
};

static const size_t sizeofType[DT_LAST] =
{
    sizeof(char),
    sizeof(byte),
    sizeof(i16),
    sizeof(u16),
    sizeof(i32),
    sizeof(u32),
    sizeof(i64),
    sizeof(u64),
    sizeof(float),
    sizeof(double),
    sizeof(BOOL),
    sizeof(char*)
};

template <class T> class Buffer
{
public:
    Buffer(size_t size = 0, bool shrinkable = false)
        : mBuff(0)
        , mSize(size)
        , mAllocatedSize(size)
        , mShrinkable(shrinkable)
    {
        if (size)
            mBuff = new T[size];
    }

    Buffer(const Buffer<T> &b)
        : mBuff(0)
        , mSize(b.mSize)
        , mAllocatedSize(b.mSize)
        , mShrinkable(b.mShrinkable)
    {
        if (mSize) {
            mBuff = new T[mSize];
            memcpy(mBuff, b.mBuff, b.byteSize());
        }
    }

    ~Buffer(){
        delete[] mBuff;
    }

    Buffer<T> & operator=(const Buffer<T> &b) {
        if (this != &b) {
            if (mSize != b.mSize)
                reinit(b.mSize);
            memcpy(mBuff, b.mBuff, byteSize());
        }
        return *this;
    }

    bool operator==(const Buffer<T> &b) {
        return mSize == b.size() && memcmp(mBuff, b.data(), byteSize()) == 0;
    }

    void setVal(T val) {
        if (mBuff)
            for (size_t i = 0; i < mSize; i++)
                mBuff[i] = val;
    }

    void zero() {
        if (mBuff)
            memset(mBuff, 0, mSize*sizeof(T));
    }

    void reinit(size_t size) {
        if (size == mSize) return;
        if (size > mAllocatedSize || mShrinkable) {
            delete[] mBuff;
            mAllocatedSize = mSize = 0; // in case new fails
            mBuff = new T[size];
            mAllocatedSize = mSize = size;
        }else
            mSize = size;
    }

    void reinit(size_t size, T val) {
        reinit(size);
        setVal(val);
    }

    template<typename U> void assignData(U *data, size_t size) {
        if (size != mSize)
            reinit(size);
        for (size_t i = 0; i < size; i++)
            mBuff[i] = (T)data[i];
    }

    // exchange all inner data between two buffers
    void exchangeBuffers(Buffer<T> &b)
    {
        T* tmpbuff = b.mBuff; b.mBuff = mBuff; mBuff = tmpbuff;
        size_t tmpsize = b.mSize; b.mSize = mSize; mSize = tmpsize;
        size_t tmpalloc = b.mAllocatedSize; b.mAllocatedSize = mAllocatedSize; mAllocatedSize = tmpalloc;
        bool tmpshrink = b.mShrinkable; b.mShrinkable = mShrinkable; mShrinkable = tmpshrink;
    }

    void clear() {
        delete[] mBuff;
        mBuff = 0;
        mAllocatedSize = mSize = 0;
    }

public:
    operator T*()             { return mBuff; }
    T* data()                 { return mBuff; }
    const T* data() const     { return mBuff; }
    size_t size() const       { return mSize; }
    size_t byteSize() const   { return mSize*sizeof(T); }
    T& get(size_t i) const    { return mBuff[i]; }
    void set(size_t i, T val) { mBuff[i] = val; }
    T& last()                 { return mBuff[mSize - 1]; }
    bool empty() const        { return mSize == 0; }
    void replaceInnerBuff(T* buff) { if (mBuff) delete[] mBuff; mBuff = buff; }

private:
    T* mBuff;
    size_t mSize;
    size_t mAllocatedSize;
    bool mShrinkable;
};




class MpxFrameFile
{
public:
    static int getFrameCount(const char* fileName, u32 &count);
    static int load(const char* fileName, u32 index, Buffer<byte> &buffer, DataType &dataType, u32 &width, u32 &height, std::map<std::string, MetaData*> &metaData);
    static int save(const char* fileName, void* buffer, DataType dataType, u32 dataFormat, u32 width, u32 height, std::map<std::string, MetaData*> &metaData);
    static bool isValidFrameFile(const char* fileName);

private:
    static int writeBinary(const char *fileName, bool append, void* buffer, size_t byteSize);
    static int readBinary(const char *fileName, i64 startPos, i64 endPos, void* buffer, size_t byteSize);
    static int writeAsciiMatrix(const char *fileName, bool append, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int readAsciiMatrix(const char *fileName, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int writeAsciiSparse(const char *fileName, bool append, bool xy, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int readAsciiSparse(const char *fileName, bool xy, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int writeBinarySparse(const char *fileName, bool append, bool xy, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int readBinarySparse(const char *fileName, bool xy, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height);
    static int readDesc(const char* fileName, int index, i64 startPos, i64 endPos, u32 &dataFormat, DataType &dataType, u32 &width, u32 &height, std::map<std::string, MetaData*> &metaData);
    static int writeDesc(const char* fileName, u32 dataFormat, DataType dataType, u32 width, u32 height, std::map<std::string, MetaData*> &metaData);
    static int writeDescHeader(FILE* file, char format, int count);
    static int readDescHeader(FILE* file, char &format, int &count);
    static int updateIndexFile(std::string dataFileName, IndexItem &indexItem);
    static int getIndexItem(const char* idxFileName, int frameIndex, IndexItem &itemStart, IndexItem &itemEnd);
    static int getCurrentIndexItem(std::string dataFileName, IndexItem &indexItem);
    static void getFileNames(std::string fileName, std::string  &dataFileName, std::string &dscFileName, std::string &idxFileName);
    static int guessFileType(const char *fileName, u32 &dataFormat, DataType &dataType, u32 &width, u32 &height, bool silent=false);
    static int detectType(const char* fileName, int &format, DataType &type, u32 itemCount, bool silent=false);

private:
    static int loadData(const char* fileName, DataType dataType, int dataFormat, i64 start, i64 end, Buffer<byte> &buffer, u32 width, u32 height);
    static int getLineItemsCount(FILE *file, u32 *count);
    static int getLinesCount(const char* fileName, u32* count);
    static int getSparseDim(FILE* file, u32* width, u32* height);
    static int getSparseDim(FILE* file, u32* size);
    static int strToType(const char* strType, DataType &type);
    static inline const char* formatToStr(u32 flags);
    static int strToFormat(const char* strFormat, u32 &format);
    static inline u32 getItemSize(int type);

    static inline int logErrorFileOpen(const char *fileName);
    static inline int logErrorFileSeek(const char *fileName);
    static inline int logErrorFileRead(const char *fileName);
    static inline int logErrorFileWrite(const char *fileName);
    static inline int logErrorFileBadFormat(const char *fileName, const char* msg);
    static inline int logErrorFileDscInvalid(const char *fileName, const char* msg);
    static inline int logErrorMemoryAlloc(size_t bytes);
    static int logError(int err, const char* text, ...);
};


inline int createPath(const char *path)
{
    char dir[1024] = "";
    const char *sep = strchr(path, PATH_SEPAR);
    while (sep) {
        strncpy(dir, path, sep - path + 1);
        createDir(dir);
        sep = strchr(sep + 1, PATH_SEPAR);
    }
    return 0;
}

inline DataType getTypeFromStr(const char *str)
{
    for (int i = 0; i < DT_LAST; i++) {
        if (strcmp(str, typeNames[i]) == 0)
            return (DataType) i;
    }
    if (strcmp(str, "uchar") == 0)
        return DT_CHAR;
    return DT_LAST;
}

int getDataFromString(DataType type, void* itemData, size_t itemCount, const char* dataString)
{
    switch (type) {
        case DT_BOOL: {
                BOOL* data = (BOOL*)itemData;
                int tmp = 1;
                if (sscanf(dataString, "%d", &tmp) == 1) {
                    char* begin = (char*)dataString;
                    for (size_t i = 0; i < itemCount; i++)
                        *data++ = (BOOL)strtol(begin, &begin, 10);
                } else {
                    const char *str = dataString;
                    for (size_t i = 0; i < itemCount && *str; i++) {
                        if (strnicmp(str, "false", 5) == 0)
                            *data++ = 0;
                        else if (strnicmp(str, "true", 4) == 0)
                            *data++ = 1;
                        else
                            return -1;
                        str += 5;
                        while (*str == ' ')
                            str++;
                    }
                }
            }
            break;
        case DT_I32:    GET_STR_VALS(i32, strtol); break;
        case DT_BYTE:   GET_STR_VALS(byte, strtol); break;
        case DT_I16:    GET_STR_VALS(i16, strtol); break;
        case DT_U16:    GET_STR_VALS(u16, strtoul); break;
        case DT_U32:    GET_STR_VALS(u32, strtoul); break;
        case DT_U64:    GET_STR_VALS(u64, strtoul); break;
        case DT_FLOAT:  GET_STR_VALSF(float, strtod); break;
        case DT_DOUBLE: GET_STR_VALSF(double, strtod); break;
        case DT_CHAR:
        case DT_STRING:
            strncpy((char *) itemData, dataString, itemCount);
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

int getStringFromData(DataType type, const void *itemData, size_t itemCount, char *buff, size_t buffSize, bool maxPrec)
{
    if (buffSize <= 0) return -1;
    buff[0] = buff[--buffSize] = '\0';
    switch (type) {
        case DT_BOOL:   GET_VALS(BOOL, "%s ", values[i] ? "TRUE" : "FALSE"); break;
        case DT_I32:    GET_VALS(i32, "%d ", values[i]); break;
        case DT_BYTE:   GET_VALS(byte, "%d ", (int)values[i]); break;
        case DT_I16:    GET_VALS(i16, "%hd ", values[i]); break;
        case DT_U16:    GET_VALS(u16, "%hu ", values[i]); break;
        case DT_U32:    GET_VALS(u32, "%u ", values[i]); break;
        case DT_U64:    GET_VALS(u64, "%llu ", values[i]); break;
        case DT_FLOAT:  GET_VALS(float, maxPrec?"%f ":"%.5g ", (double)values[i]); break;
        case DT_DOUBLE: GET_VALS(double, maxPrec?"%f ":"%.5g ", values[i]); break;
        case DT_CHAR:
            if (itemCount < buffSize) {
                strncpy(buff, (const char*)itemData, itemCount + 1);
                buff[itemCount] = '\0';
            } else
                strncpy(buff, (const char*)itemData, buffSize);
            break;
        case DT_STRING:
            strncpy(buff, (const char*)itemData, buffSize);
            break;
        default:
            buff[0] = '\0';
            break;
    }
    return 0;
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



int MpxFrameFile::load(const char* fileName, u32 index, Buffer<byte> &buffer, DataType &dataType, u32 &width, u32 &height, std::map<std::string, MetaData*> &metaData)
{
    int rc;
    u32 frameCount = 0;
    u32 dataFormat = 0;
    std::string dataFileName = "";
    std::string dscFileName = "";
    std::string idxFileName = "";
    getFileNames(fileName, dataFileName, dscFileName, idxFileName);

    // try to read frame count from dsc file
    if (!dscFileName.empty() && getFrameCount(dscFileName.c_str(), frameCount))
        return -1;

    // multi-frame file -> need index file and dsc
    if (index > 0 || !idxFileName.empty() || frameCount > 1) {
        if (idxFileName.empty())
            return logError(PXERR_INVALID_ARGUMENT, "Cannot open frame %d in file \"%s\" (no index file)", index, fileName);
        if (dscFileName.empty())
            return logError(PXERR_INVALID_ARGUMENT, "Cannot open frame %d in file \"%s\" (no dsc file)", index, fileName);
    }

    // compute frame count from index file if exists
    if (!idxFileName.empty()) {
        i64 fileSize;
        if ((fileSize = getFileSize(idxFileName.c_str())) < 0)
            return logErrorFileOpen(idxFileName.c_str());
        if (frameCount == 0)
            frameCount = static_cast<u32>((fileSize/sizeof(IndexItem)) + 1);
        else {
            if (frameCount != (fileSize/sizeof(IndexItem)) + 1)
                return logError(PXERR_FILE_BADDATA, "Inconsistency between index file and dsc file.");
        }
    }

    // check requested index
    if (!dscFileName.empty() && index >= frameCount)
        return logError(PXERR_INVALID_ARGUMENT, "Invalid frame index (%d) for file \"%s\". File contains %d frames. ", index, fileName, frameCount);

    // multi-frame positions
    IndexItem indexStart = {0, 0, 0};
    IndexItem indexEnd = {0, 0, 0};

    // if dsc file do not exist, try to guess file type
    if (index == 0 && dscFileName.empty() && idxFileName.empty()) {
        if ((rc = guessFileType(fileName, dataFormat, dataType, width, height)))
            return rc;
    } else {
        if (!idxFileName.empty()) {
            if ((rc = getIndexItem(idxFileName.c_str(), index, indexStart, indexEnd)))
                return rc;
        }
        if ((rc = readDesc(dscFileName.c_str(), index, indexStart.dscPos, indexEnd.dscPos, dataFormat, dataType, width, height, metaData)))
            return rc;
    }

    if (indexEnd.dataPos == 0)
        indexEnd.dataPos = getFileSize(dataFileName.c_str());
    if ((rc = loadData(dataFileName.c_str(), dataType, dataFormat, indexStart.dataPos, indexEnd.dataPos, buffer, width, height)))
        return rc;

    return 0;
}

int MpxFrameFile::save(const char* fileName, void* buffer, DataType dataType, u32 dataFormat, u32 width, u32 height, std::map<std::string, MetaData*> &metaData)
{
    int rc;
    size_t byteSize = sizeofType[dataType]*width*height;
    bool append = (dataFormat & MPXFRAMEFILE_APPEND) != 0;
    bool sparsexy = (dataFormat & MPXFRAMEFILE_SPARSEXY) != 0;
    bool updateIdxFile = (dataFormat & MPXFRAMEFILE_APPEND) != 0 && fileExists(fileName);

    IndexItem indexItem = {0,0,0};
    if (updateIdxFile)
        getCurrentIndexItem(fileName, indexItem);

    createPath(fileName);
    if (!(dataFormat & MPXFRAMEFILE_NODSC)) {
        std::string descName = std::string(fileName) + std::string(".") + DESC_FILE_EXT;
        if ((rc = writeDesc(descName.c_str(), dataFormat, dataType, width, height, metaData)))
            return rc;
    }

    if (dataFormat & MPXFRAMEFILE_ASCII) {
        if (dataFormat & (MPXFRAMEFILE_SPARSEX | MPXFRAMEFILE_SPARSEXY))
            rc = writeAsciiSparse(fileName, append, sparsexy, buffer, byteSize, dataType, width, height);
        else
            rc = writeAsciiMatrix(fileName, append, buffer, byteSize, dataType, width, height);
    } else {
        if (dataFormat & (MPXFRAMEFILE_SPARSEX | MPXFRAMEFILE_SPARSEXY))
            rc = writeBinarySparse(fileName, append, sparsexy, buffer, byteSize, dataType, width, height);
        else
            rc = writeBinary(fileName, append, buffer, byteSize);
    }

    if (updateIdxFile)
        updateIndexFile(fileName, indexItem);

    return rc;
}

int MpxFrameFile::loadData(const char* fileName, DataType dataType, int dataFormat, i64 start, i64 end, Buffer<byte> &buffer, u32 width, u32 height)
{
    u32 itemCount = width*height;
    int format = dataFormat & (MPXFRAMEFILE_BINARY | MPXFRAMEFILE_ASCII);
    if (format == -1) {
        int rc;
        if ((rc = detectType(fileName, format, dataType, itemCount)))
            return rc;
    }

    if (itemCount*getItemSize(dataType) != buffer.size()){
        buffer.reinit(itemCount*getItemSize(dataType));
    }

    int rc = 0;
    if (format == MPXFRAMEFILE_BINARY) {
        if (dataFormat & (MPXFRAMEFILE_SPARSEX | MPXFRAMEFILE_SPARSEXY)){
            rc = readBinarySparse(fileName, (dataFormat & MPXFRAMEFILE_SPARSEXY) != 0, start, end, buffer.data(), buffer.size(), dataType, width, height);
        }else
            rc = readBinary(fileName, start, end, buffer.data(), static_cast<u32>(buffer.size()));
    }
    else {
        if (dataFormat & (MPXFRAMEFILE_SPARSEX | MPXFRAMEFILE_SPARSEXY))
            rc = readAsciiSparse(fileName, (dataFormat & MPXFRAMEFILE_SPARSEXY) != 0, start, end, buffer.data(), buffer.size(), dataType, width, height);
        else
            rc = readAsciiMatrix(fileName, start, end, buffer.data(), static_cast<u32>(buffer.size()), dataType, width, height);
    }
    return rc;
}


int MpxFrameFile::writeBinary(const char *fileName, bool append, void* buffer, size_t byteSize)
{
    File file(fileName, append ? "a+b" : "wb");
    if (!file)
        return logErrorFileOpen(fileName);
    if (fwrite(buffer, 1, byteSize, file) < (size_t)byteSize)
        return logErrorFileWrite(fileName);
    return 0;
}

int MpxFrameFile::readBinary(const char *fileName, i64 startPos, i64 endPos, void* buffer, size_t byteSize)
{
    File file(fileName, "rb");
    if (!file)
        return logErrorFileOpen(fileName);
    if (startPos > 0 && fseek64(file, startPos, SEEK_SET))
        return logErrorFileSeek(fileName);
    assert(endPos == 0 || endPos-startPos == static_cast<i64>(byteSize));
    if (fread(buffer, 1, byteSize, file) < (size_t)byteSize)
        return ferror(file) ? logErrorFileRead(fileName) : logErrorFileBadFormat(fileName, "invalid number of items");
    return 0;
}

#define WRITE_ASCII(type, format1, format2) \
    {type* pbuff = (type*)buffer;\
    for (i = 0; i < width*height; i++)\
        if (fprintf(file, ((i+1) % width) ? format1 : format2, *(pbuff++)) < 0)\
            break;\
    }\

int MpxFrameFile::writeAsciiMatrix(const char *fileName, bool append, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    (void)byteSize;
    (void)buffer;
    File file(fileName, append ? "a+" : "w");
    if (!file)
        return logErrorFileOpen(fileName);
    u32 i = 0;
    switch(dataType){
        case DT_BYTE:   WRITE_ASCII(byte, "%u ", "%u\n"); break;
        case DT_I16:    WRITE_ASCII(i16, "%hd ", "%hd\n"); break;
        case DT_U16:    WRITE_ASCII(u16, "%u ", "%u\n"); break;
        case DT_I32:    WRITE_ASCII(i32, "%d ", "%d\n"); break;
        case DT_U32:    WRITE_ASCII(u32, "%u ", "%u\n"); break;
        case DT_U64:    WRITE_ASCII(u64, "%llu ", "%llu\n"); break;
        case DT_DOUBLE: WRITE_ASCII(double, "%lf ", "%lf\n"); break;
        default: fclose(file); return PXERR_UNSUPPORTED;
    }
    if (i != width*height)
        return logErrorFileWrite(fileName);
    return 0;
}

int MpxFrameFile::readAsciiMatrix(const char *fileName, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    (void)endPos;
    (void)byteSize;
    File file(fileName, "r");
    if (!file)
        return logErrorFileOpen(fileName);
    if (startPos > 0 && fseek64(file, startPos, SEEK_SET))
        return logErrorFileSeek(fileName);

    const char *format = 0;
    int size = 0;
    switch (dataType){
        case DT_BYTE: format = "%u"; size = sizeof(byte); break;
        case DT_I16: format = "%hd"; size = sizeof(i16); break;
        case DT_U16: format = "%u"; size = sizeof(u16); break;
        case DT_I32: format = "%d"; size = sizeof(i32); break;
        case DT_U32: format = "%u"; size = sizeof(u32); break;
        case DT_U64: format = "%llu"; size = sizeof(u64); break;
        case DT_DOUBLE: format = "%lf"; size = sizeof(double); break;
        default: return PXERR_INVALID_DATA_TYPE;
    }
    byte *pitem = (byte*)buffer;
    u32 i;
    for (i = 0; i < width*height; i++) {
        if (fscanf(file, format, pitem) != 1)
            break;
        pitem += size;
    }
    if (i != width*height)
        return ferror(file) ? logErrorFileRead(fileName) : logError(PXERR_FILE_BADDATA, "Format of ASCII file \"%s\" is invalid (read: %u items, expected: %u items)", fileName, i, width*height);
    return 0;
}

#define WRITE_ASCII_XY(type, format) \
    {type* buff = (type*)buffer;\
    for (u32 j = 0; j < height; j++)\
        for (u32 i = 0; i < width; i++, buff++)\
            if (*buff && fprintf(file, format, i, j, *buff) < 0)\
                return logErrorFileWrite(fileName);\
    }

#define WRITE_ASCII_X(type, format) \
    {type* buff = (type*)buffer;\
     for (u32 i = 0; i < width*height; i++, buff++)\
        if (*buff && fprintf(file, format, i, *buff) < 0)\
            return logErrorFileWrite(fileName);\
    }

int MpxFrameFile::writeAsciiSparse(const char *fileName, bool append, bool xy, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    (void)buffer;
    (void)byteSize;
    (void)width;
    bool exists = fileExists(fileName);
    File file(fileName, append ? "a+" : "w");
    if (!file)
        return logErrorFileOpen(fileName);
    if (append && exists)
        fputs("#\n", file);
    switch(dataType){
        case DT_BYTE:   if (xy) WRITE_ASCII_XY(byte, "%u\t%u\t%u\n") else WRITE_ASCII_X(byte, "%u\t%u\n") break;
        case DT_I16:    if (xy) WRITE_ASCII_XY(i16, "%u\t%u\t%hd\n") else WRITE_ASCII_X(i16, "%u\t%hd\n") break;
        case DT_U16:    if (xy) WRITE_ASCII_XY(u16, "%u\t%u\t%hu\n") else WRITE_ASCII_X(u16, "%u\t%hu\n") break;
        case DT_I32:    if (xy) WRITE_ASCII_XY(i32, "%u\t%u\t%d\n") else WRITE_ASCII_X(i32, "%u\t%d\n") break;
        case DT_U32:    if (xy) WRITE_ASCII_XY(u32, "%u\t%u\t%u\n") else WRITE_ASCII_X(u32, "%u\t%u\n") break;
        case DT_U64:    if (xy) WRITE_ASCII_XY(u64, "%u\t%u\t%llu\n") else WRITE_ASCII_X(u64, "%u\t%llu\n") break;
        case DT_DOUBLE: if (xy) WRITE_ASCII_XY(double, "%u\t%u\t%f\n") else WRITE_ASCII_X(double, "%u\t%f\n") break;
        default: return -1;
    }
    return 0;
}

int MpxFrameFile::readAsciiSparse(const char *fileName, bool xy, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    (void)endPos;
    (void)byteSize;
    File file(fileName, "rb");
    if (!file)
        return logErrorFileOpen(fileName);

    // +1 for # frame delimiter
    if (startPos > 0 && fseek64(file, startPos+1, SEEK_SET))
        return logErrorFileSeek(fileName);

    memset(buffer, 0, byteSize);

    // second frame after first zero-frame (# at startPos=0). skip #
    if (startPos == 0){
        char c;
        if (fread(&c, 1, 1, file) != 1) return 0;
        if (c != '#') // if not #, seek back
            fseek64(file, startPos, SEEK_SET);
    }

    const char *format = 0;
    switch(dataType){
        case DT_BYTE: format = xy ? "%u %u %u" : "%u %u"; break;
        case DT_I16: format = xy ? "%u %u %hd" : "%u %hd"; break;
        case DT_U16: format = xy ? "%u %u %hu" : "%u %hu"; break;
        case DT_I32: format = xy ? "%u %u %d" : "%u %d"; break;
        case DT_U32: format = xy ? "%u %u %u" : "%u %u"; break;
        case DT_U64: format = xy ? "%u %u %llu" : "%u %llu"; break;
        case DT_DOUBLE: format = xy ? "%u %u %lf" : "%u %lf"; break;
        default: return logError(PXERR_INVALID_ARGUMENT, "Invalid data type to read from file %s", fileName);
    }

    size_t size = sizeofType[dataType];
    u32 x = 0;
    u32 y = 0;
    byte tmp[16] = {0};
    byte* data = (byte*)buffer;
    if (xy) {
        while (fscanf(file, format, &x, &y, tmp) == 3) {
            if (x >= width || y >= height)
                return logErrorFileBadFormat(fileName, "more data in file than frame dimensions");
            memcpy(data+(y*width+x)*size, tmp, size);
        }
    } else {
        while (fscanf(file, format, &x, tmp) == 2) {
            if (x >= width*height)
                return logErrorFileBadFormat(fileName, "more data in file than frame dimensions");
            memcpy(data+x*size, tmp, size);
        }
    }
    return 0;
}

#define WRITE_BIN_XY(type, size) \
    {type* buff = (type*)buffer;\
    for (u32 j = 0; j < height; j++)\
        for (u32 i = 0; i < width; i++, buff++)\
            if (*buff && (fwrite(&i, sizeof(u32), 1, file) < 1 || fwrite(&j, sizeof(u32), 1, file) < 1 || fwrite(buff, size, 1, file) < 1))\
                return logErrorFileWrite(fileName);\
    }

#define WRITE_BIN_X(type, size) \
    {type* buff = (type*)buffer;\
    for (u32 i = 0; i < width*height; i++, buff++)\
        if (*buff && (fwrite(&i, sizeof(u32), 1, file) < 1 || fwrite(buff, size, 1, file) < 1))\
            return logErrorFileWrite(fileName);\
    }

int MpxFrameFile::writeBinarySparse(const char *fileName, bool append, bool xy, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    (void)byteSize;
    (void)width;
    (void)height;
    File file(fileName, append ? "a+b" : "wb");
    if (!file)
        return logErrorFileOpen(fileName);

    const size_t size = sizeofType[dataType];
    switch(dataType){
        case DT_BYTE:   if (xy) WRITE_BIN_XY(byte, size) else WRITE_BIN_X(byte, size) break;
        case DT_I16:    if (xy) WRITE_BIN_XY(i16, size) else WRITE_BIN_X(i16, size) break;
        case DT_U16:    if (xy) WRITE_BIN_XY(u16, size) else WRITE_BIN_X(u16, size) break;
        case DT_I32:    if (xy) WRITE_BIN_XY(i32, size) else WRITE_BIN_X(i32, size) break;
        case DT_U32:    if (xy) WRITE_BIN_XY(u32, size) else WRITE_BIN_X(u32, size) break;
        case DT_U64:    if (xy) WRITE_BIN_XY(u64, size) else WRITE_BIN_X(u64, size) break;
        case DT_DOUBLE: if (xy) WRITE_BIN_XY(double, size) else WRITE_BIN_X(double, size) break;
        default: return -1;
    }
    return 0;
}

int MpxFrameFile::readBinarySparse(const char *fileName, bool xy, i64 startPos, i64 endPos, void* buffer, size_t byteSize, DataType dataType, u32 width, u32 height)
{
    File file(fileName, "rb");
    if (!file)
        return logErrorFileOpen(fileName);
    if (fseek64(file, startPos, SEEK_SET))
        return logErrorFileSeek(fileName);

    memset(buffer, 0, byteSize);
    size_t size = sizeofType[dataType];
    byte tmp[32] = {0};
    u32* x = (u32*)tmp;
    u32* y = (u32*)(tmp + sizeof(u32));
    byte* data = (byte*)buffer;
    int readCount = 0;

    if (xy) {
        byte *val = tmp + 2*sizeof(u32);
        size_t itemSize = size + 2*sizeof(u32);
        int toRead = (int)((endPos-startPos)/itemSize);
        if (toRead > 0)
            while (fread(tmp, itemSize, 1, file) == 1) {
                if (*x >= width || *y >= height)
                    return logErrorFileBadFormat(fileName, "invalid xy coordinate (exceeding frame dimension)");
                memcpy(data + (*x + *y * width) * size, val, size);
                if (endPos > 0 && ++readCount == toRead)
                    break;
            }
    } else {
        byte *val = tmp + sizeof(u32);
        size_t itemSize = size + sizeof(u32);
        int toRead = (int)((endPos - startPos)/itemSize);
        if (toRead > 0)
            while (fread(tmp, itemSize, 1, file) == 1) {
                if (*x >= width*height)
                    return logErrorFileBadFormat(fileName, "invalid xy coordinate (exceeding frame dimension)");
                memcpy(data + *x * size, val, size);
                if (endPos > 0 && ++readCount == toRead)
                    break;
            }
    }
    if (ferror(file))
        return logErrorFileRead(fileName);
    return 0;
}

int MpxFrameFile::readDesc(const char* fileName, int index, i64 startPos, i64 endPos, u32 &dataFormat, DataType &dataType, u32 &width, u32 &height, std::map<std::string, MetaData*> &metaDatas)
{
    File file(fileName, "r");
    if (!file)
        return logErrorFileOpen(fileName);

    // read first line containing header A######## or B########
    char line[64];
    if (!fgets(line, 64, file))
        return logErrorFileRead(fileName);
    if (line[0] != 'A' && line[0] != 'B')
        return logErrorFileDscInvalid(fileName, "Invalid header");
    if (startPos != 0 && fseek64(file, startPos, SEEK_SET))
        return logErrorFileSeek(fileName);

    // read frame header
    char sType[16] = "";
    char sFormat[16] = "";
    int frameIdx;
    if (fscanf(file, "\n[F%d]\nType=%15s %15s width=%u height=%u\n", &frameIdx, sType, sFormat, &width, &height) != 5)
        return ferror(file) ? logErrorFileRead(fileName) : logErrorFileDscInvalid(fileName, "Frame header reading failed");
    if (frameIdx != index)
        return logError(PXERR_FILE_BADDATA, "Inconsistency between index file and dsc file.");

    if (strToType(sType, dataType) || strToFormat(sFormat, dataFormat))
        return logErrorFileDscInvalid(fileName, "invalid frame header");
    dataFormat |= (line[0] == 'B' ? MPXFRAMEFILE_BINARY : MPXFRAMEFILE_ASCII);

    char format[128] = "";
    char name[NAME_LENGTH] = "";
    char desc[DESC_LENGTH] = "";
    char strData[MAX_METADATA_STRSIZE] = "";
    int count = 0;
    const char *err = 0;
    snprintf(format, 128, "\"%%%u[^\"]\" (\"%%%u[^\"]\"):\n%%16[^[][%%d]\n%%%u[^\n]\n\n", NAME_LENGTH, DESC_LENGTH, MAX_METADATA_STRSIZE);

    while (fscanf(file, format, name, desc, sType, &count, strData) == 5) {
        DataType type = getTypeFromStr(sType);
        if (type == DT_LAST) {
            err = "Invalid data in desc. file";
            break;
        }

        MetaData *metaData = new MetaData(name, desc, type, 0, count*sizeofType[type]);
        metaDatas[name] = metaData;
        if (!metaData || (count > 0 && !metaData->data()))
            return logErrorMemoryAlloc(count*sizeofType[type]+sizeof(MetaData));
        if (count > 0) {
            if (getDataFromString(type, metaData->data(), count, strData)) {
                err = "Data conversion error in desc. file";
                break;
            }
        }
    }
    if (ferror(file))
        return logErrorFileRead(fileName);

    if (!feof(file)) {
        if (err)
            return logErrorFileDscInvalid(fileName, err);
        i64 curPos = ftell64(file);
        if (curPos < endPos)
            return logErrorFileDscInvalid(fileName, "invalid format of metaDataute");
    }
    return 0;
}

int MpxFrameFile::writeDesc(const char* fileName, u32 dataFormat, DataType dataType, u32 width, u32 height,
                            std::map<std::string, MetaData*> &metaDatas)
{
    bool add = (dataFormat & MPXFRAMEFILE_APPEND) && fileExists(fileName);
    File file(fileName, add ? "r+" : "w");
    if (!file)
        return logErrorFileOpen(fileName);
    int count = 0;
    char format = dataFormat & MPXFRAMEFILE_BINARY ? 'B' : 'A';
    if (add) {
        if (readDescHeader(file, format, count) || (format != 'A' && format != 'B'))
            return logErrorFileDscInvalid(fileName, "Invalid header");
        if ((format == 'A' && (dataFormat & MPXFRAMEFILE_BINARY)) || (format == 'B' && (dataFormat & MPXFRAMEFILE_ASCII))) {
            return logError(PXERR_INVALID_ARGUMENT, "Format is different than format of existing file, %s",
                format == 'A' ? "binary, existing file: ASCII." : "ASCII, existing file: binary.");
        }
    }
    if (writeDescHeader(file, format, count + 1))
        logErrorFileWrite(fileName);
    if (fseek(file, 0, SEEK_END))
        return -1;

    // frame header
    if (fprintf(file, "\n[F%d]\n", count) < 0)
        logErrorFileWrite(fileName);
    if (fprintf(file, "Type=%s %s width=%u height=%u\n", typeNames[dataType], formatToStr(dataFormat), width, height) < 0)
        return logErrorFileWrite(fileName);

    std::map<std::string, MetaData*>::iterator it;
    for (it = metaDatas.begin(); it != metaDatas.end(); ++it) {
        MetaData *metaData = it->second;
        char strData[MAX_METADATA_STRSIZE] = "";
        getStringFromData(metaData->mType, metaData->data(), metaData->size()/sizeofType[metaData->type()], strData, MAX_METADATA_STRSIZE, true);
        if (strData[0] && fprintf(file, "\"%s\" (\"%s\"):\n%s[%u]\n%s\n\n", metaData->name(),
            metaData->mDesc.c_str(), typeNames[metaData->mType],static_cast<u32>(metaData->size()/sizeofType[metaData->mType]), strData) < 0)
        {
            return logErrorFileWrite(fileName);
        }
    }
    return 0;
}

int MpxFrameFile::getFrameCount(const char* fileName, u32 &count)
{
    std::string sFileName = fileName;
    if (!hasFileExt(fileName, "dsc")){
        sFileName += ".dsc";
        if (!fileExists(sFileName.c_str())){
            sFileName = changeFileExt(fileName, "dsc");
            if (!fileExists(sFileName.c_str())){
                if (isValidFrameFile(fileName)){
                    count = 1;
                    return 0;
                }
            }
        }
    }

    File file(sFileName.c_str(), "r");
    if (!file) {
        count = 0;
        return logError(PXERR_FILE_OPEN, "Cannot open file \"%s\" for reading: %s.", sFileName.c_str(), strerror(errno));
    }
    char type;
    if (fscanf(file, "%c%d", &type, &count) < 2) {
        count = 1;
        return logError(PXERR_FILE_OPEN, "Cannot read frame count from file \"%s\", invalid dsc file.", sFileName.c_str());
    }
    return 0;
}

int MpxFrameFile::updateIndexFile(std::string dataFileName, IndexItem &indexItem)
{
    std::string fileName =  dataFileName + "." + IDX_FILE_EXT;
    File file(fileName.c_str(), "ab");
    if (!file)
        return logErrorFileOpen(fileName.c_str());
    if (fwrite(&indexItem, sizeof(IndexItem), 1, file) != 1)
        return logErrorFileWrite(fileName.c_str());
    return 0;
}


int MpxFrameFile::getIndexItem(const char* idxFileName, int frameIndex, IndexItem &itemStart, IndexItem &itemEnd)
{
    File file(idxFileName, "rb");
    if (!file)
        return logErrorFileOpen(idxFileName);
    if (frameIndex > 0) {
        if (fseek64(file, (frameIndex-1) * sizeof(IndexItem), SEEK_SET))
            return logErrorFileSeek(idxFileName);
        if (fread(&itemStart, sizeof(IndexItem), 1, file) != 1)
            return logErrorFileRead(idxFileName);
    } else {
        itemStart.dataPos = 0;
        itemStart.dscPos = 0;
        itemStart.sfPos = 0;
    }
    if (fread(&itemEnd, sizeof(IndexItem), 1, file) != 1 && !feof(file))
        return logErrorFileRead(idxFileName);
    return 0;
}

int MpxFrameFile::getCurrentIndexItem(std::string dataFileName, IndexItem &indexItem)
{
    if ((indexItem.dataPos = getFileSize(dataFileName.c_str())) < 0)
        return -1;
    std::string dscFile = dataFileName + "." + DESC_FILE_EXT;
    if ((indexItem.dscPos = getFileSize(dscFile.c_str())) < 0)
        return -1;
    return 0;
}


void MpxFrameFile::getFileNames(std::string fileName, std::string  &dataFileName, std::string &dscFileName, std::string &idxFileName)
{
    if (hasFileExt(fileName, DESC_FILE_EXT)) {
        dscFileName = fileName;
        dataFileName = changeFileExt(dscFileName, "");
        idxFileName = changeFileExt(dscFileName, IDX_FILE_EXT);
    } else if (hasFileExt(fileName, IDX_FILE_EXT)) {
        idxFileName = fileName;
        dataFileName = changeFileExt(idxFileName, "");
        dscFileName = changeFileExt(idxFileName, DESC_FILE_EXT);
    } else {
        dataFileName = fileName;
        dscFileName = fileName + "." + DESC_FILE_EXT;
        idxFileName = fileName + "." + IDX_FILE_EXT;
    }
    if (!fileExists(dataFileName.c_str())) dataFileName = "";
    if (!fileExists(dscFileName.c_str())) dscFileName = "";
    if (!fileExists(idxFileName.c_str())) idxFileName = "";
}

int MpxFrameFile::getLineItemsCount(FILE *file, u32 *count)
{
    fpos_t pos;
    if (fgetpos(file, &pos))
        return -1;

    u32 cnt = 0;
    int rc = 0;
    int c = 0;
    bool numPos = false;
    while (1) {
        c = fgetc(file);
        if (c == '\n' || c == '\r' || c == EOF)
            break;
        else if ((c >= '0' && c <= '9') || c == 'E' || c == 'e' || c == '.' || c == '+' || c == '-') {
            if (!numPos) {
                numPos = true;
                cnt++;
            }
        }
        else if (c == ' ' || c == '\t')
            numPos = false;
        else {
            rc = -1;
            break;
        }
    }
    *count = cnt;
    return rc;
}

int MpxFrameFile::getLinesCount(const char* fileName, u32* count)
{
    File file(fileName, "rb");
    if (!file)
        return -1;
    const u32 buffSize = 16*1024;
    char buff[buffSize];
    *count = 0;
    size_t read = 0;
    while ((read = fread(buff, 1, buffSize, file)) > 0)
    {
        for (size_t i = 0; i < read; i++)
            if (buff[i] == '\n')
                (*count)++;
        if (read < buffSize && buff[read - 1] != '\n')
            (*count)++;
    }
    return ferror(file);
}


int MpxFrameFile::getSparseDim(FILE* file, u32* width, u32* height)
{
    char line[4096];
    *width = *height = 0;
    u32 x = 0, y = 0;
    while (fgets(line, 4096, file))
    {
        if (sscanf(line, "%u %u", &x, &y) == EOF)
            return -1;
        if (x >= *width)
            *width = x + 1;
        if (y >= *height)
            *height = y + 1;
    }
    return 0;
}

int MpxFrameFile::getSparseDim(FILE* file, u32* size)
{
    char line[4096];
    *size = 0;
    u32 x = 0;
    while (fgets(line, 4096, file))
    {
        if (sscanf(line, "%u", &x) == EOF)
            return -1;
        if (x >= *size)
            *size = x + 1;
    }
    return 0;
}

int MpxFrameFile::detectType(const char* fileName, int &format, DataType &type, u32 itemCount, bool silent)
{
    byte tmpBuff[4096];
    File file(fileName, "rb");
    if (!file)
        return logErrorFileOpen(fileName);

    u32 count;
    bool flPoint = false;
    bool binary = false;
    bool cont = true;
    while (cont) {
        count = (u32) fread(tmpBuff, 1, 4096, file);
        if (count < 4096) {
            cont = false;
            if (ferror(file))
                return logErrorFileRead(fileName);
        }
        for (u32 i = 0; i < count; i++) {
            byte b = tmpBuff[i];
            if (b == 'E' || b == 'e' || b == '.')
                flPoint = true;
            else if ((b < '0' || b > '9') && b != '\n' && b != '\r' && b != ' ' && b != '\t' && b != '+' && b != '-') {
                binary = true;
                cont = false;
                break;
            }
        }
    }

    if (binary) {
        format = MPXFRAMEFILE_BINARY;
        fseek(file, 0, SEEK_END);
        size_t fsize = static_cast<size_t>(ftell64(file));
        if (fsize == (itemCount * sizeof(i16)))
            type = DT_I16;
        else if (fsize == (itemCount * sizeof(u32)))
            type = DT_U32;
        else if (fsize == (itemCount * sizeof(u64)))
            type = DT_U64;
        else if (fsize == itemCount * sizeof(double))
            type = DT_DOUBLE;
        else
            return !silent ? logError(PXERR_UNEXPECTED, "Unexpected error during frame operation.") : PXERR_UNEXPECTED;
    } else {
        format = MPXFRAMEFILE_ASCII;
        type = flPoint ? DT_DOUBLE : DT_U32;
    }
    return 0;
}

int MpxFrameFile::guessFileType(const char* fileName, u32 &dataFormat, DataType &dataType, u32 &width, u32 &height, bool silent)
{
    int rc = 0, format = 0;
    DataType type = DT_LAST;
    if ((rc = detectType(fileName, format, type, width*height, silent)))
        return rc;

    dataFormat = format;
    if (dataFormat == MPXFRAMEFILE_BINARY)
        return !silent ? logErrorFileBadFormat(fileName, "cannot read binary data file without corresponding description file") : PXERR_FILE_BADDATA;

    File file(fileName, "r");
    if (!file)
        return !silent ? logErrorFileOpen(fileName) : PXERR_FILE_OPEN;

    if (getLineItemsCount(file, &width))
        return !silent ? logErrorFileBadFormat(fileName, "it is not ASCII matrix") : PXERR_FILE_BADDATA;
    if (width == 0)
        return !silent ? logError(PXERR_UNEXPECTED, "Unexpected error during frame operation.") : PXERR_UNEXPECTED;
    if (width == 2 || width == 3) {
        bool xy = width == 3;
        if (xy)
            getSparseDim(file, &width, &height);
        else {
            u32 itemCount = 0;
            getSparseDim(file, &itemCount);
            height = width = (u32) (sqrt((double)itemCount));
        }
        width = ((width + 255) / 256) * 256;
        height = ((height + 255) / 256) * 256;
        dataFormat |= (xy ? MPXFRAMEFILE_SPARSEXY : MPXFRAMEFILE_SPARSEX);
    }
    else {
        if (getLinesCount(fileName, &height))
            return logErrorFileOpen(fileName);
    }
    dataType = (type == DT_DOUBLE) ? DT_DOUBLE : DT_U32;
    return 0;
}

bool MpxFrameFile::isValidFrameFile(const char* fileName)
{
    unsigned frameCount = 0;
    std::string dataFileName = "";
    std::string dscFileName = "";
    std::string idxFileName = "";
    getFileNames(fileName, dataFileName, dscFileName, idxFileName);

    // try to read frame count from dsc file
    if (!dscFileName.empty() && getFrameCount(dscFileName.c_str(), frameCount))
        return false;

    // multi-frame file -> need index file and dsc
    if (frameCount > 1 && idxFileName.empty())
        return false;

    // if dsc file do not exist, try to guess file type
    if (dscFileName.empty() && idxFileName.empty()) {
        unsigned width, height, dataFormat;
        DataType dataType;
        if (guessFileType(fileName, dataFormat, dataType, width, height, true))
            return false;
    }
    return true;
}

int MpxFrameFile::writeDescHeader(FILE* file, char format, int count)
{
    if (fseek(file, 0, SEEK_SET))
        return -1;
    if (fprintf(file, "%c%09d", format, count) < 0)
        return -1;
    return 0;
}

int MpxFrameFile::readDescHeader(FILE* file, char &format, int &count)
{
    if (fseek(file, 0, SEEK_SET))
        return -1;
    if (fscanf(file, "%c%d", &format, &count) < 0)
        return -1;
    return 0;
}

int MpxFrameFile::strToType(const char* strType, DataType &type)
{
    if (strcmp(strType, typeNames[DT_I16]) == 0)
        type = DT_I16;
    else if (strcmp(strType, typeNames[DT_U32]) == 0)
        type = DT_U32;
    else if (strcmp(strType, typeNames[DT_U64]) == 0)
        type = DT_U64;
    else if (strcmp(strType, typeNames[DT_DOUBLE]) == 0)
        type = DT_DOUBLE;
    else
        return -1;
    return 0;
}

#define NAME_SPARSE_X    "[X,C]"
#define NAME_SPARSE_XY   "[X,Y,C]"
#define NAME_MATRIX      "matrix"
inline const char* MpxFrameFile::formatToStr(u32 flags)
{
    if (flags & MPXFRAMEFILE_SPARSEX)
        return NAME_SPARSE_X;
    else if (flags & MPXFRAMEFILE_SPARSEXY)
        return NAME_SPARSE_XY;
    else
        return NAME_MATRIX;
}

int MpxFrameFile::strToFormat(const char* strFormat, u32 &format)
{
    if (strcmp(strFormat, NAME_SPARSE_X) == 0)
        format = MPXFRAMEFILE_SPARSEX;
    else if (strcmp(strFormat, NAME_SPARSE_XY) == 0)
        format = MPXFRAMEFILE_SPARSEXY;
    else if (strcmp(strFormat, NAME_MATRIX) == 0)
        format = 0;
    else
        return -1;
    return 0;
}

inline u32 MpxFrameFile::getItemSize(int type)
{
    if (type == DT_I16)
        return sizeof(i16);
    else if (type == DT_U32)
        return sizeof(32);
    else if (type == DT_DOUBLE)
        return sizeof(double);
    else if (type == DT_U64)
        return sizeof(u64);
    else
        return 0;
}


int MpxFrameFile::logError(int err, const char* text, ...)
{
    va_list args;
    va_start(args, text);
#ifdef TESTING
    int size = 512;
    std::string str;
    while (1) {
        str.resize(size);
        int n = vsnprintf((char *)str.c_str(), size, text, args);
        if (n > -1 && n < size) break;
        size = n > -1 ? n+1 : 2*size;
    }
    printf("ERROR: %s\n", str.c_str());
#else
//    pxLogMsgV(0, text, args);
#endif
    va_end(args);
    return err;
}

inline int MpxFrameFile::logErrorFileOpen(const char *fileName)
{
    return logError(PXERR_FILE_OPEN, "Cannot open file \"%s\" (%s)", fileName, strerror(errno));
}

inline int MpxFrameFile::logErrorFileSeek(const char *fileName)
{
    return logError(PXERR_FILE_SEEK, "Cannot Seek file \"%s\" (%s)", fileName, strerror(errno));
}

inline int MpxFrameFile::logErrorFileRead(const char *fileName)
{
    return logError(PXERR_FILE_READ, "Cannot read file \"%s\" (%s)", fileName, strerror(errno));
}

inline int MpxFrameFile::logErrorFileWrite(const char *fileName)
{
    return logError(PXERR_FILE_WRITE, "Cannot write to file \"%s\" (%s)", fileName, strerror(errno));
}

inline int MpxFrameFile::logErrorFileBadFormat(const char *fileName, const char* msg)
{
    return logError(PXERR_FILE_BADDATA, "Invalid file (%s) format (%s)", fileName, msg);
}

inline int MpxFrameFile::logErrorFileDscInvalid(const char *fileName, const char* msg)
{
    return logError(PXERR_FILE_BADDATA, "Invalid format of dsc file (%s): %s", fileName, msg);
}

inline int MpxFrameFile::logErrorMemoryAlloc(size_t bytes)
{
    return logError(PXERR_MEMORY_ALLOC, "Memory allocation of %u bytes failed.", bytes);
}


//#########################################################################################################################
//                                                      HDF5
//#########################################################################################################################

namespace str{
    inline std::vector<std::string> split(const std::string &str, const std::string delims, bool skipEmpty=false, unsigned maxItems=0)
    {
        std::vector<std::string> vect;
        std::string item;
        size_t start = 0, end = 0;
        while (end != std::string::npos){
            end = str.find_first_of(delims, start);
            item = str.substr(start, (end == std::string::npos) ? std::string::npos : end-start);
            if (!skipEmpty || !item.empty())
                vect.push_back(item);
            start = (end > std::string::npos - delims.size()) ? std::string::npos : end + delims.size();
            if (maxItems != 0 && vect.size()+1 == maxItems && end != std::string::npos){
                item = str.substr(start, std::string::npos);
                vect.push_back(item);
                return vect;
            }
        }
        return vect;
    }

    inline bool endsWith(const std::string &fullstr, const std::string &ending)
    {
        return (fullstr.length() >= ending.length() ? (fullstr.compare(fullstr.length() - ending.length(), ending.length(), ending)==0) : false);
    }

    inline bool startsWith(const char *s1, const char *s2)
    {
        return strlen(s1) < strlen(s2) ? false : strncmp(s2, s1, strlen(s2)) == 0;
    }

    inline bool startsWith(const std::string &s1, const std::string &s2)
    {
        return s2.size() <= s1.size() && s1.compare(0, s2.size(), s2) == 0;
    }

    inline std::string format(const std::string fmt, ...)
    {
        int size = 512;
        std::string str;
        va_list ap;
        while (1) {
            str.resize(size);
            va_start(ap, fmt);
#ifdef _MSC_VER
            int n = _vsnprintf_s((char*)str.c_str(), size, size-1, fmt.c_str(), ap);
#else
            int n = vsnprintf((char*)str.c_str(), size, fmt.c_str(), ap);
#endif
            va_end(ap);
            if (n > -1 && n < size) {
                str.resize(strlen(str.c_str()));
                return str;
            }
            size = (n>-1)? n+1 : 2*size;
        }
    }


}

#ifndef NO_HDF5
class HDF
{
public:
    static const int ERR_ERROR;
    static const int ERR_CANNOT_CREATE_PATH;
    static const int ERR_INVALID_SIZE;
    static const bool OVERWRITE;
    static const bool READWRITE;

    enum Type { CHAR, BYTE, I16, U16, I32, U32, I64, U64, FLOAT, DOUBLE, STRING, UNKNOWN};

public:
    HDF();
    virtual ~HDF();
    virtual void setCompressionLevel(byte level = 3) { mCompressLevel = level; }

    virtual int open(std::string filePath, bool overwrite = READWRITE, byte compressLevel = 0);
    virtual int close();

    virtual bool exists(std::string path) const;
    virtual Type type(std::string path);
    virtual int length(std::string path);
    virtual std::vector<std::string> subItems(std::string path);
    virtual int remove(std::string path);

    virtual int setFloat(std::string path, float value);
    virtual int setDouble(std::string path, double value);
    virtual int setChar(std::string path, char value);
    virtual int setByte(std::string path, byte value);
    virtual int setI16(std::string path, i16 value);
    virtual int setU16(std::string path, u16 value);
    virtual int setI32(std::string path, i32 value);
    virtual int setU32(std::string path, u32 value);
    virtual int setI64(std::string path,  i64 value);
    virtual int setU64(std::string path, u64 value);
    virtual int setBool(std::string path, bool value);
    virtual int setString(std::string path, std::string value);

    virtual int setByteBuff(std::string path, const byte* buff, size_t size);
    virtual int setDoubleBuff(std::string path, const double* buff, size_t size);
    virtual int setI16Buff(std::string path, const i16* buff, size_t size);
    virtual int setU16Buff(std::string path, const u16* buff, size_t size);
    virtual int setI32Buff(std::string path, const i32* buff, size_t size);
    virtual int setU32Buff(std::string path, const u32* buff, size_t size);
    virtual int setI64Buff(std::string path, const i64* buff, size_t size);
    virtual int setU64Buff(std::string path, const u64* buff, size_t size);

    virtual float getFloat(std::string path, float defaultVal);
    virtual double getDouble(std::string path, double defaultVal);
    virtual char getChar(std::string path, char defaultVal);
    virtual byte getByte(std::string path, byte defaultVal);
    virtual i16 getI16(std::string path, i16 defaultVal);
    virtual u16 getU16(std::string path, u16 defaultVal);
    virtual i32 getI32(std::string path, i32 defaultVal);
    virtual u32 getU32(std::string path, u32 defaultVal);
    virtual i64 getI64(std::string path, i64 defaultVal);
    virtual u64 getU64(std::string path, u64 defaultVal);
    virtual bool getBool(std::string path, bool defaultVal);
    virtual std::string getString(std::string path, std::string defaultVal);

    virtual int getByteBuff(std::string path, byte* buff, size_t size);
    virtual int getDoubleBuff(std::string path, double* buff, size_t size);
    virtual int getI16Buff(std::string path, i16* buff, size_t size);
    virtual int getU16Buff(std::string path, u16* buff, size_t size);
    virtual int getI32Buff(std::string path, i32* buff, size_t size);
    virtual int getU32Buff(std::string path, u32* buff, size_t size);
    virtual int getI64Buff(std::string path, i64* buff, size_t size);
    virtual int getU64Buff(std::string path, u64* buff, size_t size);

protected:
    virtual int createPathDir(std::string path);
    inline int setCompressDataSet(std::string path, const void* buff, size_t size, int type);

protected:
    int mFile;
    byte mCompressLevel;
};

#define CHUNK_SIZE 65536

const int HDF::ERR_ERROR = -1;
const int HDF::ERR_CANNOT_CREATE_PATH = -2;
const int HDF::ERR_INVALID_SIZE = -3;
const bool HDF::OVERWRITE = true;
const bool HDF::READWRITE = false;


static herr_t file_info(hid_t loc_id, const char *name, void *opdata)
{
    std::vector<std::string>* items = (std::vector<std::string>*)opdata;
    H5G_stat_t statbuf;

    H5Gget_objinfo(loc_id, name, FALSE, &statbuf);
    items->push_back(name);
    //statbuf.type) - H5G_GROUP,H5G_DATASET, H5G_TYPE
    return 0;
}


HDF::HDF()
    : mFile(0)
    , mCompressLevel(2)
{

}

HDF::~HDF()
{
    close();
}

int HDF::open(std::string filePath, bool overwrite, byte compressLevel)
{
    mCompressLevel = compressLevel;
    if (fileExists(filePath.c_str()) && !overwrite)
        mFile = H5Fopen(filePath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    else
        mFile = H5Fcreate(filePath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    return mFile ? 0 : -1;
}

int HDF::close()
{
    if (mFile){
        H5Fclose(mFile);
        mFile = 0;
    }
    return 0;
}

bool HDF::exists(std::string path) const
{
    return H5Lexists(mFile, path.c_str(), H5P_DEFAULT) != 0;
}

HDF::Type HDF::type(std::string path)
{
    Type out = UNKNOWN;
    hid_t did = H5Dopen2(mFile, path.c_str(), H5P_DEFAULT);
    hid_t tid = H5Dget_type(did);
    hid_t type = H5Tget_native_type(tid, H5T_DIR_ASCEND);
    H5T_class_t cl =  H5Tget_class(tid);

    if (cl == H5T_STRING) out = STRING;
    if (H5Tequal(type, H5T_NATIVE_CHAR)) out = CHAR;
    if (H5Tequal(type, H5T_NATIVE_UCHAR)) out = BYTE;
    if (H5Tequal(type, H5T_NATIVE_INT16)) out = I16;
    if (H5Tequal(type, H5T_NATIVE_UINT16)) out = U16;
    if (H5Tequal(type, H5T_NATIVE_INT32)) out = I32;
    if (H5Tequal(type, H5T_NATIVE_UINT32)) out = U32;
    if (H5Tequal(type, H5T_NATIVE_INT64)) out = I64;
    if (H5Tequal(type, H5T_NATIVE_UINT64)) out = U64;
    if (H5Tequal(type, H5T_NATIVE_FLOAT)) out = FLOAT;
    if (H5Tequal(type, H5T_NATIVE_DOUBLE)) out = DOUBLE;

    H5Tclose(tid);
    H5Dclose(did);
    return out;
}

std::vector<std::string> HDF::subItems(std::string path)
{
    std::vector<std::string> items;
    H5Giterate(mFile, path.c_str(), NULL, file_info, (void*)&items);
    return items;
}

int HDF::remove(std::string path)
{
    return H5Ldelete(mFile, path.c_str(), H5P_DEFAULT);
}

int HDF::setFloat(std::string path, float value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_FLOAT, &value);
}

int HDF::setDouble(std::string path, double value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_DOUBLE, &value);
}

int HDF::setChar(std::string path, char value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_CHAR, &value);
}

int HDF::setByte(std::string path, byte value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UCHAR, &value);
}

int HDF::setI16(std::string path, i16 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT16, &value);
}

int HDF::setU16(std::string path, u16 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT16, &value);
}

int HDF::setI32(std::string path, i32 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT32, &value);
}

int HDF::setU32(std::string path, u32 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT32, &value);
}

int HDF::setI64(std::string path,  i64 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT64, &value);
}

int HDF::setU64(std::string path, u64 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT64, &value);
}

int HDF::setBool(std::string path, bool value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {1};
    byte bvalue = value ? 1 : 0;
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UCHAR, &bvalue);
}

int HDF::setString(std::string path, std::string value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    return H5LTmake_dataset_string(mFile, path.c_str(), value.c_str());
}


int HDF::setCompressDataSet(std::string path, const void* buff, size_t size, int type)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {size};
    hsize_t chunk[1] = {std::min((size_t)CHUNK_SIZE, size)};
    if (mCompressLevel > 0){
        hid_t space = H5Screate_simple(1, dims, NULL);
        hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
        herr_t status = H5Pset_deflate(dcpl, mCompressLevel);
        status = H5Pset_chunk(dcpl, 1, chunk);
        hid_t dset = H5Dcreate(mFile, path.c_str(), H5T_STD_I32LE, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
        status += H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff);
        status += H5Pclose(dcpl);
        status += H5Dclose(dset);
        status += H5Sclose(space);
        return status;
    } else
        return H5LTmake_dataset(mFile, path.c_str(), 1, dims, type, buff);
}

int HDF::setByteBuff(std::string path, const byte* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_UCHAR);
}

int HDF::setDoubleBuff(std::string path, const double* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_DOUBLE);
}

int HDF::setI16Buff(std::string path, const i16* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_INT16);
}

int HDF::setU16Buff(std::string path, const u16* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_UINT16);
}

int HDF::setI32Buff(std::string path, const i32* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_INT32);
}

int HDF::setU32Buff(std::string path, const u32* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_UINT32);
}

int HDF::setI64Buff(std::string path, const i64* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_INT64);
}

int HDF::setU64Buff(std::string path, const u64* buff, size_t size)
{
    return setCompressDataSet(path, buff, size, H5T_NATIVE_UINT64);
}

float HDF::getFloat(std::string path, float defaultVal)
{
    float value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_FLOAT, &value);
    return value;
}

double HDF::getDouble(std::string path, double defaultVal)
{
    double value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_DOUBLE, &value);
    return value;
}

char HDF::getChar(std::string path, char defaultVal)
{
    char value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_CHAR, &value);
    return value;
}

byte HDF::getByte(std::string path, byte defaultVal)
{
    byte value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UCHAR, &value);
    return value;
}

i16 HDF::getI16(std::string path, i16 defaultVal)
{
    i16 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT16, &value);
    return value;
}

u16 HDF::getU16(std::string path, u16 defaultVal)
{
    u16 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT16, &value);
    return value;
}

i32 HDF::getI32(std::string path, i32 defaultVal)
{
    i32 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT32, &value);
    return value;
}

u32 HDF::getU32(std::string path, u32 defaultVal)
{
    u32 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT32, &value);
    return value;
}

i64 HDF::getI64(std::string path, i64 defaultVal)
{
    i64 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT64, &value);
    return value;
}

u64 HDF::getU64(std::string path, u64 defaultVal)
{
    u64 value = defaultVal;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT64, &value);
    return value;
}

bool HDF::getBool(std::string path, bool defaultVal)
{
    char value = defaultVal ? 1 : 0;
    H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_CHAR, &value);
    return value != 0;
}

std::string HDF::getString(std::string path, std::string defaultVal)
{
    int size = length(path);

    char* buff = new char[size + 1];
    memset(buff, 0, size + 1);
    std::string str = defaultVal;

    if (H5LTread_dataset_string(mFile, path.c_str(), buff) >= 0)
        str = std::string(buff);

    delete[] buff;
    return str;
}

int HDF::length(std::string path)
{
    hsize_t dims[2] = {0, 0};
    size_t size = 0;
    H5LTget_dataset_info(mFile, path.c_str(), dims, NULL, &size);
    if (dims[0] == 0)
        return (int)size;
    return (int)dims[0];
}

int HDF::getByteBuff(std::string path, byte* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UCHAR, buff) < 0)
        return -1;
    return 0;
}

int HDF::getDoubleBuff(std::string path, double* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_DOUBLE, buff) < 0)
        return -1;
    return 0;
}

int HDF::getI16Buff(std::string path, i16* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT16, buff) < 0)
        return -1;
    return 0;
}

int HDF::getU16Buff(std::string path, u16* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT16, buff) < 0)
        return -1;
    return 0;
}

int HDF::getI32Buff(std::string path, i32* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT32, buff) < 0)
        return -1;
    return 0;
}

int HDF::getU32Buff(std::string path, u32* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT32, buff) < 0)
        return -1;
    return 0;
}

int HDF::getI64Buff(std::string path, i64* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_INT64, buff) < 0)
        return -1;
    return 0;
}

int HDF::getU64Buff(std::string path, u64* buff, size_t size)
{
    if (length(path) != (int)size)
        return ERR_INVALID_SIZE;

    if (H5LTread_dataset(mFile, path.c_str(), H5T_NATIVE_UINT64, buff) < 0)
        return -1;
    return 0;
}

int HDF::createPathDir(std::string path)
{
    std::vector<std::string> items = str::split(path, "/");
    std::string newpath;
    // skip the first (/) and last item (name of the actual dataset)
    for (size_t i = 1; i < items.size() - 1; i++){
        newpath += "/" + items[i];
        if (exists(newpath))
            continue;
        hid_t group_id = H5Gcreate(mFile, newpath.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Gclose(group_id);
    }
    return 0;
}

#endif

//#########################################################################################################################
//                                                      MPX FRAME
//#########################################################################################################################



MpxFrame::MpxFrame()
    : mData(NULL)
    , mDataType(FRMT_DOUBLE)
    , mWidth(0)
    , mHeight(0)
    , mSize(0)
{

}

MpxFrame::~MpxFrame()
{
    clear();
}

int MpxFrame::init(size_t width, size_t height, FrameType type)
{
    switch(type){
        case FRMT_I16: mData = (byte*)new i16[width * height]; break;
        case FRMT_U32: mData = (byte*)new u32[width * height]; break;
        case FRMT_U64: mData = (byte*)new u64[width * height]; break;
        case FRMT_DOUBLE: mData = (byte*)new double[width * height]; break;
    }
    mWidth = width;
    mHeight = height;
    mDataType = type;
    mSize = width * height;
    return 0;
}

size_t MpxFrame::frameCount(const char* filePath) const
{
    if (str::endsWith(filePath, ".h5")){
#ifndef NO_HDF5
        HDF hdf;

        int rc = hdf.open(filePath);
        if (rc)
            return 0;
        std::vector<std::string> subItems = hdf.subItems("/");
        size_t frameCount = 0;
        for (size_t i = 0; i < subItems.size(); i++){
            if (str::startsWith(subItems[i], "Frame"))
                frameCount++;
        }
        hdf.close();
        return frameCount;
#else
        return PXERR_UNSUPPORTED;
#endif
    }
    u32 count = 0;
    int rc = MpxFrameFile::getFrameCount(filePath, count);
    return rc == 0 ? count : 0;
}

#ifndef NO_HDF5
static void loadMetaDataFromHDF(HDF* hdf, std::map<std::string, MetaData*>& metaData, std::string framePath)
{
    std::string path = framePath + "/MetaData";
    if (!hdf->exists(path))
        return;

    std::vector<std::string> metaNames = hdf->subItems(path);
    for (size_t i = 0; i < metaNames.size(); i++){
        std::string metaPath = path + "/" + metaNames[i];
        HDF::Type dtype = hdf->type(metaPath);
        size_t count = hdf->length(metaPath);

        if (count == 1){
            if (dtype == HDF::CHAR) { char val = hdf->getChar(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_CHAR, &val, 1); }
            if (dtype == HDF::BYTE) { byte val = hdf->getByte(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_BYTE, &val, 1); }
            if (dtype == HDF::FLOAT) { float val = hdf->getFloat(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_FLOAT, &val, sizeof(float)); }
            if (dtype == HDF::DOUBLE) { double val = hdf->getDouble(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_DOUBLE, &val, sizeof(double)); }
            if (dtype == HDF::I16) { i16 val = hdf->getI16(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I16, &val, sizeof(i16)); }
            if (dtype == HDF::U16) { u16 val = hdf->getU16(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U16, &val, sizeof(u16)); }
            if (dtype == HDF::I32) { i32 val = hdf->getI32(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I32, &val, sizeof(i32)); }
            if (dtype == HDF::U32) { u32 val = hdf->getU32(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U32, &val, sizeof(u32)); }
            if (dtype == HDF::I64) { i64 val = hdf->getI64(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I64, &val, sizeof(i64)); }
            if (dtype == HDF::U64) { u64 val = hdf->getU64(metaPath, 0); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U64, &val, sizeof(u64)); }
        }else{
            if (dtype == HDF::BYTE) { byte* buff = new byte[count]; hdf->getByteBuff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_BYTE, buff, sizeof(byte)*count); delete[] buff; }
            if (dtype == HDF::DOUBLE) { double* buff = new double[count]; hdf->getDoubleBuff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_DOUBLE, buff, sizeof(double)*count); delete[] buff; }
            if (dtype == HDF::I16) { i16* buff = new i16[count]; hdf->getI16Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I16, buff, sizeof(i16)*count); delete[] buff; }
            if (dtype == HDF::U16) { u16* buff = new u16[count]; hdf->getU16Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U16, buff, sizeof(u16)*count); delete[] buff; }
            if (dtype == HDF::I32) { i32* buff = new i32[count]; hdf->getI32Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I32, buff, sizeof(i32)*count); delete[] buff; }
            if (dtype == HDF::U32) { u32* buff = new u32[count]; hdf->getU32Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U32, buff, sizeof(u32)*count); delete[] buff; }
            if (dtype == HDF::I64) { i64* buff = new i64[count]; hdf->getI64Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I64, buff, sizeof(i64)*count); delete[] buff; }
            if (dtype == HDF::U64) { u64* buff = new u64[count]; hdf->getU64Buff(metaPath, buff, count); metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U64, buff, sizeof(u64)*count); delete[] buff; }
            if (dtype == HDF::STRING) {
                std::string val = hdf->getString(metaPath, "");
                metaData[metaNames[i]] = new MetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_CHAR, (char*)val.c_str(), val.length());
            }
        }
    }
}
#endif

int MpxFrame::loadFromFile(const char* filePath, size_t frameIndex)
{
    clear();
    if (str::endsWith(filePath, ".h5")){
#ifndef NO_HDF5
    HDF hdf;
    int rc = hdf.open(filePath);
    if (rc)
        return PXERR_FILE_OPEN;

    std::string framePath = str::format("/Frame_%u", frameIndex);
    if (!hdf.exists(framePath)){
        hdf.close();
        return PXERR_FILE_BADDATA;
    }

    HDF::Type frType = hdf.type(framePath + "/Data");
    u32 width = hdf.getU32(framePath + "/Width", 0);
    u32 height = hdf.getU32(framePath + "/Width", 0);
    if (width == 0 || height == 0){
        hdf.close();
        return PXERR_FILE_BADDATA;
    }

    if (frType == HDF::I16){
        init(width, height, FRMT_I16);
        hdf.getI16Buff(framePath + "/Data", (i16*)mData, mSize);
    }
    if (frType == HDF::U32 || frType == HDF::I32){
        init(width, height, FRMT_U32);
        hdf.getU32Buff(framePath + "/Data", (u32*)mData, mSize);
    }
    if (frType == HDF::DOUBLE){
        init(width, height, FRMT_DOUBLE);
        hdf.getDoubleBuff(framePath + "/Data", (double*)mData, mSize);
    }
    if (frType == HDF::U64){
        init(width, height, FRMT_U64);
        hdf.getU64Buff(framePath + "/Data", (u64*)mData, mSize);
    }

    loadMetaDataFromHDF(&hdf, mMetaData, framePath);
    hdf.close();
    return 0;
#else
        return PXERR_UNSUPPORTED;
#endif
    }

    Buffer<byte> buff;
    DataType dataType;
    u32 width = 0;
    u32 height = 0;
    int rc = MpxFrameFile::load(filePath, (u32)frameIndex, buff, dataType, width, height, mMetaData);
    if (rc)
        return rc;

    mDataType = dataType;
    switch(dataType){
        case DT_I16: init(width, height, FRMT_I16); break;
        case DT_U32: init(width, height, FRMT_U32); break;
        case DT_U64: init(width, height, FRMT_U64); break;
        case DT_DOUBLE: init(width, height, FRMT_DOUBLE); break;
        default: return PXERR_INVALID_DATA_TYPE;
    }

    memcpy(mData, buff.data(), buff.size());
    return rc;
}

void MpxFrame::clear()
{
    mWidth = 0;
    mHeight = 0;
    if (mData)
        delete[] mData;
    mData = NULL;
    mSize = 0;
    for (std::map<std::string, MetaData*>::iterator it = mMetaData.begin(); it != mMetaData.end(); ++it){
        delete it->second;
    }
    mMetaData.clear();
}

double MpxFrame::acqTime()
{
    if (mMetaData.find("Acq time") == mMetaData.end())
        return 0;
    return *(double*)(mMetaData["Acq time"]->data());
}

double MpxFrame::startTime()
{
    if (mMetaData.find("Start time") == mMetaData.end())
        return 0;
    return *(double*)(mMetaData["Start time"]->data());
}

double MpxFrame::bias()
{
    if (mMetaData.find("HV") == mMetaData.end())
        return 0;
    return *(double*)(mMetaData["HV"]->data());
}
