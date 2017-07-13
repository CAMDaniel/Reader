/**
 * Copyright (C) 2015 Daniel Turecek
 *
 * @file      main.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2015-03-11
 *
 */
#include "mpxframe.h"

void loadFrame(const char* fileName, int index)
{
    MpxFrame f;
    int rc = f.loadFromFile(fileName, index);
    printf("Load: %d\n", rc);
    printf("FrameType: %d\n", f.frameType());
    unsigned* data = f.u32Data();

    double sum = 0;
    for (size_t i = 0; i < f.size(); i++) {
        sum += data[i];
    }
    printf("Sum: %f\n", sum);

    //std::map<std::string, MetaData*>& mdata = f.metaData();
    //for (std::map<std::string, MetaData*>::const_iterator it = mdata.begin(); it != mdata.end(); ++it){
        //MetaData* m = it->second;
        //printf("%s\n", it->first.c_str());
    //}

    printf("AcqTime: %f\n", f.acqTime());
    printf("StartTime: %f\n", f.startTime());
    printf("HV: %f\n", f.bias());
}


int main (int argc, char const* argv[])
{
    int rc = 0;
    MpxFrame f;
    const char* filePath = "/x/data/f.h5";
    size_t count = f.frameCount(filePath);
    printf("Count: %lu\n", count);

    loadFrame(filePath, 0);
    printf("--------\n");
    loadFrame(filePath, 1);

    return 0;
}


