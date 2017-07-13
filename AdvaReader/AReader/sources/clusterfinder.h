/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      clusterfinder.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-07-31
 *
 */
#ifndef CLUSTERFINDER_H
#define CLUSTERFINDER_H
#include <vector>
#include <map>
#include <cmath>

#define UNTESTED       -1
#define INNER_PIX_MASK 85 // (0x1 | 0x4 | 0x10 | 0x40)

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

struct Pixel
{
    enum PixType { INNER = 0, BORDER };
    enum Direction {LEFT=0, UP_LEFT, UP, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT};
    unsigned x;
    unsigned y;
    double val;
    unsigned char neighborMask;
    PixType type;
    //std::map<unsigned char, int> neighbors;

    Pixel(unsigned _x , unsigned _y, double _val)
        : x(_x), y(_y), val(_val), neighborMask(0), type(INNER)
    {
    }
    /*void addNeighbor(int pixelIndex, Direction direction, bool generateNeighbourMap = false)
    {
        neighborMask |= (1 << static_cast<int>(direction));
        if (generateNeighbourMap)
            neighbors[static_cast<unsigned char>(direction)] = pixelIndex;
    }*/
};


class Cluster
{
public:
    Cluster()
    {
        clearParameters();
    }

    virtual ~Cluster()
    {
        clear();
    }

    void clear() {
        pixels.clear();
        clearParameters();
    }

    void clearParameters() {
        volume = 0;
        minHeight = 0;
        maxHeight = 0;
        innerPixCount = 0;
        borderPixCount = 0;
        centroidX = 0;
        centroidY = 0;
        volCentroidX = 0;
        volCentroidY = 0;
        size = 0;
        minX = 0;
        maxX = 0;
        minY = 0;
        maxY = 0;
    }

    int addPixel(Pixel pixel)
    {
        pixels.push_back(pixel);
        return (int)(pixels.size() - 1); // index of cluster
    }

    virtual void analyze()
    {
        size = pixels.size();
        minHeight = 1e100;
        maxHeight = -1e100;
        minX = 0xFFFFFFFF;
        minY = 0xFFFFFFFF;
        maxX = 0;
        maxY = 0;

        for (size_t pixidx = 0; pixidx < pixels.size(); pixidx++) {
            Pixel& pixel = pixels[pixidx];
            double val = pixel.val;
            volume += val;
            centroidX += pixel.x;
            centroidY += pixel.y;
            volCentroidX += (double)(pixel.x) * val;
            volCentroidY += (double)(pixel.y) * val;

            if (val > maxHeight) maxHeight = val;
            if (val < minHeight) minHeight = val;
            if (pixel.x > maxX) maxX = pixel.x;
            if (pixel.y > maxY) maxY = pixel.y;
            if (pixel.x < minX) minX = pixel.x;
            if (pixel.y < minY) minY = pixel.y;

            if ((pixel.neighborMask & INNER_PIX_MASK) == INNER_PIX_MASK){
                innerPixCount++;
                pixel.type = Pixel::INNER;
            }else{
                borderPixCount++;
                pixel.type = Pixel::BORDER;
            }
        }

        centroidX /= (double)size;
        centroidY /= (double)size;
        volCentroidX /= volume;
        volCentroidY /= volume;
    }

public:
    virtual double roundness() const  {return 2 * sqrt(M_PI*size) / borderPixCount;}
    virtual double diameter() const   {return 2 * sqrt((double)size / M_PI);}

public:
    std::vector<Pixel> pixels;
    double volume;
    double maxHeight;
    double minHeight;
    double centroidX;
    double centroidY;
    double volCentroidX;
    double volCentroidY;
    size_t size;
    size_t borderPixCount;
    size_t innerPixCount;
    size_t minX;
    size_t minY;
    size_t maxX;
    size_t maxY;
};

class IClusterFinder
{
public:
    virtual void clear() = 0;
    virtual std::vector<Cluster*>* clusterList() = 0;
    virtual void destroy() = 0;
};


template <class T> class ClusterFinder : public IClusterFinder
{
public:

    ClusterFinder()
        : mMask(NULL)
    {

    }

    virtual ~ClusterFinder()
    {
        clear();
        if (mMask)
            delete[] mMask;
    }

    void searchFrame(T* data, unsigned width, unsigned height){
        // 8-way direction indexes
        static int dirx[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
        static int diry[8] = {0, 1, 1, 1, 0, -1, -1, -1};

        if (!mMask)
            mMask = new int[width*height]; // map of found pixels
        for (size_t i = 0; i < width*height; i++)
            mMask[i] = UNTESTED;

        for (unsigned pixIndex = 0; pixIndex < width*height; pixIndex++) 
        {
            if (data[pixIndex] == 0 || mMask[pixIndex] != UNTESTED)
                continue;

            // new pixel (not part of any cluster) found
            unsigned x = pixIndex % width;
            unsigned y = pixIndex / width;

            // create new cluster and add first pixel
            Cluster* cluster = new Cluster();
            std::vector<Pixel>& pixels = cluster->pixels;
            Pixel firstPixel = Pixel(x, y, static_cast<double>(data[pixIndex]));
            mMask[pixIndex] = cluster->addPixel(firstPixel);

            // go through all pixels in the cluster (pixels added as they are found)
            for (unsigned pix = 0; pix < pixels.size(); ++pix) {
                x = pixels[pix].x;
                y = pixels[pix].y;

                // find all neighbours 8-way search
                for (unsigned dir = 0; dir < 8; dir++) {
                    int dx = x + dirx[dir];
                    int dy = y + diry[dir];
                    unsigned didx = dy * width + dx;
                    if (dx < 0 || dy < 0 || dx >= (int)width || dy >= (int)height) // out of frame
                        continue;
                    if (data[didx] == 0)
                        continue;

                    int pixelIndex = 0;
                    if (mMask[didx] == UNTESTED){
                        pixelIndex = cluster->addPixel(Pixel(dx, dy, static_cast<double>(data[didx])));
                        mMask[didx] = pixelIndex;
                    }else
                       pixelIndex = mMask[didx];

                    //pixels[pix].addNeighbor(pixelIndex, static_cast<Pixel::Direction>(dir), generateNeighbourMap);
                    pixels[pix].neighborMask |= (1 << static_cast<int>(dir));
                }
            }
            clusters.push_back(cluster);
        }

        for (size_t i = 0; i < clusters.size(); i++)
            clusters[i]->analyze();
    }

    virtual void clear() {
        for (size_t i = 0; i < clusters.size(); i++)
            delete clusters[i];
        clusters.clear();
    }

    virtual std::vector<Cluster*>* clusterList(){
        return &clusters;
    }

    virtual void destroy() {
        delete this;
    }

public:
    std::vector<Cluster*> clusters;
    int* mMask;
};

#endif /* end of include guard: CLUSTERFINDER_H */

