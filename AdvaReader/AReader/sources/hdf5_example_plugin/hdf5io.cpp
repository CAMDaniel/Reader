/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      hdf5io.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-06-06
 *
 */
#include "hdf5io.h"
#include "ipixet.h"
#include "ipxplugin.h"
#include "hdf.h"
#include "strutils.h"
#include "pxerrors.h"
#include "osdepend.h"
#include "datautils.h"

#define PAR_COMPRESS_LEVEL "CompressLevel"

using namespace px;

namespace PluginHDF5IO {

const char* HDF5IO::PLUGIN_NAME = "hdf5io";
const char* HDF5IO::PLUGIN_DESC = "Plugin that adds HDF5 file support";
ThreadSyncObject HDF5FileType::mSync;


HDF5IO::HDF5IO()
    : mPixet(nullptr)
{
}

HDF5IO::~HDF5IO()
{
}

int HDF5IO::initialize(IPixet* pixet)
{
    mPixet = pixet;
    pixet->dataMgr()->registerFileType("h5", &mHdf5);
    mHdf5.setIPixet(pixet);
    return 0;
}

void HDF5IO::onStart()
{

}

void HDF5IO::onClose()
{
    mPixet->dataMgr()->unregisterFileType("h5", &mHdf5);
}



HDF5FileType::HDF5FileType()
    : mPixet(nullptr)
    , mPars(NULL)
    , mID(0)
{

}

HDF5FileType::~HDF5FileType()
{
    mPars->destroy();
    mPars = NULL;
}

void HDF5FileType::setIPixet(IPixet* pixet)
{
    mPixet = pixet;
    createParams();
}


bool HDF5FileType::isDataSupported(px::IData* data) const
{
    if (data->dataFormat() == PX_DATAFORMAT_FRAME)
        return true;
    return false;
}

bool HDF5FileType::isDataFormatSupported(u32 dataFormat) const
{
    if (dataFormat == PX_DATAFORMAT_FRAME)
        return true;
    return false;
}

u32 HDF5FileType::dataCountInFile(const char* fileName) const
{
    ThreadLock lock(&mSync);
    std::string filePath, prefix;
    getPrefix(fileName, filePath, prefix);
    HDF hdf;
    int rc = hdf.open(filePath.c_str());
    if (rc)
        return 0;
    if (!prefix.empty() && !hdf.exists(prefix))
        return 0;

    std::vector<std::string> subItems = hdf.subItems(prefix + "/");
    u32 frameCount = 0;
    for (size_t i = 0; i < subItems.size(); i++){
        if (str::startsWith(subItems[i], "Frame"))
            frameCount++;
    }
    return frameCount;
}

u32 HDF5FileType::frameCountInFile(const char* fileName) const
{
    return dataCountInFile(fileName);
}

int HDF5FileType::saveData(IData* data, const char* fileName, u32 flags)
{
    ThreadLock lock(&mSync);
    if (data->dataFormat() != PX_DATAFORMAT_FRAME){
        mPixet->logMsg(HDF5IO::PLUGIN_NAME, 0, "Invalid data format");
        return PXERR_INVALID_ARGUMENT;
    }

    std::string prefix;
    std::string filePath;
    getPrefix(fileName, filePath, prefix);

    byte compressLevel = mPars ? mPars->get(PAR_COMPRESS_LEVEL)->getByte() : 0;
    u32 frameCount = frameCountInFile(fileName);
    std::string framePath = str::format("%s/Frame_%d", prefix.c_str(), frameCount);

    HDF hdf;
    int rc = hdf.open(filePath.c_str(), !fileExists(filePath.c_str()), compressLevel);
    if (rc){
        mPixet->logMsg(HDF5IO::PLUGIN_NAME, 0, "Cannot open file %s", filePath.c_str());
        return PXERR_FILE_OPEN;
    }

    IMpxFrame* frame = reinterpret_cast<IMpxFrame*>(data);
    hdf.setU32(framePath + "/Width", frame->width());
    hdf.setU32(framePath + "/Height", frame->height());
    hdf.setDouble(framePath + "/AcqTime", frame->acqTime());
    hdf.setDouble(framePath + "/StartTime", frame->startTime());

    if (frame->frameType() == PX_FRAMETYPE_I16)
        hdf.setI16Buff(framePath + "/Data", frame->dataI16(), frame->size());
    if (frame->frameType() == PX_FRAMETYPE_U32)
        hdf.setU32Buff(framePath + "/Data", frame->dataU32(), frame->size());
    if (frame->frameType() == PX_FRAMETYPE_DOUBLE)
        hdf.setDoubleBuff(framePath + "/Data", frame->dataDouble(), frame->size());
    if (frame->frameType() == PX_FRAMETYPE_U64)
        hdf.setU64Buff(framePath + "/Data", frame->dataU64(), frame->size());

    addMetaDataToHDF(&hdf, frame, framePath);
    return 0;
}

px::IData* HDF5FileType::loadData(const char* fileName, u32 index)
{
    ThreadLock lock(&mSync);
    std::string prefix;
    std::string filePath;
    getPrefix(fileName, filePath, prefix);

    HDF hdf;
    int rc = hdf.open(filePath.c_str());
    if (rc){
        mPixet->logMsg(HDF5IO::PLUGIN_NAME, 0, "Cannot open file %s", filePath.c_str());
        return NULL;
    }

    std::string framePath = str::format("%s/Frame_%u", prefix.c_str(), index);
    if (!hdf.exists(framePath)){
        mPixet->logMsg(HDF5IO::PLUGIN_NAME, 0, "Cannot open frame %u from file %s", index, fileName);
        return NULL;
    }

    HDF::Type frType = hdf.type(framePath + "/Data");
    u32 width = hdf.getU32(framePath + "/Width", 0);
    u32 height = hdf.getU32(framePath + "/Height", 0);
    if (width == 0 || height == 0){
        mPixet->logMsg(HDF5IO::PLUGIN_NAME, 0, "Invalid data for frame %u in file %s", index, fileName);
        return NULL;
    }

    IMpxFrame* frame = NULL;
    if (frType == HDF::I16){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_I16);
        hdf.getI16Buff(framePath + "/Data", frame->dataI16(), frame->size());
    }
    if (frType == HDF::I32){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_U32);
        hdf.getI32Buff(framePath + "/Data", (i32*)frame->dataU32(), frame->size());
    }
    if (frType == HDF::U32){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_U32);
        hdf.getU32Buff(framePath + "/Data", frame->dataU32(), frame->size());
    }
    if (frType == HDF::DOUBLE){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_DOUBLE);
        hdf.getDoubleBuff(framePath + "/Data", frame->dataDouble(), frame->size());
    }
    if (frType == HDF::U64){
        frame = mPixet->dataMgr()->createFrame(width, height, PX_FRAMETYPE_U64);
        hdf.getU64Buff(framePath + "/Data", frame->dataU64(), frame->size());
    }

    loadMetaDataFromHDF(&hdf, frame, framePath);
    return frame;
}

