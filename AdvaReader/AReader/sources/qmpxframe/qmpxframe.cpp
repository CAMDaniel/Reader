/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      qmpxframe.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-07-08
 *
 */
#include "qmpxframe.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMimeData>
#include <QImage>
#include <QScrollBar>
#include <sstream>
#include <string>
#include <cmath>
#include <QDebug>

static std::string doubleToStringNice(double number)
{
    std::ostringstream buff;
    buff.precision(5);
    if (!(buff << number))
        return std::string("");
    return buff.str();
}

static const unsigned COOL_WARM_VALS[257] = {
    0xFF3B4CC0, 0xFF3C4EC2, 0xFF3D50C3, 0xFF3E51C5, 0xFF3F53C6, 0xFF4055C8, 0xFF4257C9, 0xFF4358CB, 0xFF445ACC, 0xFF455CCE, 0xFF465DCF, 0xFF475FD1, 0xFF4961D2, 0xFF4A63D3, 0xFF4B64D5, 0xFF4C66D6, 0xFF4D68D7, 0xFF4F69D9,
    0xFF506BDA, 0xFF516DDB, 0xFF526EDD, 0xFF5470DE, 0xFF5572DF, 0xFF5673E0, 0xFF5775E1, 0xFF5977E2, 0xFF5A78E4, 0xFF5B7AE5, 0xFF5D7BE6, 0xFF5E7DE7, 0xFF5F7FE8, 0xFF6080E9, 0xFF6282EA, 0xFF6383EB, 0xFF6485EC, 0xFF6687ED,
    0xFF6788EE, 0xFF688AEF, 0xFF6A8BEF, 0xFF6B8DF0, 0xFF6C8EF1, 0xFF6E90F2, 0xFF6F91F3, 0xFF7093F3, 0xFF7294F4, 0xFF7396F5, 0xFF7497F6, 0xFF7699F6, 0xFF779AF7, 0xFF789CF7, 0xFF7A9DF8, 0xFF7B9EF9, 0xFF7CA0F9, 0xFF7EA1FA,
    0xFF7FA3FA, 0xFF81A4FB, 0xFF82A5FB, 0xFF83AFC, 0xFF85A8FC, 0xFF86A9FC, 0xFF87ABFD, 0xFF89ACFD, 0xFF8AADFD, 0xFF8CAEFE, 0xFF8DB0FE, 0xFF8EB1FE, 0xFF90B2FE, 0xFF91B3FE, 0xFF93B5FF, 0xFF94B6FF, 0xFF95B7FF, 0xFF97B8FF,
    0xFF98B9FF, 0xFF99BAFF, 0xFF9BBBFF, 0xFF9CBCFF, 0xFF9EBEFF, 0xFF9FBFFF, 0xFFA0C0FF, 0xFFA2C1FF, 0xFFA3C2FF, 0xFFA4C3FE, 0xFFA6C4FE, 0xFFA7C5FE, 0xFFA8C6FE, 0xFFAAC7FD, 0xFFABC7FD, 0xFFACC8FD, 0xFFAEC9FD, 0xFFAFCAFC,
    0xFFB0CBFC, 0xFFB2CCFB, 0xFFB3CDFB, 0xFFB4CDFB, 0xFFB6CEFA, 0xFFB7CFFA, 0xFFB8D0F9, 0xFFB9D0F8, 0xFFBBD1F8, 0xFFBCD2F7, 0xFFBDD2F7, 0xFFBED3F6, 0xFFC0D4F5, 0xFFC1D4F5, 0xFFC2D5F4, 0xFFC3D5F3, 0xFFC5D6F3, 0xFFC6D6F2,
    0xFFC7D7F1, 0xFFC8D7F0, 0xFFC9D8EF, 0xFFCBD8EE, 0xFFCCD9EE, 0xFFCDD9ED, 0xFFCED9EC, 0xFFCFDAEB, 0xFFD0DAEA, 0xFFD1DBE9, 0xFFD2DBE8, 0xFFD3DBE7, 0xFFD5DBE6, 0xFFD6DCE5, 0xFFD7DCE4, 0xFFD8DCE3, 0xFFD9DCE1, 0xFFDADCE0,
    0xFFDBDCDF, 0xFFDCDDDE, 0xFFDDDDDD, 0xFFDEDCDB, 0xFFDFDCDA, 0xFFE0DBD8, 0xFFE1DBD7, 0xFFE2DAD6, 0xFFE3DAD4, 0xFFE4D9D3, 0xFFE5D8D1, 0xFFE6D8D0, 0xFFE7D7CE, 0xFFE8D7CD, 0xFFE8D6CB, 0xFFE9D5CA, 0xFFEAD4C8, 0xFFEBD4C7,
    0xFFECD3C5, 0xFFECD2C4, 0xFFEDD1C2, 0xFFEED1C1, 0xFFEED0BF, 0xFFEFCFBE, 0xFFF0CEBC, 0xFFF0CDBB, 0xFFF1CCB9, 0xFFF1CBB8, 0xFFF2CAB6, 0xFFF2C9B5, 0xFFF3C8B3, 0xFFF3C7B2, 0xFFF4C6B0, 0xFFF4C5AE, 0xFFF5C4AD, 0xFFF5C3AB,
    0xFFF5C2AA, 0xFFF5C1A8, 0xFFF6C0A7, 0xFFF6BFA5, 0xFFF6BEA3, 0xFFF6BCA2, 0xFFF7BBA0, 0xFFF7BA9F, 0xFFF7B99D, 0xFFF7B89C, 0xFFF7B69A, 0xFFF7B598, 0xFFF7B497, 0xFFF7B295, 0xFFF7B194, 0xFFF7B092, 0xFFF7AE91, 0xFFF7AD8F,
    0xFFF7AC8D, 0xFFF7AA8C, 0xFFF7A98A, 0xFFF7A789, 0xFFF7A687, 0xFFF6A486, 0xFFF6A384, 0xFFF6A183, 0xFFF6A081, 0xFFF59E7F, 0xFFF59D7E, 0xFFF59B7C, 0xFFF49A7B, 0xFFF49879, 0xFFF49778, 0xFFF39576, 0xFFF39375, 0xFFF29273,
    0xFFF29072, 0xFFF18E70, 0xFFF18D6F, 0xFFF08B6D, 0xFFF0896C, 0xFFEF886A, 0xFFEE8669, 0xFFEE8467, 0xFFED8266, 0xFFEC8164, 0xFFEC7F63, 0xFFEB7D61, 0xFFEA7B60, 0xFFE9795F, 0xFFE9785D, 0xFFE8765C, 0xFFE7745A, 0xFFE67259,
    0xFFE57058, 0xFFE46E56, 0xFFE36C55, 0xFFE36A53, 0xFFE26852, 0xFFE16651, 0xFFE0644F, 0xFFDF624E, 0xFFDE604D, 0xFFDD5E4B, 0xFFDC5C4A, 0xFFDA5A49, 0xFFD95847, 0xFFD85646, 0xFFD75445, 0xFFD65243, 0xFFD55042, 0xFFD44E41,
    0xFFD24B40, 0xFFD1493E, 0xFFD0473D, 0xFFCF453C, 0xFFCD423B, 0xFFCC4039, 0xFFCB3E38, 0xFFCA3B37, 0xFFC83936, 0xFFC73635, 0xFFC63334, 0xFFC43132, 0xFFC32E31, 0xFFC12B30, 0xFFC0282F, 0xFFBE252E, 0xFFBD222D, 0xFFBC1E2C,
    0xFFBA1A2B, 0xFFB91629, 0xFFB71128, 0xFFB50B27, 0xFFB40426
};

