/**
 * Copyright (C) 2016 Daniel Turecek
 *
 * @file      qmpxframepanel.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2016-08-03
 *
 */
#ifndef QMPXFRAMEPANEL_H
#define QMPXFRAMEPANEL_H
#include <QObject>
#include <QWidget>
#include <QList>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "qmpxframe.h"

class QToolBar;

class QMpxFramePanel : public QWidget
{
    Q_OBJECT

public:
    QMpxFramePanel(QWidget* parent = 0);
    virtual ~QMpxFramePanel();

    QMpxFrame* mpxFrame() { return mFrame; }

    void setRange(double minVal, double maxVal){ setMinMax(minVal, maxVal, false); }
    double getMin() { return mMinVal; }
    double getMax() { return mMaxVal; }
    void setAutoRange(int index);
    void setAutoRangeWithSelectionRectEnabled(bool enabled) { mSelRectAutoRange = enabled; if (!enabled) mFrameSelectsRect.clear(); }

    template <typename T> void setData(T* data, unsigned width, unsigned height) {
        mFrame->setData(data, width, height);
        mEdMin->setToolTip(QString::number(mDataMin));
        mEdMax->setToolTip(QString::number(mDataMax));
        mDataMin = mDataMinNZ = 1e300;
        mDataMax = mDataMaxNZ = -1e300;
        for (unsigned i = 0; i < width * height; i++){
            if (data[i] < mDataMin) mDataMin = data[i];
            if (data[i] > mDataMax) mDataMax = data[i];
            if (data[i] != 0 && data[i] < mDataMinNZ) mDataMinNZ = data[i];
            if (data[i] != 0 && data[i] > mDataMaxNZ) mDataMaxNZ = data[i];
        }
        mFractileData.clear();
        mFractileRect = QRect();
        setAutoRange(-1);
    }

    virtual QSize sizeHint() const { return QSize(400, 400); }

signals:
    void rangeChanged(double minRange, double maxRange);

protected slots:
    void onMinEditChanged(const QString &text);
    void onMaxEditChanged(const QString &text);
    void onFrameZoomRectChanged(QRect zoomRect);
    void onFrameSelectionRectsChanged(QList<QRect> rects);
    void onCmMenuTriggered(QAction* action);
    void onArMenuTriggered(QAction* action);
    void onAcSaveTriggered(bool triggered);
    void onAcGridTriggered(bool triggered);
    void onAcUnderTriggered(bool triggered);
    void onAcOverTriggered(bool triggered);
    void onAcRotateTriggered(bool triggered);

protected:
    void setupGui();
    void updateRange();
    std::string doubleToStringNice(double number);
    void setMinMax(double minVal, double maxVal, bool emitSignal);
    void calculateFractileData(int size, const QRect& rect) const;
    double fractile(double fract, const QRect& rect) const;
    QToolBar* createToolbar();

protected:
    QMpxFrame* mFrame;
    QLineEdit* mEdMin;
    QLineEdit* mEdMax;
    QComboBox* mCmbAutoRange;
    double mMinVal;
    double mMaxVal;
    double mDataMin;
    double mDataMax;
    double mDataMinNZ;
    double mDataMaxNZ;
    bool mChanging;
    int mAutoRangeIndex;
    int mRotation;
    bool mSelRectAutoRange;
    mutable QList<double> mFractileData;
    mutable QRect mFractileRect;
    QList<QRect> mFrameSelectsRect;
    QRect mFrameZoomRect;
};

#endif /* !QMPXFRAMEPANEL_H */