void HDF5FileType::addMetaDataToHDF(HDF* hdf, IMpxFrame* frame, std::string framePath)
{
    StrList metas;
    frame->metaDataNames(&metas);
    for (u32 i = 0; i < metas.size(); i++){
        IMetaData* meta = frame->metaData(metas.get(i));
        std::string path = framePath + "/MetaData/" + meta->name();
        size_t count = meta->size() / sizeofType[meta->type()];
        if (count == 1) {
            if (meta->type() == DT_BYTE) hdf->setByte(path, (*(byte*)meta->data()));
            if (meta->type() == DT_BOOL) hdf->setBool(path, (*(byte*)meta->data()) != 0);
            if (meta->type() == DT_CHAR) hdf->setChar(path, (*(char*)meta->data()));
            if (meta->type() == DT_DOUBLE) hdf->setDouble(path, (*(double*)meta->data()));
            if (meta->type() == DT_FLOAT) hdf->setFloat(path, (*(float*)meta->data()));
            if (meta->type() == DT_I16) hdf->setI16(path, (*(i16*)meta->data()));
            if (meta->type() == DT_U16) hdf->setU16(path, (*(u16*)meta->data()));
            if (meta->type() == DT_I32) hdf->setI32(path, (*(i32*)meta->data()));
            if (meta->type() == DT_U32) hdf->setU32(path, (*(u32*)meta->data()));
            if (meta->type() == DT_I64) hdf->setI64(path, (*(i64*)meta->data()));
            if (meta->type() == DT_U64) hdf->setU64(path, (*(u64*)meta->data()));
        }else {
            if (meta->type() == DT_BYTE) hdf->setByteBuff(path, (byte*)meta->data(), count);
            if (meta->type() == DT_DOUBLE) hdf->setDoubleBuff(path, (double*)meta->data(), count);
            if (meta->type() == DT_I16) hdf->setI16Buff(path, (i16*)meta->data(), count);
            if (meta->type() == DT_U16) hdf->setU16Buff(path, (u16*)meta->data(), count);
            if (meta->type() == DT_I32) hdf->setI32Buff(path, (i32*)meta->data(), count);
            if (meta->type() == DT_U32) hdf->setU32Buff(path, (u32*)meta->data(), count);
            if (meta->type() == DT_I64) hdf->setI64Buff(path, (i64*)meta->data(), count);
            if (meta->type() == DT_U64) hdf->setU64Buff(path, (u64*)meta->data(), count);
            if (meta->type() == DT_CHAR){
                std::string value = std::string((char*)meta->data(), meta->size());
                hdf->setString(path, value);
            }
        }
    }
}