inline double mylog10(double x){ return x == 0 ? 0 : log10(x); }

//####################################################################################################
//                                  COLORMAP
//####################################################################################################

ColorMap::ColorMap(unsigned * _colors, unsigned size)
    : colorCount(size)
    , colors(_colors)
    , underColor(0)
    , overColor(0)
    , specialColor(0)
{
}

ColorMap::ColorMap(DefaultColorMap cm)
    : colorCount(0)
    , colors(NULL)
{
    setColorMap(cm);
}

ColorMap::~ColorMap()
{
    if (colors)
        delete[] colors;
}

void ColorMap::setColorMap(DefaultColorMap cm)
{
    switch (cm) {
        case CM_GRAY: setGrayMap(); break;
        case CM_JET: setJetMap(); break;
        case CM_JETWHITE: setJetMapWhite(); break;
        case CM_HOT: setHotMap(); break;
        case CM_COOL: setCoolMap(); break;
        case CM_HSL: setHSLMap(); break;
        case CM_COOLWARM: setCoolWarm(); break;
        case CM_INVGRAY: setInvertedGrayMap(); break;
    }
}

inline void ColorMap::setGrayMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[256];
    for (int i = 0; i < 256; i++) {
        colors[i] = 0xFF000000 +  (i) + (i<<8) + (i<<16);
    }
    colorCount = 256;
    underColor = 0xFF00FF00;
    overColor = 0xFFFF0000;
    specialColor = 0xFF0000FF;
    defColorMap = CM_GRAY;
}

inline void ColorMap::setJetMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[256];
    unsigned* clrs = colors;
    for (int i = 0; i < 32; i++)
        *(clrs++) = 0xFF000000 + (130 + i*4);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + 0xFF + ((i*4)<<8);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + ((255 - i*4)) + 0xFF00 + ((i*4)<<16);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + ((255 - i*4)<<8) + 0xFF0000;
    for (int i = 0; i < 32; i++)
        *(clrs++) =  0xFF000000 + ((255 - i*4)<<16);
    colorCount = 256;
    underColor = 0xFF000000;
    overColor = 0xFFFFFFFF;
    specialColor = 0xFF0000FF;
    defColorMap = CM_JET;
}

inline void ColorMap::setJetMapWhite()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[257];
    unsigned* clrs = colors;
    (*clrs++) = 0xFFFFFFFF;
    for (int i = 0; i < 32; i++)
        *(clrs++) = 0xFF000000 + (130 + i*4);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + 0xFF + ((i*4)<<8);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + ((255 - i*4)) + 0xFF00 + ((i*4)<<16);
    for (int i = 0; i < 64; i++)
        *(clrs++) = 0xFF000000 + ((255 - i*4)<<8) + 0xFF0000;
    for (int i = 0; i < 32; i++)
        *(clrs++) =  0xFF000000 + ((255 - i*4)<<16);
    colorCount = 257;
    underColor = 0xFF000000;
    overColor = 0xFFCC0BBF;
    specialColor = 0xFF0000FF;
    defColorMap = CM_JETWHITE;
}


inline void ColorMap::setHotMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[3*256];
    unsigned *clrs = colors;
    for (int i = 0; i < 256; i++)
        *(clrs++) = 0xFF000000 + (i << 16);
    for (int i = 0; i < 256; i++)
        *(clrs++) = (i << 8) + 0xFFFF0000;
    for (int i = 0; i < 256; i++)
        *(clrs++) = (i) + 0xFFFFFF00;
    colorCount = 3 * 256;
    underColor = 0xFF00FF00;
    overColor = 0xFF00FFFF;
    specialColor = 0xFF0000FF;
    defColorMap = CM_HOT;
}

inline void ColorMap::setCoolMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[256];
    for (int i = 0; i < 256; i++)
        colors[i] = 0xFF000000 + 0xFF + ((0xFF-i)<<8) + (i<<16);
    colorCount = 256;
    underColor = 0xFF00FF00;
    overColor = 0xFFFF0000;
    specialColor = 0xFF0000FF;
    defColorMap = CM_COOL;
}

unsigned HSLToRGB(double h, double sl, double l)
{
    unsigned char r = 0, g = 0, b = 0;
    double v;

    v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
    if (v <= 0) {
        r = g = b = 0.0;
    } else {
        double m;
        double sv;
        int sextant;
        double fract, vsf, mid1, mid2;

        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0;
        sextant = h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        m*=256; v*=256; mid1*=256; mid2*=256;
        switch (sextant) {
            case 0: r = v; g = mid1; b = m; break;
            case 1: r = mid2; g = v; b = m; break;
            case 2: r = m; g = v; b = mid1; break;
            case 3: r = m; g = mid2; b = v; break;
            case 4: r = mid1; g = m; b = v; break;
            case 5: r = v; g = m; b = mid2; break;
        }
    }
    return (r<<16)+(g << 8)+b;
}

inline void ColorMap::setHSLMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[3*256];
    for (int i = 0; i < 3*256; i++) {
        colors[i] = 0xFF000000+HSLToRGB((double)i/(double)(3*256+3*256*0.1), 0.9, 0.5);
    }
    colorCount = 3*256;
    underColor = 0xFF000000;
    overColor = 0xFFFFFFFF;
    specialColor = 0xFF0000FF;
    defColorMap = CM_HSL;
}

inline void ColorMap::setInvertedGrayMap()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[256];
    for (int i = 0; i < 256; i++) {
        colors[255-i] = 0xFF000000 + (i) + (i<<8) + (i<<16);
    }
    colorCount = 256;
    underColor = 0xFF00FF00;
    overColor = 0xFFFF0000;
    specialColor = 0xFF0000FF;
    defColorMap = CM_INVGRAY;
}

void ColorMap::setCoolWarm()
{
    if (colors)
        delete[] colors;
    colors = new unsigned[256];
    for (int i = 0; i < 256; i++)
        colors[i] = COOL_WARM_VALS[i];
    colorCount = 256;
    underColor = 0xFF000000;
    overColor = 0xFF00FF00;
    specialColor = 0xFF00FFFF;
    defColorMap = CM_COOLWARM;
}


//####################################################################################################
//                               QMPX COLOR BAR
//####################################################################################################

const int QMpxColorBar::COLORBAR_WIDTH = 15;
QMpxColorBar::QMpxColorBar(QWidget *parent)
    : QWidget(parent)
    , mColorMap(NULL)
    , mMinRange(0)
    , mMaxRange(1)
    , mLogScale(false)
{
    setMinimumHeight(30);
}

