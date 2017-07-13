/**
 * Copyright (C) 2017 Daniel Turecek
 *
 * @file      metadata.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2017-01-10
 *
 */
#include "metadata.h"
#include <cstring>


MetaData::MetaData(const char *name, const char *desc, DataType type,
            const void *data, size_t size,  bool preallocated)
        : mName(name),  mDesc(desc) , mType(type) , mSize(size),  mData(0)
{
    if (preallocated)
        mData = (void *)data;
    else {
        mData = malloc(mSize);
        if (mData && data)
            memcpy(mData, data, mSize);
    }
}

MetaData::MetaData(const MetaData &item)
    : mName(item.mName) , mDesc(item.mDesc) , mType(item.mType) , mSize(item.mSize)
{
    mData = malloc(mSize);
    if (mData)
        memcpy(mData, item.mData, mSize);
}

MetaData & MetaData::operator=(const MetaData &item) 
{
    mName = item.mName;
    mDesc = item.mDesc;
    mType = item.mType;
    mSize = item.mSize;
    if (mData)
        free(mData);
    mData = malloc(mSize);
    if (mData)
        memcpy(mData, item.mData, mSize);
    return *this;
}

MetaData::~MetaData() {
    free(mData);
}