void HDF5FileType::loadMetaDataFromHDF(HDF* hdf, IMpxFrame* frame, std::string framePath)
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
            if (dtype == HDF::CHAR) { char val = hdf->getChar(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_CHAR, &val, 1); }
            if (dtype == HDF::BYTE) { byte val = hdf->getByte(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_BYTE, &val, 1); }
            if (dtype == HDF::FLOAT) { float val = hdf->getFloat(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_FLOAT, &val, sizeof(float)); }
            if (dtype == HDF::DOUBLE) { double val = hdf->getDouble(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_DOUBLE, &val, sizeof(double)); }
            if (dtype == HDF::I16) { i16 val = hdf->getI16(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I16, &val, sizeof(i16)); }
            if (dtype == HDF::U16) { u16 val = hdf->getU16(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U16, &val, sizeof(u16)); }
            if (dtype == HDF::I32) { i32 val = hdf->getI32(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I32, &val, sizeof(i32)); }
            if (dtype == HDF::U32) { u32 val = hdf->getU32(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U32, &val, sizeof(u32)); }
            if (dtype == HDF::I64) { i64 val = hdf->getI64(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I64, &val, sizeof(i64)); }
            if (dtype == HDF::U64) { u64 val = hdf->getU64(metaPath, 0); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U64, &val, sizeof(u64)); }
        }else{
            if (dtype == HDF::BYTE) { byte* buff = new byte[count]; hdf->getByteBuff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_BYTE, buff, sizeof(byte)*count); delete[] buff; }
            if (dtype == HDF::DOUBLE) { double* buff = new double[count]; hdf->getDoubleBuff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_DOUBLE, buff, sizeof(double)*count); delete[] buff; }
            if (dtype == HDF::I16) { i16* buff = new i16[count]; hdf->getI16Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I16, buff, sizeof(i16)*count); delete[] buff; }
            if (dtype == HDF::U16) { u16* buff = new u16[count]; hdf->getU16Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U16, buff, sizeof(u16)*count); delete[] buff; }
            if (dtype == HDF::I32) { i32* buff = new i32[count]; hdf->getI32Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I32, buff, sizeof(i32)*count); delete[] buff; }
            if (dtype == HDF::U32) { u32* buff = new u32[count]; hdf->getU32Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U32, buff, sizeof(u32)*count); delete[] buff; }
            if (dtype == HDF::I64) { i64* buff = new i64[count]; hdf->getI64Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_I64, buff, sizeof(i64)*count); delete[] buff; }
            if (dtype == HDF::U64) { u64* buff = new u64[count]; hdf->getU64Buff(metaPath, buff, count); frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_U64, buff, sizeof(u64)*count); delete[] buff; }
            if (dtype == HDF::STRING) {
                std::string val = hdf->getString(metaPath, "");
                frame->addMetaData(metaNames[i].c_str(), metaNames[i].c_str(), DT_CHAR, (char*)val.c_str(), val.length());
            }
        }
    }
}

void HDF5FileType::createParams()
{
    mPars = mPixet->createParamMgr();
    mPars->createByte(PAR_COMPRESS_LEVEL, "Compress Data Level (0 - no compression, 9-best)", 2, NULL, 0, PX_PAR_SAVETOSTG);
}

void HDF5FileType::getPrefix(const std::string fileNameWithPrefix, std::string& filePath, std::string& prefix) const
{
    prefix = "";
    filePath = fileNameWithPrefix;
    size_t pos = fileNameWithPrefix.find(":", 2);

    if (pos != std::string::npos){
        filePath = fileNameWithPrefix.substr(0, pos);
        prefix = "/" + fileNameWithPrefix.substr(pos+1);
    }
}

HDF5IO hdf5io;

}


EXPORTFUNC IPxPlugin* pxGetPlugin()
{
    return &PluginHDF5IO::hdf5io;
}