void QMpxColorBar::setColorMap(ColorMap* map)
{
    mColorMap = map;
    mImage = QImage(map->getCount(), COLORBAR_WIDTH, QImage::Format_ARGB32);
    QPainter p;
    p.begin(&mImage);
    unsigned* colors = mColorMap->getColors();
    for (unsigned i = 0; i < mColorMap->getCount(); i++){
        p.setPen(QColor(QRgb(colors[i])));
        p.drawLine(i, 0, i, COLORBAR_WIDTH);
    }
    p.end();
}

void QMpxColorBar::paint(QPainter* painter, unsigned x, unsigned y, unsigned width)
{
    if (!mColorMap)  return;
    QPen textPen = painter->pen();
    painter->drawImage(QRect(x, y, width, COLORBAR_WIDTH), mImage);
    painter->setPen(Qt::black);
    painter->drawLine(x, y + COLORBAR_WIDTH, x + width-1, y + COLORBAR_WIDTH);
    double stepx = (double)width/4.0;
    QFontMetrics fm = fontMetrics();
    int lh = fm.lineSpacing();

    painter->drawLine(x, y + COLORBAR_WIDTH-5, x, y + COLORBAR_WIDTH);
    painter->setPen(textPen);
    painter->drawText(x, y + COLORBAR_WIDTH+lh, doubleToStringNice(mMinRange).c_str());
    for (int i = 1; i < 4; i++){
        double val = (double)(mMaxRange-mMinRange)/4.0*i;
        if (mLogScale)
            val = (double)pow(10, (mylog10(mMaxRange-mMinRange)/4.0*i));
        QString strVal = doubleToStringNice(val).c_str();
        painter->setPen(Qt::black);
        painter->drawLine(x + i*stepx, y + COLORBAR_WIDTH-5, x + i*stepx, y + COLORBAR_WIDTH);
        painter->setPen(textPen);
        painter->drawText(x + i*stepx-fm.width(strVal)/2, y + COLORBAR_WIDTH+lh, strVal);
    }
    QString strVal = doubleToStringNice(mMaxRange).c_str();
    painter->drawText(x + 4*stepx-fm.width(strVal), y + COLORBAR_WIDTH+lh, strVal);
    painter->setPen(Qt::black);
    painter->drawLine(x + 4*stepx-1, y + COLORBAR_WIDTH-5, x + 4*stepx-1, y + COLORBAR_WIDTH);
}

unsigned QMpxColorBar::getColorbarHeight()
{
    QFontMetrics fm = fontMetrics();
    return COLORBAR_WIDTH + fm.lineSpacing();
}

void QMpxColorBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!mColorMap)  return;
    QPainter painter(this);
    paint(&painter, 0, 0, rect().width());
}


//####################################################################################################
//                                   QMpx Axis
//####################################################################################################

QMpxAxis::QMpxAxis(QWidget *parent, AxisType axisType)
    : QWidget(parent)
    , mAxisType(axisType)
{
    mMax.setText("256");
    mMin.setText("0");
    mMax.setMargin(2);
    mName.setMargin(2);
    mMin.setMargin(2);
    if (axisType == AxisX){
        mName.setText("X (column number)");
        mMin.setAlignment(Qt::AlignLeft | Qt::AlignTop);
        mName.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        mMax.setAlignment(Qt::AlignRight | Qt::AlignTop);
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(&mMin);
        layout->addWidget(&mName);
        layout->addWidget(&mMax);
        this->setLayout(layout);
    }else{
        setMinimumWidth(30);
        mName.setText("Y");
        mMax.setAlignment(Qt::AlignTop | Qt::AlignRight);
        mName.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        mMin.setAlignment(Qt::AlignBottom | Qt::AlignRight);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(&mMax);
        layout->addWidget(&mName);
        layout->addWidget(&mMin);
        this->setLayout(layout);
    }
}

QSize QMpxAxis::getAxisSize()
{
    QFontMetrics fm = fontMetrics();
    if (mAxisType == AxisY){
        unsigned width1 = fm.width(mMin.text());
        unsigned width2 = fm.width(mMax.text());
        unsigned width = width1>width2?width1:width2;
        return QSize(width + 5, height());
    }else{
        return QSize(width(), fm.lineSpacing() + 5);
    }
}

void QMpxAxis::paint(QPainter* painter, unsigned x, unsigned y, unsigned width, unsigned height)
{
    painter->setPen(Qt::black);
    QFontMetrics fm = fontMetrics();
    int offset = 3;
    int lh = fm.height();
    if (mAxisType == AxisX){
        painter->drawText(x, y - offset + lh, mMin.text());
        painter->drawText(x+width-fm.width(mMax.text()), y - offset + lh, mMax.text());
        painter->drawText(x+width/2-fm.width(mName.text())/2-offset, y - offset + lh, mName.text());
    }else{
        int w1 = fm.width(mMax.text());
        int w2 = fm.width(mMin.text());
        int w = w1>w2?w1:w2;
        painter->drawText(x+offset + w - fm.width(mMax.text()), y+fm.height(), mMax.text());
        painter->drawText(x+offset + w - fm.width(mMin.text()), y+height, mMin.text());
        painter->drawText(x+offset + w - fm.width(mName.text()), y+height/2-lh/2, mName.text());
    }
}

void QMpxAxis::setRange(double minRange, double maxRange)
{
    mMin.setText(doubleToStringNice(minRange).c_str());
    mMax.setText(doubleToStringNice(maxRange).c_str());
}


//####################################################################################################
//                               MY SCROLL AREA
//####################################################################################################

QMpxScrollArea::QMpxScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));
    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void QMpxScrollArea::onScroll(int)
{
    mImage->onScroll();
}

void QMpxScrollArea::resizeEvent(QResizeEvent* event)
{
    if (widget() == NULL)
        return QScrollArea::resizeEvent(event);
    int width = widget()->size().width();
    int height = widget()->size().height();
    int viewWidth = viewport()->size().width();
    int viewHeight = viewport()->size().height();
    int imgWidth = widget()->sizeHint().width();
    int imgHeight = widget()->sizeHint().height();
    int zoomWidth = imgWidth*mImage->getZoomFactorX();
    int zoomHeight = imgHeight*mImage->getZoomFactorY();
    if (viewWidth > width || viewWidth > zoomWidth || mImage->getZoomFactorX() == 1) width = viewWidth;
    if (viewHeight >  height || viewHeight > zoomHeight || mImage->getZoomFactorY() == 1) height = viewHeight;
    if (width != widget()->size().width() || height != widget()->size().height())
        widget()->resize(width, height);
    else
        mImage->onScroll();
    return QScrollArea::resizeEvent(event);
}

//####################################################################################################
//                               QMPX IMAGE
//####################################################################################################

