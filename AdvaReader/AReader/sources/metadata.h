/**
 * Copyright (C) 2017 Daniel Turecek
 *
 * @file      metadata.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2017-01-10
 *
 */
#ifndef METADATA_H
#define METADATA_H
#include "common.h"
#include <string>

class MetaData
{
public:
    MetaData(const char *name, const char *desc, DataType type, const void *data, size_t size,  bool preallocated = false);
    MetaData(const MetaData &item);
    MetaData & operator=(const MetaData &item);
    virtual ~MetaData();
    virtual const char*  name() const { return mName.c_str(); }
    virtual const char* desc() const { return mDesc.c_str(); }
    virtual unsigned char type() const { return static_cast<unsigned char>(mType); }
    virtual size_t size() const { return mSize; }
    virtual void* data() const { return mData; }
public:
    std::string mName;
    std::string mDesc;
    DataType mType;
    size_t mSize;
    void* mData;
};


#endif /* !METADATA_H */

