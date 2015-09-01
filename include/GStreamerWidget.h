#pragma once


#include <QtWidgets/qwidget.h>
#include <QMap>
#include <QTime>

class Entry;
class QSlider;
class VideoProvider;
struct Format;
class QSignalMapper;
class QActionGroup;
class QMenu;
class GStreamerPipeline;

enum StreamType : unsigned;

namespace QGst {
    namespace Ui {
        class VideoWidget;
    }
}

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
    void setCustomFormat( QString formatTitle );

    void toogleFullscreen();
    void fullscreen();
    void noFullscreen();
    
    void showCreateCustomFormat();

    void createCustomFormat( QString title, QString videoFormat, QString audioFormat );

private slots:
    void onPositionUpdate();
    void entryChanged();
    void videoProviderChanged();
    void positionSliderMoved();

    void positionSliderPressed();
    void positionSliderReleased();

    void formatSelected( QObject *format );

    void onBuffering( int percent );
        

private:
    enum PlayerMode {
        PM_Normal,
        PM_Visual,
        PM_AudioOnly
    };

    struct CustomStream {
        QString formatId;
        StreamType type;
    };
    struct CustomFormat {
        QVector<CustomStream> streams;
        QString title;
    };


private:
    bool findFormatForMode( PlayerMode mode, Format &format );

    void internalPause();
    void internalResume();

    void populateFormatMenu();

    bool isProviderSupportingCustomFormat( const CustomFormat &format );

    bool formatIsAboutToChange();
    void formatChanged( QString formatId, bool isCustom );
        
    void createPipeline();
    void destroyPipeline();
private:
    QWidget *mWidget = nullptr;
    QGst::Ui::VideoWidget *mVideoWidget = nullptr;

    QTimer *mPositionTimer = nullptr;
    QSlider *mPositionSlider = nullptr;

    GStreamerPipeline *mPipeline = nullptr;

    QSharedPointer<Entry> mEntry;
    QSharedPointer<VideoProvider> mProvider;

    PlayerMode mMode = PM_Normal;
    
    QAction *mModeNormalAction = nullptr,
            *mModeVisualAction = nullptr,
            *mModeAudioOnlyAction = nullptr;

    bool mHaveVideoSource = false,
         mIsBuffering = false,
         mUserSeeks = false,
         mInternalPaused = false;

    bool mUserChooseFormat = false,
         mIsCustomFormat = false;

    QString mCurrentFormatId;
    QMenu *mFormatMenu = nullptr;
    QActionGroup *mFormatGroup = nullptr;
    QSignalMapper *mFormatMapper = nullptr;
    QMap<QString,QAction*> mFormatIdToAction;

    QWidget *mOverlay = nullptr;
    QTimer *mOverlayTimer = nullptr;

    bool mFullscreen = false;
    QAction *mFullscreenAction = nullptr;
    
    QVector<CustomFormat> mCustomFormats;
};