QMpxImage::QMpxImage(QWidget *parent)
    : QWidget(parent)
    , mMousePos(QPointF(-1, -1))
    , mWidth(0)
    , mHeight(0)
    , mData(NULL)
    , mSpecialMask(NULL)
    , mSpecialValue(0)
    , mMinRange(0)
    , mMaxRange(1)
    , mZoomFactorX(1)
    , mZoomFactorY(1)
    , mSetZoomFactorX(1)
    , mSetZoomFactorY(1)
    , mUnderWarning(false)
    , mOverWarning(false)
    , mSpecialEnabled(false)
    , mShowGrid(false)
    , mMirrored(true)
    , mDraggingImage(false)
    , mZoomingEnabled(true)
    , mShowRelativeTime(false)
    , mShowFirstRectDifferentColor(false)
    , mLogScale(false)
    , mSelRectModifierOn(true)
    , mRotation(180)
    , mMarkType(QMpxFrame::MRK_NONE)
    , mGamma(0)
    , mScrollArea(NULL)
    , mTooltipCb(NULL)
    , mTooltipCbData(NULL)
{
    mColorMap = new ColorMap(ColorMap::CM_GRAY);
    setRange(0, 1);
    setMouseTracking(true);
    mPosTooltip =  new QLabel("0, 0", this);
    mPosTooltip->hide();
    mPosTooltip->setStyleSheet("background-color: #f5e39a;");
    setAcceptDrops(true);
}

QMpxImage::~QMpxImage()
{
    if (mColorMap) delete mColorMap;
    if (mData) delete[] mData;
    if (mSpecialMask) delete[] mSpecialMask;
}

// converts screen coordinates (in pixels) to detector pixels
// rotation is not reflected
QRect QMpxImage::screenRectToRealRect(QRect rect)
{
    if (mZoomFactorX == 0 || mZoomFactorY == 0)
        return QRect();
    // unzoom:
    QRect rectout = QRect(rect.left()/mZoomFactorX+0.5, rect.top()/mZoomFactorY+0.5,
                          rect.width()/mZoomFactorX+0.5, rect.height()/mZoomFactorY+0.5);
    rectout = rectout.normalized();

    // make sure it's in the bounds
    if (rectout.left() < 0) rectout.setLeft(0);
    if (rectout.top() < 0) rectout.setTop(0);
    if (rectout.right() > (int)mImage.width() - 1) rectout.setRight(mImage.width() - 1);
    if (rectout.bottom() > (int)mImage.height() - 1) rectout.setBottom(mImage.height() - 1);
    return rectout;
}

// converts detector pixels rect to screen rect (zooms it)
// rotation not reflected
QRect QMpxImage::realRectToScreenRect(QRect rect)
{
    return QRect(rect.left()*mZoomFactorX+0.5, rect.top()*mZoomFactorY+0.5,
                 rect.width()*mZoomFactorX+0.5, rect.height()*mZoomFactorY+0.5);
}

// convertes detector pixels (rotation not reflected) to detector pixels where
// rotation and mirroring is reflected
QRect QMpxImage::realRectToRectRotatedMirrored(QRect rect, bool reverse)
{
    int rotation = mMirrored ? (mRotation == 0 ? -360 : -mRotation) : mRotation;

    QSize imgsize = QSize(mNonRotWidth, mNonRotHeight);
    if (reverse){
        switch(rotation){
            case 0: return QRect(rect.left(), imgsize.height()-rect.bottom()-1, rect.width(), rect.height());
            case 270:  return QRect(rect.top(), imgsize.width()-rect.right()-1, rect.height(), rect.width());
            case 180: return QRect(imgsize.width()-rect.right()-1, imgsize.height()-rect.bottom()-1, rect.width(), rect.height());
            case 90: return QRect(imgsize.height()-rect.bottom()-1, rect.left(), rect.height(), rect.width());
            case -90: return QRect(imgsize.height()-rect.bottom()-1, imgsize.width()-rect.right()-1, rect.height(), rect.width());
            case -270: return QRect(rect.top(), rect.left(), rect.height(), rect.width());
            case -360: return QRect(imgsize.width()-rect.right()-1, rect.top(), rect.width(), rect.height());
        }
    }

    switch(rotation){
        case 0: return rect;
        case 90:  return QRect(rect.top(), imgsize.height()-rect.right()-1, rect.height(), rect.width());
        case 180: return QRect(imgsize.width()-rect.right()-1, imgsize.height()-rect.bottom()-1, rect.width(), rect.height());
        case 270: return QRect(imgsize.width()-rect.bottom()-1, rect.left(), rect.height(), rect.width());
        case -90: return QRect(imgsize.width()-rect.bottom()-1, imgsize.height()-rect.right()-1, rect.height(), rect.width());
        case -180: return QRect(rect.left(), imgsize.height()-rect.bottom()-1, rect.width(), rect.height());
        case -270: return QRect(rect.top(), rect.left(), rect.height(), rect.width());
        case -360: return QRect(imgsize.width()-rect.right()-1, rect.top(), rect.width(), rect.height());
    }
    return QRect();
}

QPointF QMpxImage::screenPointToRealPoint(QPointF point)
{
    QPoint p = QPoint(point.x()/mZoomFactorX, point.y()/mZoomFactorY);
    int rotation = mMirrored ? (mRotation == 0 ? -360 : -mRotation) : mRotation;

    QSize imgsize = mImage.size();
    switch(rotation){
        case 0: return p;
        case 90:  return QPoint(p.y(), imgsize.width()-p.x()-1);
        case 180: return QPoint(imgsize.width()-p.x()-1, imgsize.height()-p.y()-1);
        case 270: return QPoint(imgsize.height()-p.y()-1, p.x());
        case -90: return QPoint(imgsize.height()-p.y()-1, imgsize.width()-p.x()-1);
        case -180: return QPoint(p.x(), imgsize.height()-p.y()-1);
        case -270: return QPoint(p.y(), p.x());
        case -360: return QPoint(imgsize.width()-p.x()-1, p.y());
    }
    return p;
}

QRect QMpxImage::getVisibleRect()
{
    QRect visibleRect = QRect(-pos().x(), -pos().y(), parentWidget()->size().width(), parentWidget()->size().height());
    if (mZoomFactorX == 0 || mZoomFactorY == 0)
        return QRect(0, 0, mNonRotWidth, mNonRotHeight);
    QRect rect= visibleRect;
    int left = visibleRect.left() / mZoomFactorX+0.5;
    int top =  visibleRect.top() / mZoomFactorY+0.5;
    int width = visibleRect.width() / mZoomFactorX + 0.5;
    int height =  visibleRect.height() / mZoomFactorY + 0.5;
    if (width == 0) width = 1;
    if (height == 0) height = 1;
    rect = QRect(left, top, width, height);
    return rect;
}

