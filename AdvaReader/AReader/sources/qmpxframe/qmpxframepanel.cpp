/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      qmpxframepanel.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-08-03
 *
 */
#include "qmpxframepanel.h"
#include <QGridLayout>
#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <QFileDialog>
#include <QDebug>
#include <QFile>
#include <QMenu>
#include <sstream>

#define AR_MINMAX          1
#define AR_MINMAX_NONZERO  2
#define AR_FRACT_001       3
#define AR_FRACT_005       4
#define AR_FRACT_01        5
#define AR_FRACT_05        6


QMpxFramePanel::QMpxFramePanel(QWidget* parent)
    : QWidget(parent)
    , mMinVal(0)
    , mMaxVal(1)
    , mDataMin(0)
    , mDataMax(0)
    , mAutoRangeIndex(0)
    , mRotation(0)
{

    setupGui();
}

QMpxFramePanel::~QMpxFramePanel()
{

}

void QMpxFramePanel::setupGui()
{
    QGridLayout* layout = new QGridLayout();
    mFrame = new QMpxFrame();
    mEdMin = new QLineEdit("0");
    mEdMax = new QLineEdit("1");
    mFrame->showAxis(false);
    mEdMin->setMinimumWidth(70);
    mEdMax->setMinimumWidth(70);
    mCmbAutoRange = new QComboBox();
    mCmbAutoRange->addItem("None");
    mCmbAutoRange->addItem("Min-Max");
    mCmbAutoRange->addItem("Min-Max Non Zero");
    mCmbAutoRange->addItem("0.001 - 0.999 fractile");
    mCmbAutoRange->addItem("0.005 - 0.995 fractile");
    mCmbAutoRange->addItem("0.01 - 0.99 fractile");
    mCmbAutoRange->addItem("0.05 - 0.95 fractile");
    mCmbAutoRange->setMinimumWidth(80);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QVBoxLayout* hlayout = new QVBoxLayout();
    QToolBar* toolbar = createToolbar();
    hlayout->addWidget(toolbar, 0, 0);
    hlayout->addWidget(mFrame, 1, 0);
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    hlayout->setStretch(1, 1);
    layout->addLayout(hlayout, 0, 0, 1, 7);
    this->setLayout(layout);

    connect(mEdMin, SIGNAL(textChanged(const QString)), this, SLOT(onMinEditChanged(const QString)));
    connect(mEdMax, SIGNAL(textChanged(const QString)), this, SLOT(onMaxEditChanged(const QString)));
    connect(mFrame, SIGNAL(zoomRectChanged(QRect)), this, SLOT(onFrameZoomRectChanged(QRect)));
    connect(mFrame, SIGNAL(selectionRectsChanged(QList<QRect>)), this, SLOT(onFrameSelectionRectsChanged(QList<QRect>)));
}

static QAction* createAction(const QIcon& icon, QString text, QMenu* menu, QActionGroup* group, int index)
{
    QAction* ac = menu->addAction(icon, text);
    ac->setCheckable(true);
    ac->setActionGroup(group);
    ac->setData(index);
    return ac;
}

static QAction* createActionNoIcon(const QString text, QMenu* menu, QActionGroup* group, int index)
{
    QAction* ac = menu->addAction(text);
    ac->setCheckable(true);
    ac->setActionGroup(group);
    ac->setData(index);
    return ac;
}

