#include "GStreamerWidget.h"
#include "Feed.h"
#include "VideoProvider.h"

#include "ui_VideoPlayer.h"

#include <QGst/pipeline.h>
#include <QGst/elementfactory.h>
#include <QGst/message.h>
#include <QGst/bus.h>
#include <QGst/query.h>
#include <QGlib/connect.h>

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

    mPipeline = QGst::ElementFactory::make("playbin").dynamicCast<QGst::Pipeline>();
    Q_ASSERT( mPipeline );
    mPipeline->setProperty( "buffer-duration", QGst::ClockTime::fromSeconds(20) );
    QGlib::connect( mPipeline, "source-setup", this, &GStreamerWidget::sourceSetup );

    ui.video->watchPipeline( mPipeline );


    QGst::BusPtr bus = mPipeline->bus();
    bus->addSignalWatch();
    QGlib::connect(bus, "message", this, &GStreamerWidget::onBusMessage);
    /*
    setAttribute( Qt::WA_NoSystemBackground, true );
    setAttribute( Qt::WA_OpaquePaintEvent, true );
    setWindowFlags(  windowFlags() | Qt::FramelessWindowHint );
    ui.video->setAttribute( Qt::WA_NoSystemBackground, true );
    ui.video->setAttribute( Qt::WA_OpaquePaintEvent, true );
    ui.video->setWindowFlags(  ui.video->windowFlags() | Qt::FramelessWindowHint );
    */
    
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
    mPipeline->setState( QGst::StateNull );
    mWidget->deleteLater();
}

QTime GStreamerWidget::position()
{
    QGst::PositionQueryPtr query = QGst::PositionQuery::create(QGst::FormatTime);
    mPipeline->query( query );
    return QGst::ClockTime(query->position()).toTime();
}

QTime GStreamerWidget::lenght()
{
    QGst::DurationQueryPtr query = QGst::DurationQuery::create(QGst::FormatTime);
    mPipeline->query(query);
    return QGst::ClockTime(query->duration()).toTime();
}

bool GStreamerWidget::isPlaying()
{
    return mTargetState == QGst::StatePlaying;
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
    mPipeline->setState( QGst::StatePlaying );
    mTargetState = QGst::StatePlaying;
    mInternalPaused = false;
}

void GStreamerWidget::pause()
{
    mPipeline->setState( QGst::StatePaused );
    mTargetState = QGst::StatePaused;
    mInternalPaused = false;
}

void GStreamerWidget::stop()
{
    mPipeline->setState( QGst::StateReady );
    mPositionTimer->stop();
    mTargetState = QGst::StateReady;
    mInternalPaused = false;
    mHaveVideoSource = false;
}

void GStreamerWidget::tooglePlayPause()
{
    if( mTargetState == QGst::StatePlaying && !mIsBuffering ) {
        pause();
    }
    else {
        play();
    }
}

void GStreamerWidget::seek( QTime time )
{
    mPipeline->seek( QGst::FormatTime, QGst::SeekFlagAccurate|QGst::SeekFlagFlush, QGst::ClockTime::fromTime(time) );
}

static const int DEFAULT_PLAYBIN_FLAGS = 0x617;

void GStreamerWidget::modeNormal()
{
    mMode = PM_Normal;
    // default flags + visualisation if no video
    mPipeline->setProperty( "flags", DEFAULT_PLAYBIN_FLAGS | 0x8 );
    mUserChooseFormat = false;
    mModeNormalAction->setChecked( true );
}

void GStreamerWidget::modeVisual()
{
    mMode = PM_Visual;
    // default flags + visualisation if no video
    mPipeline->setProperty( "flags", DEFAULT_PLAYBIN_FLAGS | 0x8 );
    mUserChooseFormat = false;
    mModeVisualAction->setChecked( true );
}

void GStreamerWidget::modeAudioOnly()
{
    mMode = PM_AudioOnly;
    // default flags, without vidoe video
    mPipeline->setProperty( "flags", DEFAULT_PLAYBIN_FLAGS & ~0x1 );
    mUserChooseFormat = false;
    mModeAudioOnlyAction->setChecked( true );
}