void QMpxImage::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), QBrush(Qt::black));
    QRect rect = mZoomRectRaw;
    rect = QRect(std::max(rect.left()-1, 0), std::max(rect.top()-1, 0),
                         std::min(rect.width()+2,mImage.width()), std::min(rect.height()+2,mImage.height()));
    QRect visibleRect = QRect(rect.left()*mZoomFactorX, rect.top()*mZoomFactorY,
                              ceil(rect.width()*mZoomFactorX), ceil(rect.height()*mZoomFactorY));
    painter.drawImage(visibleRect, mImage,rect);

    if (mShowGrid){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(QColor(0x80,0x80,0x80));
        for (int x = rect.left(); x < rect.right(); x++)
            painter.drawLine(x*mZoomFactorX+0.5, rect.top()*mZoomFactorY+0.5, x*mZoomFactorX+0.5, rect.bottom()*mZoomFactorY+0.5);
        for (int y = rect.top(); y < rect.bottom(); y++)
            painter.drawLine(rect.left()*mZoomFactorX+0.5, y*mZoomFactorY+0.5, rect.right()*mZoomFactorX+0.5, y*mZoomFactorY+0.5);
    }

    if (!mCurRect.isEmpty()){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(QColor(0xFF,0xFF,0xFF));
        QRect rect(mCurRect.left(), mCurRect.top(), mCurRect.width(), mCurRect.height());
        painter.drawRect(rect);
        if (mShowGrid && (mZoomFactorX > 1.2 || mZoomFactorY > 1.2)) // so that the rect is visible, before xoring twice hides it
            painter.drawRect(rect.adjusted(1,1,-1,-1));
    }

    if (!mSelRectsScreen.isEmpty()){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        //painter.setPen(QColor(0xFF,0xFF,0xFF));
        for (int i = 0; i < mSelRectsScreen.size(); i++) {
            QRect rect = mSelRectsScreen[i];
            if (mShowFirstRectDifferentColor && i == 0 && mSelRectsScreen.size() > 1)
                painter.setPen(QColor(0xFF,0x00,0x00));
            else
                painter.setPen(QColor(0xFF,0xFF,0xFF));
            painter.drawRect(rect);
            if (mShowGrid && (mZoomFactorX > 1.2 || mZoomFactorY > 1.2)) // so that the rect is visible, before xoring twice hides it
                painter.drawRect(rect.adjusted(1,1,-1,-1));
        }
    }

    if (mMarkType == QMpxFrame::MRK_COLUMN){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        QRect mrect = realRectToScreenRect(realRectToRectRotatedMirrored(QRect(mMousePos.x(), 0, 1, mImage.height()), true));
        painter.fillRect(mrect, QBrush(Qt::white));
    }

    if (mMarkType == QMpxFrame::MRK_ROW){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        QRect mrect = realRectToScreenRect(realRectToRectRotatedMirrored(QRect(0, mMousePos.y(), mImage.width(), 1), true));
        painter.fillRect(mrect, QBrush(Qt::white));
    }

    //if (mMarkType == QMpxFrame::MRK_PIXEL){
        //painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        //QRect mrect = realRectToScreenRect(realRectToRectRotatedMirrored(QRect(mMousePos.x(), mMousePos.y(), 1, 1)));
        //painter.fillRect(mrect, QBrush(Qt::white));
    //}

    if (!mOverlayTexts.empty()){
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(QColor(0x80,0x80,0x80));
        for (int i = 0; i < mOverlayTexts.size(); i++){
            QRect r = realRectToScreenRect(realRectToRectRotatedMirrored(mOverlayTextRects[i], true));
            int flags = Qt::TextDontClip|Qt::TextWordWrap;
            QRect fontBoundRect =  painter.fontMetrics().boundingRect(r, flags, mOverlayTexts[i]);
            float xFactor = r.width() / fontBoundRect.width();
            float yFactor = r.height() / fontBoundRect.height();
            float factor = xFactor < yFactor ? xFactor : yFactor;
            QFont f = painter.font();
            f.setPointSizeF(f.pointSizeF()*factor);
            painter.setFont(f);
            painter.drawText(r, mOverlayTexts[i], QTextOption(Qt::AlignVCenter | Qt::AlignCenter));
            painter.drawRect(r.adjusted(1, 1, -1, -1));
        }
    }
}

void QMpxImage::setZoom(double zoomFactorX, double zoomFactorY)
{
    mZoomFactorX = zoomFactorX;
    mZoomFactorY = zoomFactorY;
    mSetZoomFactorX = zoomFactorX;
    mSetZoomFactorY = zoomFactorY;
    int newWidth = mImage.width()*mZoomFactorX;
    int newHeight = mImage.height()*mZoomFactorY;
    resize(std::max(newWidth, parentWidget()->rect().width()), std::max(newHeight, parentWidget()->rect().height()));
}

void QMpxImage::setZoomRectRaw(QRect zoomRect)
{
    mZoomRectRaw = zoomRect;
    emit zoomRectAboutToChange();
    mZoomFactorX = (double)parentWidget()->rect().width()/(double)zoomRect.width();
    mZoomFactorY = (double)parentWidget()->rect().height()/(double)zoomRect.height();
    mSetZoomFactorX = mZoomFactorX;
    mSetZoomFactorY = mZoomFactorY;
    int newWidth = mImage.width()*mZoomFactorX;
    int newHeight = mImage.height()*mZoomFactorY;
    resize(std::max(newWidth, parentWidget()->rect().width()), std::max(newHeight, parentWidget()->rect().height()));
    if (mScrollArea){
        mScrollArea->setHScrollBar(zoomRect.left()*mZoomFactorX);
        mScrollArea->setVScrollBar(zoomRect.top()*mZoomFactorY);
    }
    update();
}

void QMpxImage::onScroll()
{
    mZoomRectRaw = getVisibleRect();
    update();
    emit zoomRectChanged(realRectToRectRotatedMirrored(mZoomRectRaw));
}

void QMpxImage::resizeEvent(QResizeEvent* event)
{
    QSize imgSize = mImage.size();
    mZoomFactorX = imgSize.width() == 0 ? 1 : (double)size().width()/(double)imgSize.width();
    mZoomFactorY = imgSize.height() == 0 ? 1 : (double)size().height()/(double)imgSize.height();
    if (mZoomFactorX == 0) mZoomFactorX = 1;
    if (mZoomFactorY == 0) mZoomFactorY = 1;
    mZoomRectRaw = getVisibleRect();
    //update();
    onScroll();
    return QWidget::resizeEvent(event);
}

