/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      qmpxframe.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-06-11
 *
 */
#ifndef QMPXFRAME_H
#define QMPXFRAME_H
#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QGridLayout>


//####################################################################################################
//                               COLOR MAP
//####################################################################################################
class ColorMap
{
public:
    enum DefaultColorMap {CM_GRAY = 0, CM_JET, CM_JETWHITE, CM_HOT, CM_COOL, CM_HSL, CM_COOLWARM, CM_INVGRAY};
    ColorMap(unsigned* _colors = NULL, unsigned size = 0);
    ColorMap(DefaultColorMap cm);
    ~ColorMap();
    void setColorMap(DefaultColorMap cm);
    unsigned getCount()          { return colorCount;}
    unsigned char red(int idx)   { return (colors[idx] >> 16) & 0xFF; }
    unsigned char green(int idx) { return (colors[idx] >> 8) & 0xFF; }
    unsigned char blue(int idx)  { return colors[idx] & 0xFF; }
    int getUnderRangeColor()     { return underColor; }
    int getOverRangeColor()      { return overColor; }
    int getSpecialColor()        { return specialColor; }
    unsigned *getColors()        { return colors;}
    void setColors(unsigned* _colors, unsigned size) { memcpy(colors, _colors, size*sizeof(unsigned)); colorCount = size; }
    DefaultColorMap getDefaultColorMap() const { return defColorMap; }
public:
    void setGrayMap();
    void setJetMap();
    void setJetMapWhite();
    void setHotMap();
    void setCoolMap();
    void setHSLMap();
    void setCoolWarm();
    void setInvertedGrayMap();
private:
    unsigned  colorCount;
    unsigned* colors;
    unsigned underColor;
    unsigned overColor;
    unsigned specialColor;
    DefaultColorMap defColorMap;
};


//####################################################################################################
//                               QMPX AXIS
//####################################################################################################

class QMpxAxis : public QWidget
{
    Q_OBJECT
public:
    enum AxisType {AxisX = 0, AxisY = 1};
    QMpxAxis(QWidget *parent, AxisType axisType);
    virtual~QMpxAxis() {}
    void setRange(double minRange, double maxRange);
    void setName(const char* name) { mName.setText(name);}
    void paint(QPainter* painter, unsigned x, unsigned y, unsigned width, unsigned height);
    QSize getAxisSize();
private:
    AxisType mAxisType;
    QLabel mMax;
    QLabel mMin;
    QLabel mName;
};


//####################################################################################################
//                               QMPX COLOR BAR
//####################################################################################################

class QMpxColorBar : public QWidget
{
    Q_OBJECT
public:
    QMpxColorBar(QWidget *parent = 0);
    virtual ~QMpxColorBar() {}
public:
    void paintEvent(QPaintEvent *event);
    void paint(QPainter* painter, unsigned x, unsigned y, unsigned width);
    void setColorMap(ColorMap* map);
    void setLogScale(bool logScale) { mLogScale = logScale; repaint(); }
    void setRange(double minRange, double maxRange) { mMinRange = minRange; mMaxRange = maxRange; update();}
    QSize sizeHint() const { return QSize(100, 35); }
    unsigned getColorbarHeight();
private:
    const static int COLORBAR_WIDTH;
    ColorMap* mColorMap;
    QImage mImage;
    double mMinRange;
    double mMaxRange;
    bool mLogScale;
    friend class QMpxFrame;
};


//####################################################################################################
//                              QMPX SCROLL AREA
//####################################################################################################

class QMpxImage;
class QMpxScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    QMpxScrollArea(QWidget *parent = 0);
    ~QMpxScrollArea() {}
    void setWidget(QMpxImage* image){ mImage = image; QScrollArea::setWidget((QWidget*)image);}

    void setHScrollBar(int value) { horizontalScrollBar()->setValue(value); }
    void setVScrollBar(int value) { verticalScrollBar()->setValue(value); }
private slots:
    void onScroll(int value);
protected:
    virtual void resizeEvent(QResizeEvent* event);
private:
    QMpxImage* mImage;
};


//####################################################################################################
//                               QMPX IMAGE
//####################################################################################################
typedef QString (*TooltipCallback)(unsigned x, unsigned y, double value, void* userData);

