/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      hdf.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-03-16
 *
 */
#ifndef HDF_H
#define HDF_H
#include <string>
#include <vector>
#include "common.h"

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
    virtual bool isGroup(std::string path) const;
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



#endif /* !HDF_H */