#define OFFSET 25.0
void QMpxImage::mouseMoveEvent(QMouseEvent* event)
{
    // coordinates tooltip
    if (mData){
        QPointF p = screenPointToRealPoint(event->localPos());
        int x = p.x();
        int y = p.y();
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > (int)mNonRotWidth - 1) x = mNonRotWidth - 1;
        if (y > (int)mNonRotHeight - 1) y = mNonRotHeight - 1;

        unsigned idx = y*mNonRotWidth+x;
        QString tooltip;
        if (mTooltipCb){
            tooltip = mTooltipCb(x, y, idx<mNonRotWidth*mNonRotHeight?mData[y*mNonRotWidth+x] : 0, mTooltipCbData);
            mPosTooltip->setText(tooltip);
        }
        if (!mTooltipCb || tooltip.isEmpty()){
            if (idx < mNonRotWidth*mNonRotHeight){
                if (mShowRelativeTime){
                    double relative = mMinRange <= mData[y*mNonRotWidth+x] ? mData[y*mNonRotWidth+x]-mMinRange : 0;
                    mPosTooltip->setText(QString("[%1,%2]=%3 (%4)").arg(x+1).arg(y+1).arg(mData[y*mNonRotWidth+x]).arg(relative));
                }else
                    mPosTooltip->setText(QString("[%1,%2]=%3").arg(x+1).arg(y+1).arg(mData[y*mNonRotWidth+x]));
            }else
                mPosTooltip->setText(QString("[%1,%2]=%3").arg(x+1).arg(y+1).arg(0));
        }
        mPosTooltip->adjustSize();
        int posx = mapToParent(event->pos()).x();
        int posy = mapToParent(event->pos()).y();
        if (posy > mScrollArea->height()-mPosTooltip->height()-40)
            posy = event->pos().y() - 30;
        else
            posy = event->pos().y() + 30;
        mPosTooltip->move(event->pos().x()-(-30+1.0*(mPosTooltip->width()+50)/mScrollArea->width()*posx), posy);
        if (!mPosTooltip->isVisible())
            mPosTooltip->show();

        if ((mMousePos.x() != p.x() || mMousePos.y() != p.y()) && mMarkType != QMpxFrame::MRK_NONE){
            mMousePos = p;
            redraw();
        }else
            mMousePos = p;
    }

    if (mDragMousePos.isNull())
        return QWidget::mouseMoveEvent(event);

    // dragging image with right button
    if (mDraggingImage && !mDragMousePos.isNull() && mScrollArea) {
        mScrollArea->setHScrollBar(mOriginalScrollBarPos.x() - (event->screenPos().x() - mDragMouseScreenPos.x()));
        mScrollArea->setVScrollBar(mOriginalScrollBarPos.y() - (event->screenPos().y() - mDragMouseScreenPos.y()));
        return QWidget::mouseMoveEvent(event);
    }

    // creating rect
    QRect rect = QRect(mDragMousePos.x(), mDragMousePos.y(), event->localPos().x()-mDragMousePos.x(),
                       event->localPos().y()-mDragMousePos.y()).normalized();

    if (event->modifiers() & Qt::SHIFT){
        if (rect.width() > rect.height())
            rect.setHeight(rect.width()*this->height()/this->width());
        else
            rect.setWidth(this->width()*rect.height()/this->height());
    }

    rect = screenRectToRealRect(rect);
    if (rect.width() == 0) rect.setWidth(1);
    if (rect.height() == 0) rect.setHeight(1);
    mCurRect = realRectToScreenRect(rect);
    update();
    return QWidget::mouseMoveEvent(event);
}

void QMpxImage::mousePressEvent(QMouseEvent* event)
{
    if (!mData) return;
    if (mZoomingEnabled)
        mDragMousePos = event->localPos();

    // draging of image with right button
    if (event->button() == Qt::RightButton){
        setCursor(Qt::ClosedHandCursor);
        mDraggingImage = true;
        mDragMouseScreenPos = event->screenPos();
        mOriginalScrollBarPos = QPoint(mScrollArea->horizontalScrollBar()->value(),
                                      mScrollArea->verticalScrollBar()->value());
    }
    QPointF p = screenPointToRealPoint(event->localPos());
    emit clicked(p.x(), p.y(), event->localPos().x(), event->localPos().y(), event->button());
    return QWidget::mousePressEvent(event);
}

void QMpxImage::mouseReleaseEvent(QMouseEvent* event)
{
    setCursor(Qt::ArrowCursor);
    mDraggingImage = false;
    if (mCurRect.isEmpty()) {
        mDragMousePos = QPoint();
        if (!mSelRectsScreen.empty() && event->button() == Qt::RightButton){
            for (int i = 0; i < mSelRectsScreen.size(); i++)
                if (mSelRectsScreen[i].contains(event->localPos().x(), event->localPos().y())){
                    mSelRectsScreen.removeAt(i);
                    mSelRectsReal.removeAt(i);
                    emit selectionRectsChanged(mSelRectsReal);
                    update();
                    break;
                }
        }
        return;
    }

    QRect rect = QRect(mDragMousePos.x(), mDragMousePos.y(), event->localPos().x()-mDragMousePos.x(),
                       event->localPos().y()-mDragMousePos.y());
    if (event->modifiers() & Qt::SHIFT){
        if (rect.width() > rect.height())
            rect.setHeight(rect.width()*this->height()/this->width());
        else
            rect.setWidth(this->width()*rect.height()/this->height());
    }

    // selection rect
    if (event->modifiers() & Qt::CTRL || !mSelRectModifierOn){
        mSelRectsScreen.append(mCurRect);
        QRect realRect = realRectToRectRotatedMirrored(screenRectToRealRect(rect.normalized()));
        mSelRectsReal.append(realRect);
        emit selectionRectsChanged(mSelRectsReal);
    }else{
        clearSelectRects();
        QRect zoomRect = screenRectToRealRect(rect.normalized());
        setZoomRectRaw(zoomRect);
        emit zoomRectChanged(realRectToRectRotatedMirrored(zoomRect));
    }
    mCurRect = QRect();
    mDragMousePos = QPoint();
    return QWidget::mouseReleaseEvent(event);
}

void QMpxImage::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (!mZoomingEnabled)
        return;
    Q_UNUSED(event)
    clearSelectRects();
    setCursor(Qt::ArrowCursor);
    mDraggingImage = false;
    setZoomRectRaw(mImage.rect());
    mSetZoomFactorX = 1;
    mSetZoomFactorY = 1;
    emit zoomRectChanged(QRect(0, 0, mNonRotWidth, mNonRotHeight));
    mCurRect = QRect();
    mDragMousePos = QPoint();
}

void QMpxImage::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    mPosTooltip->hide();
}

void QMpxImage::redraw()
{
    paintFrame(mMinRange, mMaxRange, mSpecialEnabled, mUnderWarning, mOverWarning);
    update();
}

void QMpxImage::setSelectionRects(QList<QRect> rects)
{
    mSelRectsReal = rects;
    mSelRectsScreen.clear();
    for (int i = 0; i < mSelRectsReal.size(); i++){
        QRect rect = mSelRectsReal[i];
        QRect srect = realRectToScreenRect(realRectToRectRotatedMirrored(rect, true));
        mSelRectsScreen.append(srect);
    }
    update();
}

inline void QMpxImage::clearSelectRects()
{
    if (!mSelRectsReal.isEmpty() || !mSelRectsScreen.isEmpty()){
        mSelRectsScreen.clear();
        mSelRectsReal.clear();
        emit selectionRectsChanged(mSelRectsReal);
    }
}

void QMpxImage::createImage(unsigned width, unsigned height)
{
    if (mRotation == 90 || mRotation == 270){
        mWidth = height;
        mHeight = width;
    }else{
        mWidth = width;
        mHeight = height;
    }
    if (mWidth != static_cast<unsigned>(mImage.width()) || mHeight != static_cast<unsigned>(mImage.height()))
        mImage = QImage(mWidth, mHeight, QImage::Format_ARGB32);
}

void QMpxImage::setRotation(int angle)
{
    mRotation = angle;
    createImage(mNonRotWidth, mNonRotHeight);
    mouseDoubleClickEvent(NULL);
    redraw();
}