class QMpxImage : public QWidget
{
    Q_OBJECT
public:
    QMpxImage(QWidget *parent = 0);
    virtual ~QMpxImage();

public:
    void setRotation(int angle);
    virtual void redraw();
    void setUnderWarning(bool enabled)              { mUnderWarning = enabled; redraw(); }
    void setOverWarning(bool enabled)               { mOverWarning = enabled; redraw(); }
    void setSpecial(bool enabled)                   { mSpecialEnabled = enabled; redraw(); }
    void setRange(double minRange, double maxRange) { mMinRange = minRange; mMaxRange = maxRange; redraw();}
    void showGrid(bool show)                        { mShowGrid = show; redraw(); }
    bool isGridVisible()                            { return mShowGrid; }
    void setMirrored(bool mirrored)                 { mMirrored = mirrored; mouseDoubleClickEvent(NULL);}
    ColorMap* getColorMap()                         { return mColorMap; }
    QSize sizeHint() const                          { return mImage.size(); }
    double getZoomFactorX()                         { return mSetZoomFactorX; }
    double getZoomFactorY()                         { return mSetZoomFactorY; }
    void showMark(int type)                         { mMarkType = type; redraw(); }
    void showRelativeTime(bool enabled)             { mShowRelativeTime = enabled; }
    void setZoomingEnabled(bool enabled)            { mZoomingEnabled = enabled; }
    void setGamma(int gamma)                        { mGamma = gamma; }
    void setLogScale(bool logScale)                 { mLogScale = logScale; }
    int gamma() const                               { return mGamma; }
    void showFirstRectDifferentColor(bool show)     { mShowFirstRectDifferentColor = show; }
    void setSelectionRects(QList<QRect> rects);
    void setSelectionRectModifierEnabled(bool enabled) { mSelRectModifierOn = enabled; }

    void setZoom(double zoomFactorX, double zoomFactorY);
    void setZoomRectRaw(QRect zoomRect);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void setTooltipCallback(TooltipCallback cb, void* userData) { mTooltipCb = cb; mTooltipCbData = userData; }
    void setOverlayTexts(QList<QRect> rects, QList<QString> texts) { mOverlayTextRects = rects; mOverlayTexts = texts; }


public:

    template <typename T> void setData(T* data, unsigned width, unsigned height)  {
        if (!data)
            return;
        if (mData) delete[] mData;
        mData = new double[width*height];
        for (unsigned i = 0; i < width*height; i++)
            mData[i] = data[i];
        mNonRotWidth = width;
        mNonRotHeight = height;
        createImage(width, height);
        if (mMaskWidth != width || mMaskHeight != height){
            delete[] mSpecialMask;
            mSpecialMask = NULL;
        }
    }

    template <typename T> void setSpecialMask(T* data, unsigned width, unsigned height){
        if (width != mWidth || height != mHeight)
            return;

        if ((mMaskWidth != width || mMaskHeight != height) && mSpecialMask){
            delete[] mSpecialMask;
            mSpecialMask = NULL;
        }

        if (!mSpecialMask){
            mSpecialMask = new unsigned char[width*height];
            mMaskWidth = width;
            mMaskHeight = height;
        }

        for (unsigned i = 0; i < width*height; i++)
            mSpecialMask[i] = static_cast<unsigned char>(data[i]);
    }

protected:
    void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);

signals:
    void zoomRectChanged(QRect zoomRect);
    void zoomRectAboutToChange();
    void dropFiles(QList<QString> files);
    void clicked(unsigned x, unsigned y, unsigned screenX, unsigned screenY, Qt::MouseButton button);
    void selectionRectsChanged(QList<QRect> rects);

protected:
    void paintFrame(double minRange, double maxRange, bool showSpecial = false, bool underWarn = false, bool overWarn = false);
    virtual void createImage(unsigned width, unsigned height);
    virtual void onScroll();
    QRect getVisibleRect();
    void clearSelectRects();
    void setQMpxScrollArea(QMpxScrollArea* scrollArea) {mScrollArea = scrollArea;}
    QRect realRectToScreenRect(QRect rect);
    QRect screenRectToRealRect(QRect rect);
    QRect realRectToRectRotatedMirrored(QRect rect, bool reverse=false);
    QPointF screenPointToRealPoint(QPointF point);

