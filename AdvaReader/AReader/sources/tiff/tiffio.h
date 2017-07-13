/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      tiffio.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-10-26
 *
 */
#ifndef TIFFIO_H
#define TIFFIO_H
#include "common.h"
#include "ipixet.h"
#include "ipxplugin.h"
#include "osdepend.h"

namespace PluginTIFFIO {

#define PX_TiffFLAG_COMPRESS 1

class TIFFFileType : public px::IFileType
{
public:
    TIFFFileType(const char* ext);
    virtual ~TIFFFileType();
    void setIPixet(px::IPixet* pixet);

public:
    // IFileType interface
    virtual bool FCALL isDataSupported(px::IData* data) const;
    virtual bool FCALL isDataFormatSupported(u32 dataFormat) const;
    virtual bool FCALL isMultiDataInSingleFileSupported() const { return false; }
    virtual u32 FCALL dataCountInFile(const char* fileName) const;
    virtual u32 FCALL frameCountInFile(const char* fileName) const;
    virtual int FCALL saveData(px::IData* data, const char* fileName, u32 flags);
    virtual px::IData* FCALL loadData(const char* fileName, u32 index);
    virtual u32 FCALL id() const { return mID; }
    virtual const char* FCALL name() const { return "TIFF File"; }
    virtual const char* FCALL extension() const { return mExt.c_str(); }
    virtual px::IParamMgr* FCALL parameters() const { return mPars; }
    virtual void FCALL setID(u32 id) { mID = id; }
    virtual int FCALL destroy() { return 0; }

protected:
    void createParams();

private:
    static ThreadSyncObject mSync;
    px::IPixet* mPixet;
    px::IParamMgr* mPars;
    std::string mExt;
    u32 mID;
};


class TIFFIO : public px::IPxPlugin
{
public:
    const static char* PLUGIN_NAME;
    const static char* PLUGIN_DESC;

public:
    TIFFIO();
    virtual ~TIFFIO();

public:
    // IPxPlugin interface
    virtual int FCALL initialize(px::IPixet* pixet);
    virtual void FCALL onStart();
    virtual void FCALL onClose();
    virtual const char* FCALL name() const { return PLUGIN_NAME;}
    virtual const char* FCALL description() const { return PLUGIN_DESC; }
    virtual px::IObject* FCALL privateInterface() { return NULL; }

protected:
    px::IPixet* mPixet;
    TIFFFileType mTiff1;
    TIFFFileType mTiff2;
};


} // namespace


#endif /* end of include guard: TIFFIO_H */
