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
    mQueue = QGst::ElementFactory::make("multiqueue");

    Q_ASSERT( !mPipeline.isNull() && !mPlaySink.isNull() && !mQueue.isNull() );

    mPipeline->add( mPlaySink );
    mPipeline->add( mQueue );

    mPlaySink->setProperty( "flags", DEFAULT_PLAYSINK_FLAGS );
    mQueue->setProperty( "sync-by-running-time", true );

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

    // We store the last known lenght in a member since sometimes the quary fails.
    // most noticable right after a seek.
    if( mPipeline->query(quary) ) {
        mLenght = QGst::ClockTime(quary->duration()).toTime();
    }
    else {
        //qDebug() << "Failed to quary duration of pipeline!";
    }
    return mLenght;
}

QTime GStreamerPipeline::getPosition()
{
    QGst::PositionQueryPtr quary = QGst::PositionQuery::create( QGst::FormatTime );
    if( mPipeline->query(quary) ) {
        mPosition = QGst::ClockTime(quary->position()).toTime();
    }
    else {
        //qDebug() << "Failed to quary position of pipeline!";
    }
    return mPosition;
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
    // for now, only support adding of streams in the null state.
    Q_ASSERT( mState == PS_Null );

    StreamInfo info;

    info.type = type;
    info.uri = uri;

    info.source  = QGst::ElementFactory::make( "souphttpsrc" );
    info.queue   = QGst::ElementFactory::make( "queue2" );
    info.decoder = QGst::ElementFactory::make( "decodebin" );
    Q_ASSERT( info.source && info.decoder && info.decoder );

    mPipeline->add( info.source );
    mPipeline->add( info.queue );
    mPipeline->add( info.decoder );

    info.source->link( info.queue );
    info.queue->link( info.decoder );
    
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
    
    info.source->setProperty( "location", uri );
    info.source->setProperty( "ssl-strict", false );
    info.queue->setProperty( "max-size-time", QGst::ClockTime::fromSeconds(20) );
    info.queue->setProperty( "max-size-bytes", 20*1024*1024 );
    info.queue->setProperty( "max-size-buffers", 0 );
    info.queue->setProperty( "low-percent", 1 ); // only start sending buffering messages then the buffer is empthy
    info.queue->setProperty( "high-percent", 50 ); // consider 100% buffered, when we fill the buffer up to 50%
    info.queue->setProperty( "use-buffering", true );
    info.queue->setProperty( "use-rate-estimate", true );
    info.decoder->setProperty( "expose-all-streams", false );
    info.decoder->setProperty( "caps", caps );

    QGlib::connect( info.decoder, "pad-added", this, &GStreamerPipeline::decodedPadAdded, QGlib::PassSender );

    mStreams.push_back( info );
}

void GStreamerPipeline::seek( QTime position )
{
    mPipeline->seek( QGst::FormatTime, QGst::SeekFlagAccurate | QGst::SeekFlagFlush, QGst::ClockTime::fromTime(position) );
    // the quary about position is failing right after a seek, before the pipeline have got its new data.
    // so we help it out by seting our position to the seek position.
    mPosition = position;
}

void GStreamerPipeline::expose()
{
    QGst::VideoOverlayPtr overlay = mPlaySink.dynamicCast<QGst::VideoOverlay>();
    Q_ASSERT( overlay );
    overlay->expose();
}

void GStreamerPipeline::decodedPadAdded( QGst::ElementPtr decoder, QGst::PadPtr pad )
{
    QString caps = pad->currentCaps()->toString();

    qDebug() << "Decoded pad found, caps: " << caps;

    QGst::PadPtr target;
    if( caps.startsWith("audio/x-raw") ) {
        target = mPlaySink->getRequestPad( "audio_raw_sink" );
    }
    else if( caps.startsWith("video/x-raw") ) {
        target = mPlaySink->getRequestPad( "video_raw_sink" );
    }
    else if( caps.startsWith("text/x-raw") ) {
        target = mPlaySink->getRequestPad( "text_sink" );
    }
    if( target ) {
        mPadsFound++;

        QGst::PadPtr multi_sink,
                     multi_src;
        
        multi_sink =  mQueue->getRequestPad( QString("sink_%1").arg(mPadsFound).toStdString().c_str() );
        multi_src = mQueue->getStaticPad( QString("src_%1").arg(mPadsFound).toStdString().c_str() );
        
        pad->link( multi_sink );
        multi_src->link( target );
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
        if( mState == PS_Finnished ) {
            seek( QTime(0,0) );
        }
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
    QGst::ObjectPtr source = msg->source();

    int minPercent = 100;

    bool isFromRightSource = false;
    for( StreamInfo &info : mStreams ) {
        if( info.queue == source ) {
            isFromRightSource = true;
            if( percent <= 5 ) {
                info.isBuffering = true;
                switchToState( PS_Buffering );
            }
            if( percent == 100 ) {
                info.isBuffering = false;
            }
            info.percent = percent;
        }
        if( info.isBuffering ) {
            minPercent = std::min( minPercent, info.percent );
        }
    }
    if( !isFromRightSource ) return;


    qDebug() << "Buffering... " << minPercent << "Source: " << source->name();

    buffering( minPercent );
    if( minPercent == 100 ) {
        finnishedBuffering();
        if( mPlayAfterBuffering ) {
            switchToState( PS_Playing );
        }
        else {
            switchToState( PS_Paused );
        }
    }
}

