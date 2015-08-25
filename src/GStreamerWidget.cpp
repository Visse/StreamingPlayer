#include "GStreamerWidget.h"
#include "Feed.h"
#include "VideoProvider.h"
#include "GStreamerPipeline.h"

#include "ui_VideoPlayer.h"
#include "ui_CreateCustomFormatDialog.h"


#include <qtimer.h>
#include <qmenu.h>
#include <qtooltip.h>
#include <qevent.h>
#include <qsignalmapper.h>
#include <qwindow.h>
#include <qmessagebox.h>

class OverlayEventFilter :
    public QObject
{
    Q_OBJECT
public:
    QTimer *timer;

    QWidget *overlay,
            *widget;
    bool mOverOverlay = false;
public:
    OverlayEventFilter( QObject *parent ) :
            QObject(parent)
    {}

    virtual bool eventFilter( QObject *object, QEvent *event ) override
    {
        if( object == overlay ) {
            if( event->type() == QEvent::Enter ) {
                timer->stop();
                mOverOverlay = true;
            }
            else if( event->type() == QEvent::Leave ) {
                timer->start();
                mOverOverlay = false;
            }
        }
        if( object == widget ) {
            if( mOverOverlay == false && event->type() == QEvent::MouseMove ) {
                timer->start();
                overlay->show();

                widget->setCursor( Qt::ArrowCursor );
            }
        }
        return QObject::eventFilter( object, event );
    }

public slots:
    void timerTriggerd() {
        widget->setCursor( Qt::BlankCursor );
    }
};

GStreamerWidget::GStreamerWidget( QWidget *parent ) :
    QWidget(parent)
{
    Ui::VideoPlayer ui;
    ui.setupUi( this );
    mWidget = ui.widget;
    mOverlay = ui.overlay;
    mVideoWidget = ui.video;
    
    mPositionSlider = ui.timeSlider;
    
    mPositionTimer = new QTimer( this );
    mPositionTimer->setInterval( 100 );
    connect( mPositionTimer, SIGNAL(timeout()), SLOT(onPositionUpdate()) );

    connect( ui.playPauseButton, SIGNAL(clicked()), SLOT(tooglePlayPause()) );
     {
        QMenu *menu = new QMenu;
        QMenu *mode = menu->addMenu( "Mode" );

        QActionGroup *modeGroup = new QActionGroup( mode );

        mModeNormalAction = modeGroup->addAction("Normal");
        mModeVisualAction = modeGroup->addAction("Visualisation");
        mModeAudioOnlyAction = modeGroup->addAction("Audio only");
       
        mModeNormalAction->setCheckable(true);
        mModeVisualAction->setCheckable(true);
        mModeAudioOnlyAction->setCheckable(true);

        mode->addAction( mModeNormalAction );
        mode->addAction( mModeVisualAction );
        mode->addAction( mModeAudioOnlyAction );

        connect( mModeNormalAction, SIGNAL(triggered()), SLOT(modeNormal()) );
        connect( mModeVisualAction, SIGNAL(triggered()), SLOT(modeVisual()) );
        connect( mModeAudioOnlyAction, SIGNAL(triggered()), SLOT(modeAudioOnly()) );

        ui.settings->setMenu( menu );

        mFormatMenu = menu->addMenu("Formats");

        mFullscreenAction = menu->addAction( "Fullscreen" );
        mFullscreenAction->setCheckable( true );
        connect( mFullscreenAction, SIGNAL(triggered()), SLOT(toogleFullscreen()) );
    }
    connect( mPositionSlider, SIGNAL(sliderPressed()), SLOT(positionSliderPressed()) );
    connect( mPositionSlider, SIGNAL(sliderReleased()), SLOT(positionSliderReleased()) );
    connect( mPositionSlider, SIGNAL(valueChanged(int)), SLOT(positionSliderMoved()) );
    
    connect( this, SIGNAL(bufferingStarted()), ui.bufferProgress, SLOT(show()) );
    connect( this, SIGNAL(bufferingEnded()), ui.bufferProgress, SLOT(hide()) );
    connect( this, SIGNAL(bufferingProgress(int)), ui.bufferProgress, SLOT(setValue(int)) );
    ui.bufferProgress->hide();

    mOverlayTimer = new QTimer( this );
    mOverlayTimer->setSingleShot( true );
    mOverlayTimer->setInterval( 1000 );

    OverlayEventFilter *filter = new OverlayEventFilter( mOverlay );
    filter->timer = mOverlayTimer;
    filter->overlay = mOverlay;
    filter->widget = mWidget;

    mOverlay->installEventFilter( filter );
    mWidget->installEventFilter( filter );

    connect( mOverlayTimer, SIGNAL(timeout()), mOverlay, SLOT(hide()) );
    connect( mOverlayTimer, SIGNAL(timeout()), filter, SLOT(timerTriggerd()) );

    mOverlay->hide();
    modeNormal();
}   