QToolBar* QMpxFramePanel::createToolbar()
{
    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize(QSize(16, 16));

    QAction* acSave = new QAction(QIcon(":/qmpxframepanel/save"), "Save...", 0);
    QAction* acGrid = new QAction(QIcon(":/qmpxframepanel/grid"), "Show Grid", 0);
    QAction* acRotate = new QAction(QIcon(":/qmpxframepanel/rotdata"), "Rotate clockwise", 0);
    QAction* acUnder = new QAction(QIcon(":/qmpxframepanel/underwarn"), "Under warning", 0);
    QAction* acOver = new QAction(QIcon(":/qmpxframepanel/overwarn"), "Over warning", 0);
    acUnder->setCheckable(true);
    acOver->setCheckable(true);
    acGrid->setCheckable(true);
    connect(acSave, SIGNAL(triggered(bool)), this, SLOT(onAcSaveTriggered(bool)));
    connect(acGrid, SIGNAL(triggered(bool)), this, SLOT(onAcGridTriggered(bool)));
    connect(acRotate, SIGNAL(triggered(bool)), this, SLOT(onAcRotateTriggered(bool)));
    connect(acUnder, SIGNAL(triggered(bool)), this, SLOT(onAcUnderTriggered(bool)));
    connect(acOver, SIGNAL(triggered(bool)), this, SLOT(onAcOverTriggered(bool)));

    // create color map toolbar button
    QMenu* cmMenu = new QMenu(this);
    QToolButton* cmButton = new QToolButton();
    QActionGroup* cmGroup = new QActionGroup(this);
    cmButton->setIcon(QIcon(":/qmpxframepanel/cm_color"));
    cmButton->setPopupMode(QToolButton::InstantPopup);
    createAction(QIcon(":/qmpxframepanel/cm_gray"), "Gray", cmMenu, cmGroup, 0)->setChecked(true);
    createAction(QIcon(":/qmpxframepanel/cm_jet"), "Jet", cmMenu, cmGroup, 1);
    createAction(QIcon(":/qmpxframepanel/cm_jet"), "Jet White", cmMenu, cmGroup, 2);
    createAction(QIcon(":/qmpxframepanel/cm_hot"), "Hot", cmMenu, cmGroup, 3);
    createAction(QIcon(":/qmpxframepanel/cm_cool"), "Cool", cmMenu, cmGroup, 4);
    createAction(QIcon(":/qmpxframepanel/cm_hsl"), "HSL", cmMenu, cmGroup, 5);
    createAction(QIcon(":/qmpxframepanel/cm_cool"), "Cool Warm", cmMenu, cmGroup, 6);
    createAction(QIcon(":/qmpxframepanel/cm_invgray"), "Inverted Gray", cmMenu, cmGroup, 7);
    cmButton->setMenu(cmMenu);
    connect(cmGroup, SIGNAL(triggered(QAction*)), this, SLOT(onCmMenuTriggered(QAction*)));

    // create auto range
    QMenu* arMenu = new QMenu(this);
    QToolButton* arButton = new QToolButton();
    QActionGroup* arGroup = new QActionGroup(this);
    arButton->setIcon(QIcon(":/qmpxframepanel/magnet"));
    arButton->setPopupMode(QToolButton::InstantPopup);
    createActionNoIcon("None", arMenu, arGroup, 0)->setChecked(true);
    createActionNoIcon("Min-Max", arMenu, arGroup, 1);
    createActionNoIcon("Min-Max Non Zero", arMenu, arGroup, 2);
    createActionNoIcon("0.001 - 0.999 Fractile", arMenu, arGroup, 3);
    createActionNoIcon("0.005 - 0.995 Fractile", arMenu, arGroup, 4);
    createActionNoIcon("0.01 - 0.99 Fractile", arMenu, arGroup, 5);
    createActionNoIcon("0.05 - 0.95 Fractile", arMenu, arGroup, 6);
    arButton->setMenu(arMenu);
    connect(arGroup, SIGNAL(triggered(QAction*)), this, SLOT(onArMenuTriggered(QAction*)));

    toolbar->addAction(acSave);
    toolbar->addAction(acGrid);
    toolbar->addAction(acRotate);
    toolbar->addWidget(cmButton);
    toolbar->addAction(acUnder);
    toolbar->addAction(acOver);
    toolbar->addSeparator();
    toolbar->addWidget(mEdMin = new QLineEdit("0"));
    toolbar->addWidget(new QLabel(" - "));
    toolbar->addWidget(mEdMax = new QLineEdit("1"));
    toolbar->addWidget(arButton);
    return toolbar;
}

void QMpxFramePanel::onMinEditChanged(const QString &text)
{
    bool ok = false;
    double val = text.toDouble(&ok);
    if (!ok)
        return;
    mMinVal = val;
    updateRange();
}

void QMpxFramePanel::onMaxEditChanged(const QString &text)
{
    bool ok = false;
    double val = text.toDouble(&ok);
    if (!ok)
        return;
    mMaxVal = val;
    updateRange();
}

void QMpxFramePanel::onFrameZoomRectChanged(QRect zoomRect)
{
    mFrameZoomRect = zoomRect;
    setAutoRange(-1);
}

void QMpxFramePanel::onFrameSelectionRectsChanged(QList<QRect> rects)
{
    if (!mSelRectAutoRange)
        return;
    mFrameSelectsRect = rects;
    setAutoRange(-1);
}

void QMpxFramePanel::onCmMenuTriggered(QAction* action)
{
    mFrame->setColorMap((ColorMap::DefaultColorMap)action->data().toInt());
}

void QMpxFramePanel::onArMenuTriggered(QAction* action)
{
    setAutoRange(action->data().toInt());
}

