#pragma once


#include <QtWidgets/qwidget.h>

#include <QGst/element.h>
#include <QGst/pipeline.h>

class Entry;
class QSlider;
class VideoProvider;
struct Format;
class QSignalMapper;
class QActionGroup;
class QMenu;

class GStreamerWidget :
    public QWidget 
{
    Q_OBJECT
public:
    GStreamerWidget( QWidget *parent );
    ~GStreamerWidget();
    
    QTime position();
    QTime lenght();

    bool isPlaying();

signals:
    void bufferingStarted();
    void bufferingEnded();
    void bufferingProgress( int percentDone );

    void playbackStarted( QSharedPointer<Entry> entry );
    void playbackFinnished();

public slots:
    void setEntry( QSharedPointer<Entry> entry );
    void setVideoProvider( QSharedPointer<VideoProvider> provider );
    void play();
    void pause();
    void stop();
    void tooglePlayPause();

    void seek( QTime time );

    void modeNormal();
    void modeVisual();
    void modeAudioOnly();

    void setFormat( QString formatId );

    void toogleFullscreen();
    void fullscreen();
    void noFullscreen();

private slots:
    void onBusMessage( const QGst::MessagePtr &message );
    void onPositionUpdate();
    void entryChanged();
    void videoProviderChanged();
    void positionSliderMoved();

    void positionSliderPressed();
    void positionSliderReleased();
    
private:
    enum PlayerMode {
        PM_Normal,
        PM_Visual,
        PM_AudioOnly
    };
private:
    void sourceSetup( QGst::ElementPtr source );
    bool findFormatForMode( PlayerMode mode, Format &format );

    void internalPause();
    void internalResume();

    void populateFormatMenu();

private:
    QWidget *mWidget = nullptr;

    QGst::PipelinePtr mPipeline;
    QGst::State mTargetState = QGst::StateNull;

    QTimer *mPositionTimer = nullptr;
    QSlider *mPositionSlider = nullptr;

    QSharedPointer<Entry> mEntry;
    QSharedPointer<VideoProvider> mProvider;

    PlayerMode mMode = PM_Normal;
    QString mPositionSliderTooltip;
    
    QAction *mModeNormalAction = nullptr,
            *mModeVisualAction = nullptr,
            *mModeAudioOnlyAction = nullptr;

    bool mHaveVideoSource = false,
         mIsBuffering = false,
         mUserSeeks = false,
         mInternalPaused = false;


    bool mUserChooseFormat = false;
    QString mCurrentFormatId;
    QMenu *mFormatMenu = nullptr;
    QActionGroup *mFormatGroup = nullptr;
    QSignalMapper *mFormatMapper = nullptr;
    QMap<QString,QAction*> mFormatIdToAction;

    QWidget *mOverlay = nullptr;
    QTimer *mOverlayTimer = nullptr;

    bool mFullscreen = false;
    QAction *mFullscreenAction = nullptr;
};