GStreamerWidget::~GStreamerWidget()
{
    mWidget->deleteLater();
}

QTime GStreamerWidget::position()
{
    /// @todo
    return QTime();
}

QTime GStreamerWidget::lenght()
{
    /// @todo
    return QTime();
}

bool GStreamerWidget::isPlaying()
{
    /// @todo
    return false;
}

void GStreamerWidget::setEntry( QSharedPointer<Entry> entry )
{
    disconnect( this, SLOT(entryChanged()) );
    mEntry = entry;
    connect( mEntry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
    entryChanged();
}

void GStreamerWidget::setVideoProvider( QSharedPointer<VideoProvider> provider )
{
    stop();
    disconnect( this, SLOT(videoProviderChanged()) );
    mProvider = provider;

    connect( mProvider.data(), SIGNAL(changed()), SLOT(videoProviderChanged()) );
    videoProviderChanged();
}

void GStreamerWidget::play()
{
    if( mPipeline ) {
        mPipeline->play();
    }
}

void GStreamerWidget::pause()
{
    if( mPipeline ) {
        mPipeline->pause();
    }
}

void GStreamerWidget::stop()
{
    mVideoWidget->stopPipelineWatch();
    destroyPipeline();
    mHaveVideoSource = false;
}

void GStreamerWidget::tooglePlayPause()
{
    if( mPipeline && mPipeline->getState() == PS_Playing ) {
        pause();
    }
    else {
        play();
    }
}

void GStreamerWidget::seek( QTime time )
{
    if( mPipeline ) {
        mPipeline->seek( time );
    }
}

void GStreamerWidget::modeNormal()
{
    mMode = PM_Normal;
    if( mPipeline ) {
        mPipeline->enableVideo();
    }
}

void GStreamerWidget::modeVisual()
{
    mMode = PM_Visual;
    if( mPipeline ) {
        mPipeline->enableVideo();
    }
}

void GStreamerWidget::modeAudioOnly()
{
    mMode = PM_AudioOnly;
    if( mPipeline ) {
        mPipeline->disableVideo();
    }
}

void GStreamerWidget::setFormat( QString formatId )
{
    if( !formatIsAboutToChange() ) return;

    QList<Format> formats = mProvider->getFormats();
    for( const Format &format : formats ) {
        if( format.id == formatId ) {
            StreamType type;
            switch ( format.type ) {
            case( VT_Normal ):
                type = ST_Video | ST_Audio;
                break;
            case( VT_AudioOnly ):
                type = ST_Audio;
                break;
            case( VT_VideoOnly ):
                type = ST_Video;
                break;
            }

            mPipeline->addStream( format.url, type );

            formatChanged( formatId, false );
            return;
        }
    }
}

void GStreamerWidget::setCustomFormat( QString formatTitle )
{
    if( !formatIsAboutToChange() ) return;
    /// @todo
}


void GStreamerWidget::toogleFullscreen()
{
    if( mFullscreen ) {
        noFullscreen();
    }
    else {
        fullscreen();
    }
}

void GStreamerWidget::fullscreen()
{
    if( !mFullscreen )
    {
        mFullscreenAction->setChecked( true );
        
        layout()->removeWidget( mWidget );

        mWidget->setParent( 0, Qt::FramelessWindowHint );
        mWidget->show();

        // make sure we fullscreen in the same screen as the player is in
        mWidget->window()->windowHandle()->setScreen(
            windowHandle()->screen()  
        );

        mWidget->showFullScreen();
        mFullscreen = true;
    }
}

void GStreamerWidget::noFullscreen()
{
    if( mFullscreen )
    {
        mWidget->showNormal();
        mWidget->setParent( this );
        mWidget->show();

        layout()->addWidget( mWidget );

        mFullscreenAction->setChecked( false );

        mFullscreen = false;
    }
}

void GStreamerWidget::showCreateCustomFormat()
{
    QDialog *dialog = new QDialog( mWidget );

    Ui::CreateCustomFormatDialog ui;
    ui.setupUi( dialog );

    dialog->show();
}

void GStreamerWidget::onPositionUpdate()
{
    Q_ASSERT( mPipeline );
 
    int lenght = -mPipeline->getLenght().msecsTo( QTime(0,0) );
    int position = -mPipeline->getPosition().msecsTo( QTime(0,0) );
    mPositionSlider->setMaximum( lenght );
    mPositionSlider->setValue( position );
}

void GStreamerWidget::entryChanged()
{
    Q_ASSERT( mEntry );
    QSharedPointer<VideoProvider> provider = qvariant_cast<QSharedPointer<VideoProvider>>(mEntry->properties["videoProvider"]);
    
    if( provider && provider != mProvider ) {
        setVideoProvider( provider );
    }
}

void GStreamerWidget::videoProviderChanged()
{
    populateFormatMenu();

    if( mProvider && !mHaveVideoSource ) {
        Format format;
        bool found = findFormatForMode(mMode, format);
        if( !found ) {
            found = findFormatForMode( PM_Normal, format );
        }

        if( found ) {
            setFormat( format.id );
        }
    }
}

void GStreamerWidget::positionSliderMoved()
{
    if( mUserSeeks ) {
        int pos = mPositionSlider->value();
        QTime time = QTime::fromMSecsSinceStartOfDay( pos*10 );
        seek( time );
    }
}

void GStreamerWidget::positionSliderPressed()
{
    mUserSeeks = true;
    internalPause();
}

void GStreamerWidget::positionSliderReleased()
{
    mUserSeeks = false;

    int pos = mPositionSlider->value();
    QTime time = QTime::fromMSecsSinceStartOfDay( pos*10 );
    seek( time );

    internalResume();
}

void GStreamerWidget::formatSelected( QObject *format )
{
    QAction *action = qobject_cast<QAction*>( format );
    QVariantMap data = action->data().toMap();

    if( data["custom"].toBool() ) {
        setCustomFormat( data["id"].toString() );
    }
    else {
        setFormat( data["id"].toString() );
    }
}

void GStreamerWidget::onBuffering( int percent )
{
    if( !mIsBuffering ) bufferingStarted();
    mIsBuffering = true;

    bufferingProgress( percent );

    if( percent == 100 ) {
        mIsBuffering = false;
        bufferingEnded();
    }
}

bool GStreamerWidget::findFormatForMode( PlayerMode mode, Format &format )
{
    bool found = false;
    
    QList<Format> formats = mProvider->getFormats();
    
    for( const Format &f : formats ) {
        if( mUserChooseFormat && f.id == mCurrentFormatId ) {
            format = f;
            return true;
        }

        switch ( mode )
        {
        case( PM_AudioOnly ):
        case( PM_Visual ):
            if( f.type == VT_AudioOnly ) {
                if( found == false || format.bitrate < f.bitrate ) {
                    format = f;
                    found = true;
                }
            }
            break;
        case( PM_Normal ):
            if( f.type == VT_Normal ) {
                if( found == false || format.width < f.width ) {
                    format = f;
                    found = true;
                }
            }
            break;
        }
    }
    
    return found;
}

void GStreamerWidget::internalPause()
{
    if( mInternalPaused ) return;
    
    /// @todo
}

void GStreamerWidget::internalResume()
{
    if( !mInternalPaused ) return;
    if( mIsBuffering || mUserSeeks ) return;

    /// @todo
}

void GStreamerWidget::populateFormatMenu()
{
    delete mFormatGroup;
    delete mFormatMapper;

    mFormatMenu->clear();

    mFormatGroup = new QActionGroup( this );
    mFormatMapper = new QSignalMapper( this );
    mFormatGroup->setExclusive( true );
    mFormatIdToAction.clear();;
    
    connect( mFormatMapper, SIGNAL(mapped(QObject*)), SLOT(formatSelected(QObject*)) );
    if( !mProvider ) return;

    QList<Format> formatList = mProvider->getFormats();
    
    for( const Format &f : formatList ) {
        QAction *action = mFormatGroup->addAction(f.title);
        action->setCheckable( true );
        mFormatMenu->addAction( action );

        QVariantMap data;
            data["custom"] = false;
            data["id"] = f.id;
        action->setData( data );
        
        connect( action, SIGNAL(triggered()), mFormatMapper, SLOT(map()) );
        mFormatMapper->setMapping( action, action );

        mFormatIdToAction.insert( f.id, action );

        if( f.id == mCurrentFormatId ) {
            action->setChecked( true );
        }
    }

    if( !mCustomFormats.isEmpty() ) {
        mFormatMenu->addSeparator();
        for( const CustomFormat &format : mCustomFormats ) {
            QAction *action = mFormatGroup->addAction( format.title );
            action->setCheckable( true );
            mFormatMenu->addAction( action );
            
            QVariantMap data;
                data["custom"] = true;
                data["id"] = format.title;
            action->setData( data );
            
            connect( action, SIGNAL(triggered()), mFormatMapper, SLOT(map()) );
            mFormatMapper->setMapping( action, action );
        }
    }

    mFormatMenu->addSeparator();
    QAction *createCustomFormat = mFormatMenu->addAction( "Create Custom Format" );
    connect( createCustomFormat, SIGNAL(triggered()), SLOT(showCreateCustomFormat()) );
}

bool GStreamerWidget::isProviderSupportingCustomFormat( const CustomFormat &format )
{
    if( !mProvider ) return false;

    const QList<Format> formats = mProvider->getFormats();
    QSet<QString> ids;

    for( const Format &f : formats ) {
        ids.insert( f.id );
    }

    for( const CustomStream &stream : format.streams ) {
        if( !ids.contains(stream.formatId) ) return false;
    }
    return true;
}

bool GStreamerWidget::formatIsAboutToChange()
{
    stop();
    if( !mProvider ) return false;
    createPipeline();
    
    return true;
}

void GStreamerWidget::formatChanged( QString formatId, bool isCustom )
{
    /// @todo option for autoplay
    play();
    // buffering...
    internalPause();

    mPositionSlider->setValue( 0 );
    mPositionSlider->setRange( 0, 0);
    mHaveVideoSource = true;

    auto iter = mFormatIdToAction.find( formatId );
    if( iter != mFormatIdToAction.end() ) {
        (*iter)->setChecked( true );
    }
    mCurrentFormatId = formatId;
    mUserChooseFormat = true;
    mIsCustomFormat = isCustom;

    playbackStarted( mEntry );
}

void GStreamerWidget::createPipeline()
{
    Q_ASSERT( mPipeline == nullptr );

    mPipeline = new GStreamerPipeline( this );
    mVideoWidget->watchPipeline( mPipeline->getPipeline() );

    connect( mPipeline, SIGNAL(buffering(int)), SLOT(onBuffering(int)) );

    mPositionTimer->start();

    if( mMode == PM_AudioOnly ) {
        mPipeline->disableVideo();
    }
}

void GStreamerWidget::destroyPipeline()
{
    mVideoWidget->stopPipelineWatch();

    delete mPipeline;
    mPipeline = nullptr;

    mPositionTimer->stop();
}

#include "GStreamerWidget.moc"