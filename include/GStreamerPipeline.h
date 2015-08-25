#pragma once

#include <qobject.h>

#include <QGst/pipeline.h>
#include <QGst/videooverlay.h>
#include <QGst/pad.h>

enum StreamType : unsigned {
    ST_Audio     = 1 << 0,
    ST_Video     = 1 << 1,
    ST_Subtitles = 1 << 2
};

enum PipelineState {
    PS_Null,
    PS_Paused,
    PS_Playing,
    PS_Buffering,
    PS_Finnished
};

inline StreamType operator | ( StreamType s1, StreamType s2 ) {
    return static_cast<StreamType>( static_cast<unsigned>(s1) | static_cast<unsigned>(s2) );
}


class GStreamerPipeline :
    public QObject
{
    Q_OBJECT
public:
    GStreamerPipeline( QObject *parent );
    ~GStreamerPipeline();

    QGst::PipelinePtr getPipeline() {
        return mPipeline;
    }
    PipelineState getState() {
        return mState;
    }

    QTime getLenght();
    QTime getPosition();

public slots:
    void play();
    void pause();

    void disableVideo();
    void enableVideo();

    void addStream( QString uri, StreamType type );

    void seek( QTime position );

    void expose();
    
signals:
    void stateChanged( PipelineState oldState, PipelineState newState );
    void buffering( int percent );
    void finnishedBuffering();

private slots:
    void sourceSetup( QGst::ElementPtr source );
    void decodedPadAdded( QGst::ElementPtr decoder, QGst::PadPtr pad );
    void handleBusMessage( const QGst::MessagePtr &message );

private:
    struct StreamInfo {
        StreamType type;
        QString uri;
        QGst::ElementPtr decoder;
    };

private:
    void switchToState( PipelineState state );

    void onEos();
    void onError( const QGst::ErrorMessagePtr &msg );
    void onBuffering( const QGst::BufferingMessagePtr &msg );

private:
    QGst::PipelinePtr mPipeline;
    QGst::ElementPtr  mPlaySink;

    QVector<StreamInfo> mStreams;
    
    PipelineState mState = PS_Null;
    bool mPlayAfterBuffering = false;

    QTime mLenght,
          mPosition;
};