void QMpxFramePanel::onAcSaveTriggered(bool triggered)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "ASCII Matrix (*.txt);;PNG Image (*.png)");
    if (fileName.isEmpty())
        return;
    if (fileName.endsWith("png")){
        mFrame->saveToFile(fileName.toUtf8().constData(), (unsigned)mFrame->dataWidth(), (unsigned)mFrame->dataHeight(), QMpxFrame::SizeType::DATA, false, false, false);
        return;
    }

    QFile f(fileName);
    f.open(QIODevice::WriteOnly);
    if(!f.isOpen()){
        qDebug() << "Cannot open file: "<< fileName;
        return;
    }

    QTextStream outStream(&f);
    double* data = mFrame->data();
    for (int y = 0; y < (int)mFrame->dataHeight(); y++){
        for (int x = 0; x < (int)mFrame->dataWidth(); x++){
            outStream <<  data[y * (int)mFrame->dataHeight() + x] << " ";
        }
        outStream << "\n";
    }
    f.close();
}

void QMpxFramePanel::onAcGridTriggered(bool triggered)
{
    mFrame->showGrid(triggered);
}

void QMpxFramePanel::onAcUnderTriggered(bool triggered)
{
    mFrame->setUnderWarning(triggered);
}

void QMpxFramePanel::onAcOverTriggered(bool triggered)
{
    mFrame->setOverWarning(triggered);
}

void QMpxFramePanel::onAcRotateTriggered(bool triggered)
{
    mRotation += 90;
    if (mRotation > 270)
        mRotation = 0;
    mFrame->setRotation(mRotation);
}

void QMpxFramePanel::setAutoRange(int index)
{
    if (index == -1)
        index = mAutoRangeIndex;
    mAutoRangeIndex = index;
    if (index == 0)
        return;

    QRect rect = mFrameSelectsRect.isEmpty() ? mFrameZoomRect : mFrameSelectsRect[mFrameSelectsRect.size() - 1];

    switch(index){
        case AR_MINMAX: setMinMax(mDataMin, mDataMax, true); break;
        case AR_MINMAX_NONZERO: setMinMax(mDataMinNZ, mDataMaxNZ, true); break;
        case AR_FRACT_001: setMinMax(fractile(0.001, rect), fractile(0.999, rect), true); break;
        case AR_FRACT_005: setMinMax(fractile(0.005, rect), fractile(0.995, rect), true); break;
        case AR_FRACT_01: setMinMax(fractile(0.01, rect), fractile(0.99, rect), true); break;
        case AR_FRACT_05: setMinMax(fractile(0.05, rect), fractile(0.95, rect), true); break;
    }
}

void QMpxFramePanel::updateRange()
{
    double minVal = mMinVal;
    double maxVal = mMaxVal;
    if (maxVal < minVal || maxVal == minVal){
        minVal = maxVal;
        maxVal = minVal + 1;
    }

    emit rangeChanged(minVal, maxVal);
    if (mFrame)
        mFrame->setRange(minVal, maxVal);
}

std::string QMpxFramePanel::doubleToStringNice(double number)
{
    std::ostringstream buff;
    buff.precision(5);
    if (!(buff << number))
        return std::string("");
    return buff.str();
}

void QMpxFramePanel::setMinMax(double minVal, double maxVal, bool emitSignal)
{
    mMinVal = minVal;
    mMaxVal = maxVal;
    if (!mEdMin->hasFocus())
        mEdMin->setText(doubleToStringNice(minVal).c_str());
    if (!mEdMax->hasFocus())
        mEdMax->setText(doubleToStringNice(maxVal).c_str());
    if (emitSignal){
        emit rangeChanged(mMinVal, mMaxVal);
        if (mFrame)
            mFrame->setRange(mMinVal, mMaxVal);
    }
}

void QMpxFramePanel::calculateFractileData(int size, const QRect& rect) const
{
    mFractileData.clear();
    mFractileData.reserve(size);
    double* data = mFrame->data();
    int dataWidth = static_cast<int>(mFrame->dataWidth());
    int dataSize = static_cast<int>(mFrame->dataWidth() * mFrame->dataHeight());

    if (rect.isEmpty()){
        for (int i = 0; i < dataSize; i+= (int)dataSize / size)
            mFractileData.append(data[i]);
    }else{
        int step = rect.width() * rect.height() / size;
        if (step <= 0)
            step = 1;

        for (int y = rect.y(); y < rect.bottom(); y += step)
            for (int x = rect.x(); x < rect.right(); x += step)
                mFractileData.append(data[y * dataWidth + x]);
    }
    qSort(mFractileData);
}

double QMpxFramePanel::fractile(double fract, const QRect& rect) const
{
    if (mFractileData.isEmpty() || mFractileRect != rect){
        calculateFractileData(4000, rect);
        mFractileRect = rect;
    }

    if (mFractileData.isEmpty()){
        if (fract < 0.5)
            return mDataMin;
        else
            return mDataMax;
    }

    int index = (int)((double)mFractileData.length() * fract + 0.5);
    if (index < 0)
        return mFractileData[0];
    if (index >= mFractileData.length())
        return mFractileData[mFractileData.length()-1];
    return mFractileData[index];
}

