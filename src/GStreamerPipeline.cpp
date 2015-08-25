#include "GStreamerPipeline.h"


#include <QGst/elementfactory.h>
#include <QGst/message.h>
#include <QGst/query.h>
#include <QGst/bus.h>
#include <QGlib/connect.h>

static const int DEFAULT_PLAYSINK_FLAGS = 0x41f;
static const int VIDEO_FLAG = 0x1;
static const int AUDIO_FLAG = 0x2;

GStreamerPipeline::GStreamerPipeline( QObject *parent ) :
    QObject(parent)
{
    mPipeline = QGst::ElementFactory::make("pipeline").dynamicCast<QGst::Pipeline>();
    mPlaySink = QGst::ElementFactory::make("playsink");

    Q_ASSERT( !mPipeline.isNull() && !mPlaySink.isNull() );

    mPipeline->add( mPlaySink );
    mPlaySink->setProperty( "flags", DEFAULT_PLAYSINK_FLAGS );
    qDebug() << "Hello :)";
    
    QGst::BusPtr bus = mPipeline->bus();
    bus->addSignalWatch();
    QGlib::connect(bus, "message", this, &GStreamerPipeline::handleBusMessage);
}

GStreamerPipeline::~GStreamerPipeline()
{
    mPipeline->setState( QGst::StateNull );
}

QTime GStreamerPipeline::getLenght()
{
    QGst::DurationQueryPtr quary = QGst::DurationQuery::create( QGst::FormatTime );
    if( mPipeline->query(quary) ) {
        return QGst::ClockTime(quary->duration()).toTime();
    }
    qDebug() << "Failed to quary duration of pipeline!";
    return QTime();
}

QTime GStreamerPipeline::getPosition()
{
    QGst::PositionQueryPtr quary = QGst::PositionQuery::create( QGst::FormatTime );
    if( mPipeline->query(quary) ) {
        return QGst::ClockTime(quary->position()).toTime();
    }
    qDebug() << "Failed to quary position of pipeline!";
    return QTime();
}

void GStreamerPipeline::play()
{
    switchToState( PS_Playing );
}

void GStreamerPipeline::pause()
{
    switchToState( PS_Paused );
}

void GStreamerPipeline::disableVideo()
{
    int flags = mPlaySink->property("flags").toInt();
    mPlaySink->setProperty( "flags", flags & ~VIDEO_FLAG );
}

void GStreamerPipeline::enableVideo()
{
    int flags = mPlaySink->property("flags").toInt();
    mPlaySink->setProperty( "flags", flags | VIDEO_FLAG );
}

void GStreamerPipeline::addStream( QString uri, StreamType type )
{
    StreamInfo info;

    info.type = type;
    info.uri = uri;

    info.decoder = QGst::ElementFactory::make( "uridecodebin" );
    Q_ASSERT( info.decoder );
    
    QGst::CapsPtr caps = QGst::Caps::createEmpty();
    if( type & ST_Audio ) {
        caps->append( QGst::Caps::fromString("audio/x-raw") ); 
    }
    if( type & ST_Video ) {
        caps->append( QGst::Caps::fromString("video/x-raw") );
    }
    if( type & ST_Subtitles ) {
        caps->append( QGst::Caps::fromString("text/x-raw") );
    }
    
    info.decoder->setProperty( "expose-all-streams", false );
    info.decoder->setProperty( "caps", caps );
    info.decoder->setProperty( "uri", uri );
    info.decoder->setProperty( "buffer-duration", QGst::ClockTime::fromSeconds(10) );

    mPipeline->add( info.decoder );

    QGlib::connect( info.decoder, "source-setup", this, &GStreamerPipeline::sourceSetup );
    QGlib::connect( info.decoder, "pad-added", this, &GStreamerPipeline::decodedPadAdded, QGlib::PassSender );
}

void GStreamerPipeline::seek( QTime position )
{
    mPipeline->seek( QGst::FormatTime, QGst::SeekFlagAccurate | QGst::SeekFlagFlush, QGst::ClockTime::fromTime(position) );
}

void GStreamerPipeline::sourceSetup( QGst::ElementPtr source )
{
    source->setProperty( "ssl-strict", false );
}

void GStreamerPipeline::decodedPadAdded( QGst::ElementPtr decoder, QGst::PadPtr pad )
{
    QString caps = pad->currentCaps()->toString();

    QGst::PadPtr sink;
    if( caps.startsWith("audio/x-raw") ) {
        sink = mPlaySink->getRequestPad( "audio_sink" );
    }
    else if( caps.startsWith("video/x-raw") ) {
        sink = mPlaySink->getRequestPad( "video_sink" );
    }
    else if( caps.startsWith("text/x-raw") ) {
        sink = mPlaySink->getRequestPad( "text_sink" );
    }
    if( sink ) {
        pad->link( sink );
    }
}

void GStreamerPipeline::handleBusMessage( const QGst::MessagePtr &message )
{
    switch( message->type() ) {
    case( QGst::MessageEos ):
        onEos();
        break;
    case( QGst::MessageError ):
        onError( message.staticCast<QGst::ErrorMessage>() );
        break;
    case( QGst::MessageBuffering ):
        onBuffering( message.staticCast<QGst::BufferingMessage>() );
        break;
    case( QGst::MessageClockLost ):
        qDebug() << "Clock Lost!";
        mPipeline->setState( QGst::StatePaused );
        mPipeline->setState( QGst::StatePlaying );
        break;
    }
}

void GStreamerPipeline::switchToState( PipelineState state )
{
    if( state == mState ) return;

    switch( state ) {
    case( PS_Paused ):
        mPipeline->setState( QGst::StatePaused );
        mPlayAfterBuffering = false;
        break;
    case( PS_Playing ):
        mPipeline->setState( QGst::StatePlaying );
        mPlayAfterBuffering = true;
        break;
    case( PS_Buffering ):
        mPipeline->setState( QGst::StatePaused );
        break;
    case( PS_Finnished ):
        mPipeline->setState( QGst::StatePaused );
        break;
    }
    std::swap( mState, state );
    stateChanged( state, mState );
}

void GStreamerPipeline::onEos()
{
    switchToState( PS_Finnished );
}

void GStreamerPipeline::onError( const QGst::ErrorMessagePtr &msg )
{
    qDebug() << "Error: " << msg->error().what() << ",\t DebugMsg: " << msg->debugMessage();
}

void GStreamerPipeline::onBuffering( const QGst::BufferingMessagePtr &msg )
{
    int percent = msg->percent();
    qDebug() << "Buffering... " << percent << "%";
    if( percent <= 1 ) {
        switchToState( PS_Buffering );
    }
    buffering( percent );
    if( percent == 100 ) {
        finnishedBuffering();
        if( mPlayAfterBuffering ) {
            switchToState( PS_Playing );
        }
        else {
            switchToState( PS_Paused );
        }
    }
}