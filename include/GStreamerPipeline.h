#pragma once

#include <qobject.h>

#include <QGst/pipeline.h>
#include <QGst/videooverlay.h>

class GStreamerPipeline :
    public QObject
{
    Q_OBJECT
public:

    
private slots:


private:
    QGst::PipelinePtr mPipeline;

    QVector<QGst::BinPtr> mDecodeBins;
    QGst::BinPtr mSink;
    QGst::VideoOverlayPtr mVideoSink;
};