void QMpxImage::paintFrame(double minRange, double maxRange, bool showSpecial, bool underWarn, bool overWarn)
{
    if (!mData || mWidth == 0 || mHeight == 0 || mImage.width() == 0 || mImage.height() == 0) return;
    unsigned* colors = mColorMap->getColors();
    unsigned idxMax = mColorMap->getCount() - 1;
    unsigned overColor = mColorMap->getOverRangeColor();
    unsigned underColor = mColorMap->getUnderRangeColor();
    unsigned specialColor = mColorMap->getSpecialColor();

    unsigned char* bits = mImage.bits();
    int perLine  = mImage.bytesPerLine();
    double step = (maxRange - minRange) / idxMax;
    double logStep = (mylog10(maxRange) - mylog10(minRange)) / idxMax;
    if (step == 0)
        return;
    if (mLogScale && logStep == 0)
        return;

    // if mirrored, negate angle, for 0, set it to -360
    int rotation = mMirrored ? (mRotation == 0 ? -360 : -mRotation) : mRotation;
    unsigned idx = 0;
    double gamma = mGamma;
    if (gamma > 0) gamma++;
    if (gamma < 0) gamma = 1 / (-gamma);

    if (gamma != 0) {
        for (unsigned y = 0; y < mHeight; y++) {
            unsigned* line = reinterpret_cast<unsigned*>(bits + y*perLine);
            for (unsigned x = 0; x < mWidth; x++) {
                switch(rotation){
                    case 0: idx = y*mWidth+x; break;
                    case 90: idx = (mWidth-x-1)*mHeight + y; break;
                    case 180: idx = (mHeight-y-1)*mWidth + mWidth-x-1; break;
                    case 270: idx = x*mHeight + mHeight-y-1; break;
                    case -90: idx = (mWidth-x-1)*mHeight + mHeight-y-1; break;
                    case -180: idx = (mHeight-y-1)*mWidth + x; break;
                    case -270: idx = x*mHeight + y; break;
                    case -360: idx  = mWidth*y + (mWidth-x-1); break;
                }
                if (showSpecial && mSpecialMask &&  mSpecialMask[idx] == mSpecialValue)
                    line[x] = specialColor;
                else if (maxRange <= minRange)
                    line[x] = 0;
                else if (mData[idx] > maxRange)
                    line[x] = overWarn ? overColor : colors[idxMax];
                else if (mData[idx] < minRange)
                    line[x] = underWarn ? underColor : colors[0];
                else{
                    double val = pow((mData[idx] - minRange)/(maxRange - minRange), gamma);
                    if (mLogScale)
                        val = pow(mylog10(mData[idx] - minRange)/(mylog10(maxRange) - mylog10(minRange)), gamma);
                    line[x] = colors[static_cast<int>(val*idxMax)];
                }
            }
        }
    }else {
        for (unsigned y = 0; y < mHeight; y++) {
            unsigned* line = reinterpret_cast<unsigned*>(bits + y*perLine);
            for (unsigned x = 0; x < mWidth; x++) {
                switch(rotation){
                    case 0: idx = y*mWidth+x; break;
                    case 90: idx = (mWidth-x-1)*mHeight + y; break;
                    case 180: idx = (mHeight-y-1)*mWidth + mWidth-x-1; break;
                    case 270: idx = x*mHeight + mHeight-y-1; break;
                    case -90: idx = (mWidth-x-1)*mHeight + mHeight-y-1; break;
                    case -180: idx = (mHeight-y-1)*mWidth + x; break;
                    case -270: idx = x*mHeight + y; break;
                    case -360: idx  = mWidth*y + (mWidth-x-1); break;
                }
                if (mData[idx] != mData[idx]){
                    line[x] = overWarn ? overColor : colors[idxMax];
                    continue;
                }
                if (showSpecial && mSpecialMask &&  mSpecialMask[idx] == mSpecialValue)
                    line[x] = specialColor;
                else if (maxRange <= minRange)
                    line[x] = 0;
                else if (mData[idx] > maxRange)
                    line[x] = overWarn ? overColor : colors[idxMax];
                else if (mData[idx] < minRange)
                    line[x] = underWarn ? underColor : colors[0];
                else{
                    if (mLogScale)
                        line[x] = colors[static_cast<int>((mylog10(mData[idx] - minRange))/logStep)];
                    else
                        line[x] = colors[static_cast<int>((mData[idx] - minRange)/step)];
                }
            }
        }
    }
}

void QMpxImage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void QMpxImage::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()){
        event->accept();
        QList<QUrl> urls = event->mimeData()->urls();
        QStringList files;
        for (int i = 0; i < urls.size(); i++){
            files.append(urls[i].toLocalFile());
        }
        emit dropFiles(files);
    }
}

//####################################################################################################
//                               QMPX FRAME
//####################################################################################################

QMpxFrame::QMpxFrame(QWidget *parent)
    : QWidget(parent)
    , mAxisX(parent, QMpxAxis::AxisX)
    , mAxisY(parent, QMpxAxis::AxisY)
    , mFillLeft(NULL)
    , mFillRight(NULL)
    , mFillTop(NULL)
    , mFillBottom(NULL)
    , mRotation(180)
    , mMirrored(true)
    , mAxisVisible(true)
    , mColorBarVisible(true)
    , mKeepAspectRatio(false)
{
    mImage = new QMpxImage(parent);
    mScrollArea.setWidget(mImage);
    mImage->setQMpxScrollArea(&mScrollArea);
    mColorBar.setColorMap(mImage->getColorMap());
    mFillLeft = new QWidget(this);
    mFillRight = new QWidget(this);
    mFillTop = new QWidget(this);
    mFillBottom = new QWidget(this);
    mFillColorBar = new QWidget(this);

    mFillLeft->setMinimumWidth(0);
    mFillRight->setMinimumWidth(0);
    mFillTop->setMinimumHeight(0);
    mFillBottom->setMinimumHeight(0);
    mFillColorBar->setMinimumHeight(0);

    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(mFillTop, 0, 0, 1, 4);
    layout->addWidget(mFillLeft, 1, 0, 3, 1);
    layout->addWidget(&mAxisY, 1, 1);
    layout->addWidget(&mScrollArea, 1, 2);
    layout->addWidget(mFillRight, 1, 3, 3, 1);
    layout->addWidget(&mAxisX, 2, 2);
    layout->addWidget(mFillColorBar, 3, 2);
    layout->addWidget(&mColorBar, 4, 2);
    layout->addWidget(mFillBottom, 5,0, 1, 4);
    this->setLayout(layout);
    mLayout = layout;
    connect(mImage, SIGNAL(zoomRectChanged(QRect)), this, SLOT(onZoomRectChanged(QRect)));
    connect(mImage, SIGNAL(zoomRectAboutToChange()), this, SLOT(onZoomRectChangedDirect()), Qt::DirectConnection);
    connect(mImage, &QMpxImage::dropFiles, this, &QMpxFrame::dropFiles);
    connect(mImage, &QMpxImage::clicked, this, &QMpxFrame::clicked);
    connect(mImage, &QMpxImage::selectionRectsChanged, this, &QMpxFrame::selectionRectsChanged);
    this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

void QMpxFrame::onDropFiles(QStringList files)
{
    emit dropFiles(files);
}

void QMpxFrame::onZoomRectChangedDirect()
{
    if (mKeepAspectRatio)
        ensureAspectRatio();
}

void QMpxFrame::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    if (mImage->mZoomRectRaw.width() == (int)mImage->mWidth && mImage->mZoomRectRaw.height() == (int)mImage->mHeight)
        ensureAspectRatio();
}