protected:
    QImage mImage;
    ColorMap* mColorMap;
    QRect mCurRect;
    QList<QRect> mSelRectsScreen;
    QList<QRect> mSelRectsReal;
    QList<QString> mOverlayTexts;
    QList<QRect> mOverlayTextRects;
    QRect mZoomRectRaw;
    QPointF mMousePos;
    QPointF mDragMousePos;
    QPointF mDragMouseScreenPos;
    QPoint mOriginalScrollBarPos;
    unsigned mWidth;
    unsigned mHeight;
    unsigned mNonRotWidth;
    unsigned mNonRotHeight;
    unsigned mMaskWidth;
    unsigned mMaskHeight;
    double* mData;
    unsigned char* mSpecialMask;
    unsigned mSpecialValue;
    double mMinRange;
    double mMaxRange;
    double mZoomFactorX;
    double mZoomFactorY;
    double mSetZoomFactorX;
    double mSetZoomFactorY;
    bool mUnderWarning;
    bool mOverWarning;
    bool mSpecialEnabled;
    bool mShowGrid;
    bool mMirrored;
    bool mDraggingImage;
    bool mZoomingEnabled;
    bool mShowRelativeTime;
    bool mShowFirstRectDifferentColor;
    bool mLogScale;
    bool mSelRectModifierOn;
    int mRotation;
    int mMarkType;
    int mGamma;
    QMpxScrollArea* mScrollArea;
    QLabel* mPosTooltip;
    TooltipCallback mTooltipCb;
    void* mTooltipCbData;
    friend class QMpxScrollArea;
    friend class QMpxFrame;
    friend class QRGBMpxFrame;
};


//####################################################################################################
//                               QMPX FRAME
//####################################################################################################

