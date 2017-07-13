/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      hdf.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-03-16
 *
 */
#include "hdf.h"
#include <algorithm>
#include "hdf5.h"
#include "hdf5_hl.h"
#include "osdepend.h"
#include "strutils.h"

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
    if (fileExists(filePath) && !overwrite)
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
    std::vector<std::string> items = str::split(path, "/");
    std::string newpath;
    for (size_t i = 1; i < items.size(); i++){
        newpath += "/" + items[i];
        if (H5Lexists(mFile, newpath.c_str(), H5P_DEFAULT) == 0)
            return false;
    }
    return true;
    //return H5Lexists(mFile, path.c_str(), H5P_DEFAULT) != 0;
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

bool HDF::isGroup(std::string path) const
{
    H5O_info_t infobuf;
    H5Oget_info_by_name (mFile, path.c_str(), &infobuf, H5P_DEFAULT);
    return infobuf.type == H5O_TYPE_GROUP;
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
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_FLOAT, &value);
}

int HDF::setDouble(std::string path, double value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_DOUBLE, &value);
}

int HDF::setChar(std::string path, char value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_CHAR, &value);
}

int HDF::setByte(std::string path, byte value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UCHAR, &value);
}

int HDF::setI16(std::string path, i16 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_INT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT16, &value);
}

int HDF::setU16(std::string path, u16 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_UINT16, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT16, &value);
}

int HDF::setI32(std::string path, i32 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT32, &value);
}

int HDF::setU32(std::string path, u32 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT32, &value);
}

int HDF::setI64(std::string path,  i64 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_INT64, &value);
}

int HDF::setU64(std::string path, u64 value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UINT64, &value);
}

int HDF::setBool(std::string path, bool value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    byte bvalue = value ? 1 : 0;
    if (exists(path)){
        hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
        int rc = H5Dwrite(dset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, &bvalue);
        H5Dclose(dset);
        return rc;
    }
    hsize_t dims[1] = {1};
    return H5LTmake_dataset(mFile, path.c_str(), 1, dims, H5T_NATIVE_UCHAR, &bvalue);
}

int HDF::setString(std::string path, std::string value)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    if (exists(path))
        remove(path);
    return H5LTmake_dataset_string(mFile, path.c_str(), value.c_str());
}


int HDF::setCompressDataSet(std::string path, const void* buff, size_t size, int type)
{
    if (createPathDir(path))
        return ERR_CANNOT_CREATE_PATH;
    hsize_t dims[1] = {size};
    hsize_t chunk[1] = {std::min((size_t)CHUNK_SIZE, size)};

    if (mCompressLevel > 0){
        if (exists(path))
            remove(path);
        hid_t space = H5Screate_simple(1, dims, NULL);
        hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
        herr_t status = H5Pset_deflate(dcpl, mCompressLevel);
        status = H5Pset_chunk(dcpl, 1, chunk);
        hid_t dset = H5Dcreate(mFile, path.c_str(), type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
        status += H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff);
        status += H5Pclose(dcpl);
        status += H5Dclose(dset);
        status += H5Sclose(space);
        return status;
    } else{
        if (exists(path) && length(path) == (int)size && type == this->type(path)){
            hid_t dset = H5Dopen(mFile, path.c_str(), H5P_DEFAULT);
            int rc = H5Dwrite(dset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, buff);
            H5Dclose(dset);
            return rc;
        }
        return H5LTmake_dataset(mFile, path.c_str(), 1, dims, type, buff);
    }
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
    hsize_t dims[2] = {0, 0};
    size_t size = 0;
    H5LTget_dataset_info(mFile, path.c_str(), dims, NULL, &size);

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