void QMpxFrame::ensureAspectRatio()
{
    if (!mKeepAspectRatio)
        return;
    double dw = mImage->mZoomRectRaw.width();
    double dh = mImage->mZoomRectRaw.height();
    if (dw == 0 || dh == 0)
        return;
    double w = this->width() - (mAxisY.isVisible() ? mAxisY.width() : 0);
    double h = this->height() - (mAxisX.isVisible() ? mAxisX.height() : 0) - (mColorBar.isVisible() ? mColorBar.height() : 0);
    double ver = dh/h;
    double hor = dw/w;

    if (hor > ver){
        double factor = ver / hor;
        double newheight = h * factor;
        unsigned fillHeight = (h - round(newheight))/2;
        mFillTop->setMinimumHeight(0);
        mFillBottom->setMinimumHeight(0);
        mFillLeft->setMinimumWidth(0);
        mFillRight->setMinimumWidth(0);
        mFillLeft->setMinimumWidth(0);
        mFillRight->setMinimumWidth(0);
        mImage->mScrollArea->resize(w, newheight);
        mFillTop->setMinimumHeight(fillHeight);
        mFillBottom->setMinimumHeight(fillHeight);
    }else{
        double factor = hor/ver;
        double newwidth = w*factor;
        unsigned fillWidth = (w - round(newwidth))/2;
        mFillLeft->setMinimumWidth(0);
        mFillRight->setMinimumWidth(0);
        mFillTop->setMinimumHeight(0);
        mFillBottom->setMinimumHeight(0);
        mImage->mScrollArea->resize(newwidth, h);
        mFillLeft->setMinimumWidth(fillWidth);
        mFillRight->setMinimumWidth(fillWidth);
    }
}

void QMpxFrame::onZoomRectChanged(QRect zoomRect)
{
    if (zoomRect.width() < 0 || zoomRect.height() < 0 || zoomRect.x() < 0 || zoomRect.y() < 0)
        return;

    mZoomRect = zoomRect;
    zoomRect.adjust(1,1,1,1);
    int rotation = mMirrored ? (mRotation == 0 ? -360 : -mRotation) : mRotation;
    switch(rotation){
        case 0:
            mAxisX.setRange(zoomRect.left(), zoomRect.right());
            mAxisY.setRange(zoomRect.bottom(), zoomRect.top());
            break;
        case 90:
            mAxisX.setRange(zoomRect.bottom(), zoomRect.top());
            mAxisY.setRange(zoomRect.right(), zoomRect.left());
            break;
        case 180:
            mAxisX.setRange(zoomRect.right(), zoomRect.left());
            mAxisY.setRange(zoomRect.top(), zoomRect.bottom());
            break;
        case 270:
            mAxisX.setRange(zoomRect.top(), zoomRect.bottom());
            mAxisY.setRange(zoomRect.left(), zoomRect.right());
            break;
        case -90:
            mAxisX.setRange(zoomRect.bottom(), zoomRect.top());
            mAxisY.setRange(zoomRect.left(), zoomRect.right());
            break;
        case -180:
            mAxisX.setRange(zoomRect.left(), zoomRect.right());
            mAxisY.setRange(zoomRect.top(), zoomRect.bottom());
            break;
        case -270:
            mAxisX.setRange(zoomRect.top(), zoomRect.bottom());
            mAxisY.setRange(zoomRect.right(), zoomRect.left());
            break;
        case -360:
            mAxisX.setRange(zoomRect.right(), zoomRect.left());
            mAxisY.setRange(zoomRect.bottom(), zoomRect.top());
            break;
    }
    emit zoomRectChanged(mZoomRect);
}

void QMpxFrame::setZoomRect(QRect zoomRect)
{
    Q_UNUSED(zoomRect)
    mImage->setZoomRectRaw(zoomRect);
}

void QMpxFrame::zoomOut()
{
    mImage->mouseDoubleClickEvent(NULL);
}

void QMpxFrame::setRange(double minRange, double maxRange)
{
    mImage->setRange(minRange, maxRange);
    mColorBar.setRange(minRange, maxRange);
}


void QMpxFrame::setColorMap(ColorMap::DefaultColorMap cm)
{
    ColorMap* map = mImage->getColorMap();
    map->setColorMap(cm);
    mColorBar.setColorMap(map);
    mColorBar.update();
    mImage->redraw();
}

void QMpxFrame::keepAspectRatio(bool keep)
{
    mKeepAspectRatio = keep;
    if (!keep){
        mFillTop->setMinimumHeight(0);
        mFillBottom->setMinimumHeight(0);
        mFillLeft->setMinimumWidth(0);
        mFillRight->setMinimumWidth(0);
    }
    resizeEvent(NULL);
}

void QMpxFrame::setRotation(int rotation)
{
    mRotation = (180 + rotation) % 360;
    if (mRotation == 0 || mRotation == 180){
        mAxisX.setName("X (column number)");
        mAxisY.setName("Y");
    }else{
        mAxisX.setName("Y (row number)");
        mAxisY.setName("X");
    }
    mImage->setRotation(mRotation);
    mImage->redraw();
}

void QMpxFrame::setMirrored(bool mirrored)
{
    mMirrored = !mirrored;
    mImage->setMirrored(mMirrored);
    mImage->redraw();
}

int QMpxFrame::saveToFile(const char* fileName, unsigned width, unsigned height, SizeType sizeType, bool showColorBar, bool showAxisX, bool showAxisY)
{
    unsigned offset = 2;
    unsigned colorbarHeight = mColorBar.getColorbarHeight();
    unsigned yoffset = !showAxisX && showColorBar ? 2*offset : 0;
    unsigned xoffset = showAxisY ? mAxisY.getAxisSize().width() : 0;
    if (showColorBar) yoffset += colorbarHeight;
    if (showAxisX) yoffset += mAxisX.getAxisSize().height();
    if (sizeType == DATA){
        width += xoffset + 2;
        height += yoffset + 2;
    }

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter;
    painter.begin(&image);
    if (!showAxisX && !showAxisY && !showColorBar){
        painter.drawImage(QRect(0, 0, width, height), mImage->mImage);
    }else{
        QRect rect(xoffset+1, 1, width-xoffset-2, height-yoffset-2);
        painter.drawRect(rect.adjusted(-1,-1,0,0));
        painter.drawImage(rect, mImage->mImage);

        if (showColorBar)
            mColorBar.paint(&painter, xoffset, height-colorbarHeight-offset, width-xoffset);
        if (showAxisX)
            mAxisX.paint(&painter, xoffset, height - yoffset , width - xoffset, height);
        if (showAxisY)
            mAxisY.paint(&painter, 0, 0, 40, height - yoffset);
    }
    painter.end();
    return image.save(fileName);
}

void QMpxFrame::paintToPainter(QPainter* painter)
{
    painter->drawImage(QRect(0, 0, mImage->mWidth, mImage->mHeight), mImage->mImage);
}


//#define FOLDINGEND }