class QMpxFrame : public QWidget
{
    Q_OBJECT
public:
    enum SizeType { DATA, IMAGE};
    enum MarkType { MRK_NONE = 0, MRK_COLUMN, MRK_ROW, MRK_PIXEL};
public:
    QMpxFrame(QWidget *parent = 0);
    virtual ~QMpxFrame() {}

public:
    template <typename T> void setData(T* data, unsigned width, unsigned height) {
        bool ensureAR = width != dataWidth() || height != dataHeight();
        mImage->setData(data, width, height);
        mImage->resizeEvent(NULL);
        mImage->onScroll();
        mImage->redraw();
        if (ensureAR && mKeepAspectRatio)
            ensureAspectRatio();
    }
    template <typename T> void setSpecialMask(T* data, unsigned width, unsigned height){
        mImage->setSpecialMask(data, width, height);
        mImage->redraw();
    }
    void setInitWidthHeight(unsigned width, unsigned height) { mAxisX.setRange(1, width); mAxisY.setRange(1, height); }
    void setZoom(double zoomFactorX, double zoomFactorY) { mImage->setZoom(zoomFactorX, zoomFactorY);}
    void setZoomRect(QRect zoomRect);
    void zoomOut();
    void setZoomingEnabled(bool enabled) { mImage->setZoomingEnabled(enabled); }
    void setRotation(int rotation);
    void setMirrored(bool mirrored);
    void setRange(double minRange, double maxRange);
    void setColorMap(ColorMap::DefaultColorMap cm);
    ColorMap::DefaultColorMap colorMap() { return mImage->getColorMap()->getDefaultColorMap(); }
    void setUnderWarning(bool enabled) { mImage->setUnderWarning(enabled); }
    void setOverWarning(bool enabled) { mImage->setOverWarning(enabled); }
    void showGrid(bool show) { mImage->showGrid(show); }
    void showAxis(bool show) { mAxisX.setVisible(show); mAxisY.setVisible(show); mFillColorBar->setMinimumHeight(!show && mColorBarVisible ? 5 : 0); }
    void showColorBar(bool show) { mColorBar.setVisible(show); mFillBottom->setVisible(show); mFillColorBar->setVisible(show); }
    void showMark(MarkType type) { mImage->showMark(static_cast<int>(type)); }
    void showRelativeTime(bool enabled) { mImage->showRelativeTime(enabled); }
    void showSpecialMask(bool show) { mImage->setSpecial(show);}
    void showFirstRectDifferentColor(bool show) { mImage->showFirstRectDifferentColor(show); }
    void keepAspectRatio(bool keep);
    void setGamma(int gamma) { mImage->setGamma(gamma); }
    void setSelectionRectModifierEnabled(bool enabled) { mImage->setSelectionRectModifierEnabled(enabled); }
    void setLogScale(bool logScale) { mImage->setLogScale(logScale); mColorBar.setLogScale(logScale); redraw(); }
    void redraw() { mImage->redraw(); }
    int saveToFile(const char* fileName, unsigned width, unsigned height, SizeType sizeType, bool showColorBar, bool showAxisX, bool showAxisY);
    void setTooltipCallback(TooltipCallback cb, void* userData) { mImage->setTooltipCallback(cb, userData); }
    virtual void ensureAspectRatio();
    QMpxImage* getImage() { return mImage; }
    QMpxColorBar* getColorBar() { return &mColorBar;}
    QMpxAxis* getAxisX() { return &mAxisX;}
    QMpxAxis* getAxisY() { return &mAxisY;}
    QMpxScrollArea* getScrollArea() { return &mScrollArea;}
    QList<QRect> getSelectionRects() { return mImage->mSelRectsReal; }
    void setSelectionRects(QList<QRect> rects) { mImage->setSelectionRects(rects); }
    QRect getZoomRect() { return mZoomRect; }
    double zoomFactorX() const { return mImage->getZoomFactorX(); }
    double zoomFactorY() const { return mImage->getZoomFactorY(); }
    double minRange() const { return mImage->mMinRange; }
    double maxRange() const { return mImage->mMaxRange; }
    double dataWidth() const { return mImage->mWidth; }
    double dataHeight() const { return mImage->mHeight; }
    double* data() { return mImage->mData; }
    int gamma() const { return mImage->gamma(); }
    void setOverlayTexts(QList<QRect> rects, QList<QString> texts) { mImage->setOverlayTexts(rects, texts); }
    void paintToPainter(QPainter* painter);

    template <typename T> static int saveToFile(T* data, unsigned width, unsigned height, const char* fileName,  unsigned imgWidth, unsigned imgHeight,
                                        SizeType sizeType = DATA, double minRange = 0, double maxRange = 1, bool showColorBar = true,
                                        bool showAxisX = true, bool showAxisY = true, bool markOver = false, bool markUnder = false, ColorMap::DefaultColorMap colorMap = ColorMap::CM_GRAY)
    {
        QMpxFrame frame;
        frame.setData(data, width, height);
        frame.setRange(minRange, maxRange);
        frame.setOverWarning(markOver);
        frame.setUnderWarning(markUnder);
        frame.setColorMap(colorMap);
        return frame.saveToFile(fileName, imgWidth, imgHeight, sizeType, showColorBar, showAxisX, showAxisY);
    }

public slots:
    void onZoomRectChanged(QRect zoomRect);

private slots:
    void onDropFiles(QStringList files);
    void onZoomRectChangedDirect();

signals:
    void dropFiles(QStringList files);
    void clicked(unsigned x, unsigned y, unsigned screenX, unsigned screenY, Qt::MouseButton button);
    void selectionRectsChanged(QList<QRect> rects);
    void zoomRectChanged(QRect zoomRect);

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    QMpxImage* mImage;
    QMpxScrollArea mScrollArea;
    QMpxAxis mAxisX;
    QMpxAxis mAxisY;
    QMpxColorBar mColorBar;
    QRect mZoomRect;
    QWidget* mFillLeft;
    QWidget* mFillRight;
    QWidget* mFillTop;
    QWidget* mFillBottom;
    QWidget* mFillColorBar;
    QGridLayout* mLayout;
    int mRotation;
    bool mMirrored;
    bool mAxisVisible;
    bool mColorBarVisible;
    bool mKeepAspectRatio;
};

#endif // QMPXFRAME_H




