/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      tiffio.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-10-26
 *
 */
#include "tiffio.h"
#include "ipixet.h"
#include "ipxplugin.h"
#include "strutils.h"
#include "pxerrors.h"
#include "osdepend.h"
#include "datautils.h"
#include "tinytiffreader.h"
#include "tinytiffwriter.h"

#define PAR_DOUBLE_FACTOR "DoubleConvFactor"

using namespace px;

namespace PluginTIFFIO {

const char* TIFFIO::PLUGIN_NAME = "tiffio";
const char* TIFFIO::PLUGIN_DESC = "Plugin that adds TIFF file support";

ThreadSyncObject TIFFFileType::mSync;


TIFFIO::TIFFIO()
    : mPixet(nullptr)
    , mTiff1("tif")
    , mTiff2("tiff")
{
}

TIFFIO::~TIFFIO()
{
}

int TIFFIO::initialize(IPixet* pixet)
{
    mPixet = pixet;
    pixet->dataMgr()->registerFileType("tif", &mTiff1);
    pixet->dataMgr()->registerFileType("tiff", &mTiff2);
    mTiff1.setIPixet(pixet);
    mTiff2.setIPixet(pixet);
    return 0;
}

void TIFFIO::onStart()
{

}

void TIFFIO::onClose()
{
    mPixet->dataMgr()->unregisterFileType("tif", &mTiff1);
    mPixet->dataMgr()->unregisterFileType("tiff", &mTiff2);
}



TIFFFileType::TIFFFileType(const char* ext)
    : mPixet(nullptr)
    , mPars(NULL)
    , mExt(ext)
    , mID(0)
{

}

TIFFFileType::~TIFFFileType()
{
    mPars->destroy();
    mPars = NULL;
}

void TIFFFileType::setIPixet(IPixet* pixet)
{
    mPixet = pixet;
    createParams();
}


bool TIFFFileType::isDataSupported(px::IData* data) const
{
    if (data->dataFormat() == PX_DATAFORMAT_FRAME)
        return true;
    return false;
}

bool TIFFFileType::isDataFormatSupported(u32 dataFormat) const
{
    if (dataFormat == PX_DATAFORMAT_FRAME)
        return true;
    return false;
}

u32 TIFFFileType::dataCountInFile(const char* fileName) const
{
    return 1;
}

u32 TIFFFileType::frameCountInFile(const char* fileName) const
{
    return dataCountInFile(fileName);
}

int TIFFFileType::saveData(IData* data, const char* fileName, u32 flags)
{
    ThreadLock lock(&mSync);
    if (data->dataFormat() != PX_DATAFORMAT_FRAME){
        mPixet->logMsg(TIFFIO::PLUGIN_NAME, 0, "Invalid data format");
        return PXERR_INVALID_ARGUMENT;
    }

    IMpxFrame* frame = reinterpret_cast<IMpxFrame*>(data);
    uint16_t bitSize = 8;
    if (frame->frameType() == PX_FRAMETYPE_I16) bitSize = 16;
    if (frame->frameType() == PX_FRAMETYPE_U32) bitSize = 32;
    if (frame->frameType() == PX_FRAMETYPE_U64) bitSize = 32;
    if (frame->frameType() == PX_FRAMETYPE_DOUBLE) bitSize = 32;

    TinyTIFFFile* tif = TinyTIFFWriter_open(fileName, bitSize, (uint32_t)frame->width(), (uint32_t)frame->height());
    if (!tif){
        mPixet->logMsg(TIFFIO::PLUGIN_NAME, 0, "Cannot open file %s", fileName);
        return PXERR_FILE_OPEN;
    }

    if (frame->frameType() == PX_FRAMETYPE_I16){
        TinyTIFFWriter_writeImage(tif, frame->dataI16());
    }

    if (frame->frameType() == PX_FRAMETYPE_U32){
        TinyTIFFWriter_writeImage(tif, frame->dataU32());
    }

    if (frame->frameType() == PX_FRAMETYPE_U64){
        u32* buff = new u32[frame->size()];
        u64* data = frame->dataU64();
        for (size_t i = 0; i < frame->size(); i++)
            buff[i] = (u32)data[i];
        TinyTIFFWriter_writeImage(tif, buff);
        delete[] buff;
    }

    if (frame->frameType() == PX_FRAMETYPE_DOUBLE){
        u32* buff = new u32[frame->size()];
        double* data = frame->dataDouble();
        double fact = mPars->get(PAR_DOUBLE_FACTOR)->getU32();
        for (size_t i = 0; i < frame->size(); i++)
            buff[i] = (u32)(data[i] * fact + 0.5);
        TinyTIFFWriter_writeImage(tif, buff);
        delete[] buff;
    }

    TinyTIFFWriter_close(tif);
    return 0;
}

px::IData* TIFFFileType::loadData(const char* fileName, u32 index)
{
    ThreadLock lock(&mSync);
    IMpxFrame* frame = NULL;


    TinyTIFFReaderFile* tiffr = NULL;
    tiffr = TinyTIFFReader_open(fileName);
    if (!tiffr) {
        mPixet->logMsg(TIFFIO::PLUGIN_NAME, 0, "Cannot open file %s", fileName);
        return NULL;
    }

    uint32_t width = TinyTIFFReader_getWidth(tiffr);
    uint32_t height = TinyTIFFReader_getHeight(tiffr);
    uint16_t bits = TinyTIFFReader_getBitsPerSample(tiffr, 0);

    if (bits == 16){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_I16);
        TinyTIFFReader_getSampleData(tiffr, frame->dataI16(), 0);
    }

    if (bits == 32){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_U32);
        TinyTIFFReader_getSampleData(tiffr, frame->dataU32(), 0);
    }

    TinyTIFFReader_close(tiffr);
    return frame;
}


void TIFFFileType::createParams()
{
    mPars = mPixet->createParamMgr();
    mPars->createU32(PAR_DOUBLE_FACTOR, "Conversion factor from double to 32bit tiff", 10000, NULL, 0, PX_PAR_SAVETOSTG);
}

TIFFIO tiffio;

}


EXPORTFUNC IPxPlugin* pxGetPlugin()
{
    return &PluginTIFFIO::tiffio;
}