void GStreamerWidget::setFormat( QString formatId )
{
    stop();
    if( !mProvider ) return;
    QList<Format> formats = mProvider->getFormats();
    for( const Format &format : formats ) {
        if( format.id == formatId ) {
            mPipeline->setProperty( "uri", format.url );
            /// @todo option for autoplay
            play();
            // buffering...
            internalPause();

            mPositionSlider->setValue( 0 );
            mPositionSlider->setRange( 0, 0);
            mHaveVideoSource = true;
            playbackStarted( mEntry );

            auto iter = mFormatIdToAction.find( formatId );
            if( iter != mFormatIdToAction.end() ) {
                (*iter)->setChecked( true );
            }
            mCurrentFormatId = formatId;
            mUserChooseFormat = true;
            return;
        }
    }
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

void GStreamerWidget::onBusMessage( const QGst::MessagePtr &message )
{
    switch (message->type()) {
    case QGst::MessageEos:
        playbackFinnished();
        break;
    case QGst::MessageError: {
        QGst::ErrorMessagePtr errorMsg = message.staticCast<QGst::ErrorMessage>();
        qCritical() << errorMsg->error();
        int ret = QMessageBox::critical( mWidget, "GStreamer Error", QString( "%1: %2").arg(errorMsg->source()->pathString(), errorMsg->error().what()), QMessageBox::Cancel|QMessageBox::Retry, QMessageBox::Retry );
        if( ret == QMessageBox::Retry ) {
            
        }
        stop();
        playbackFinnished();
      } break;
    case QGst::MessageStateChanged:
        if (message->source() == mPipeline ) {
            QGst::StateChangedMessagePtr stateChange = message.staticCast<QGst::StateChangedMessage>();
            if( stateChange->newState() == QGst::StatePlaying ) {
                mPositionTimer->start();
            }
            else {
                mPositionTimer->stop();
            }
        }
        break;
    case( QGst::MessageBuffering ):{
            QGst::BufferingMessagePtr buffering = message.staticCast<QGst::BufferingMessage>();

            if( buffering->percent() == 100 ) {
                qDebug() << "Finnished buffering.";
                mIsBuffering = false;
                bufferingEnded();
                internalResume();
            }
            else {
                if( !mIsBuffering ) {
                    qDebug() << "Buffering " << buffering->percent() << "%";
                    mIsBuffering = true;
                    bufferingStarted();
                    internalPause();
                }
                bufferingProgress( buffering->percent() );
            }
        } break;
    case( QGst::MessageClockLost ):
        qDebug() << "Clock lost!";
        mPipeline->setState( QGst::StatePaused );
        if( mTargetState == QGst::StatePlaying ) {
            mPipeline->setState( QGst::StatePlaying );
        }
        break;
    default:
        break;
    }
}

void GStreamerWidget::onPositionUpdate()
{
    if( mUserSeeks ) return;
    QTime pos_ = position();
    QTime len_ = lenght();

    int pos = -pos_.msecsTo( QTime(0,0) );
    int len = -len_.msecsTo( QTime(0,0) );
    
    mPositionSlider->setMaximum( len/10 );
    mPositionSlider->setValue( pos/10 );

    bool over1h = len_.hour() > 0;
    if( over1h ) {
        mPositionSliderTooltip = QString("%1 / %2").arg( pos_.toString("hh:mm:ss"), len_.toString("hh:mm:ss") );
    }
    else {
        mPositionSliderTooltip = QString("%1 / %2").arg( pos_.toString("mm:ss"), len_.toString("mm:ss") );
    }

    if( mPositionSlider->underMouse() ) {
        QToolTip::showText( QCursor::pos(), mPositionSliderTooltip, mPositionSlider );
    }
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

void GStreamerWidget::sourceSetup( QGst::ElementPtr source )
{
    source->setProperty( "ssl-strict", false );
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
    if( mTargetState == QGst::StatePlaying ) {
        mPipeline->setState( QGst::StatePaused );
        mInternalPaused = true;
    }
}

void GStreamerWidget::internalResume()
{
    if( !mInternalPaused ) return;
    if( mIsBuffering || mUserSeeks ) return;

    if( mTargetState == QGst::StatePlaying ) {
        mPipeline->setState( QGst::StatePlaying );
        mInternalPaused = false;
    }
}

void GStreamerWidget::populateFormatMenu()
{
    delete mFormatGroup;
    delete mFormatMapper;
    mFormatGroup = new QActionGroup( this );
    mFormatMapper = new QSignalMapper( this );
    mFormatGroup->setExclusive( true );
    mFormatIdToAction.clear();;

    connect( mFormatMapper, SIGNAL(mapped(QString)), SLOT(setFormat(QString)) );
    if( !mProvider ) return;

    QList<Format> formatList = mProvider->getFormats();
    
    for( const Format &f : formatList ) {
        QAction *action = mFormatGroup->addAction(f.title);
        action->setCheckable( true );
        mFormatMenu->addAction( action );

        connect( action, SIGNAL(triggered()), mFormatMapper, SLOT(map()) );
        mFormatMapper->setMapping( action, f.id );

        mFormatIdToAction.insert( f.id, action );

        if( f.id == mCurrentFormatId ) {
            action->setChecked( true );
        }
    }
}

#include "GStreamerWidget.moc"