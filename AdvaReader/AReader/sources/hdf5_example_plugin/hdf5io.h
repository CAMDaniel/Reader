/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      hdf5io.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-06-06
 *
 */
#ifndef HDF5IO_H
#define HDF5IO_H
#include "../common.h"
//#include "ipixet.h"
//#include "ipxplugin.h"
//#include "hdf.h"
#include "osdepend.h"

namespace PluginHDF5IO {

#define PX_HDF5FLAG_COMPRESS 1

class HDF5FileType : public px::IFileType
{
public:
    HDF5FileType();
    virtual ~HDF5FileType();
    void setIPixet(px::IPixet* pixet);

public:
    // IFileType interface
    virtual bool FCALL isDataSupported(px::IData* data) const;
    virtual bool FCALL isDataFormatSupported(u32 dataFormat) const;
    virtual bool FCALL isMultiDataInSingleFileSupported() const { return true; }
    virtual u32 FCALL dataCountInFile(const char* fileName) const;
    virtual u32 FCALL frameCountInFile(const char* fileName) const;
    virtual int FCALL saveData(px::IData* data, const char* fileName, u32 flags);
    virtual px::IData* FCALL loadData(const char* fileName, u32 index);
    virtual u32 FCALL id() const { return mID; }
    virtual const char* FCALL name() const { return "HDF5 File"; }
    virtual const char* FCALL extension() const { return "h5"; }
    virtual px::IParamMgr* FCALL parameters() const { return mPars; }
    virtual void FCALL setID(u32 id) { mID = id; }
    virtual int FCALL destroy() { return 0; }

protected:
    void addMetaDataToHDF(HDF* hdf, px::IMpxFrame* frame, std::string framePath);
    void loadMetaDataFromHDF(HDF* hdf, px::IMpxFrame* frame, std::string framePath);
    void createParams();
    void getPrefix(const std::string fileNameWithPrefix, std::string& filePath, std::string& prefix) const;

private:
    static ThreadSyncObject mSync;
    px::IPixet* mPixet;
    px::IParamMgr* mPars;
    u32 mID;
};


class HDF5IO : public px::IPxPlugin
{
public:
    const static char* PLUGIN_NAME;
    const static char* PLUGIN_DESC;

public:
    HDF5IO();
    virtual ~HDF5IO();

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
    HDF5FileType mHdf5;
};


} // namespace


#endif /* end of include guard: HDF5IO_H */

