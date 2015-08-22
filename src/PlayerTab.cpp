#include "PlayerTab.h"
#include "PlayerDock.h"
#include "streamingPlayer.h"
#include "GStreamerWidget.h"
#include "UrlOpener.h"
#include "Feed.h"


#include <qdebug.h>

class HistoryFeed :
    public Feed
{
    virtual bool hasMore() {
        return false;
    }
    virtual void retriveMore() {}
};

PlayerDock* createDock( PlayerTab::PlayerDockType type, PlayerTab *tab )
{
    switch( type ) {
    case( PlayerTab::PD_SearchDock ):
        return CreateSeachDock( tab );
    case( PlayerTab::PD_ReleatedDock ):
        return CreateReleatedDock( tab );
    case( PlayerTab::PD_DescriptionDoc ):
        return CreateDescriptionDock( tab );
    case( PlayerTab::PD_CommentDoc ):
        return CreateCommentDock( tab );
    case( PlayerTab::PD_HistoryDoc ):
        return CreateHistoryDock( tab );
    case( PlayerTab::PD_UploadsDoc ):
        return CreateUploadsDock( tab );
    case( PlayerTab::PD_WatchLaterDock ):
        return CreateWatchLaterDock( tab );
    }

    return nullptr;
}

Qt::DockWidgetArea defaultAreaForDock( PlayerTab::PlayerDockType type )
{
    switch( type ) {
    case( PlayerTab::PD_SearchDock ):
    case( PlayerTab::PD_ReleatedDock ):
    case( PlayerTab::PD_HistoryDoc ):
    case( PlayerTab::PD_UploadsDoc ):
    case( PlayerTab::PD_WatchLaterDock ):
        return Qt::DockWidgetArea::RightDockWidgetArea;
    case( PlayerTab::PD_DescriptionDoc ):
    case( PlayerTab::PD_CommentDoc ):                            
        return Qt::DockWidgetArea::BottomDockWidgetArea;
    }
    return Qt::DockWidgetArea::RightDockWidgetArea;
}


QString PlayerTab::NameOfDock( PlayerDockType type )
{
    switch( type ) {
    case( PlayerTab::PD_SearchDock ):
        return QStringLiteral("Search");
    case( PlayerTab::PD_ReleatedDock ):
        return QStringLiteral("Releated");
    case( PlayerTab::PD_DescriptionDoc ):
        return QStringLiteral("Description");
    case( PlayerTab::PD_CommentDoc ):
        return QStringLiteral("Comment");
    case( PlayerTab::PD_HistoryDoc ):
        return QStringLiteral("History");
    case( PlayerTab::PD_UploadsDoc ):
        return QStringLiteral("Uploads");
    case( PlayerTab::PD_WatchLaterDock ):
        return QStringLiteral("WatchLater");
    }

    return QString();
}

PlayerTab::PlayerTab( StreamingPlayer *player ) :
    QMainWindow(player),
    mPlayer(player)
{
    connect( this, SIGNAL(setEntry(QSharedPointer<Entry>)), SLOT(entrySet(QSharedPointer<Entry>)) );

    QWidget *dummyCenter = new QWidget;
    setCentralWidget(dummyCenter);
    dummyCenter->setFixedSize(0,0);

    mHistoryFeed.reset( new HistoryFeed );
}

void PlayerTab::showDockPage( DockPage page )
{
    for( unsigned int i=0; i < PD_COUNT; ++i ) {
        PlayerDockType dockType = static_cast<PlayerDockType>( 1 << i );
        if( page & dockType ) {
            openDock( dockType );
        }
    }
}

PlayerDock* PlayerTab::openDock( PlayerDockType type )
{
    if( (mOpenDocks & type) == 0 ) {
        PlayerDock *dock = createDock( type, this );
        if( dock ) {
            connect( dock, SIGNAL(openUrl(QString)), SLOT(openUrl(QString)) );
            connect( this, SIGNAL(setEntry(QSharedPointer<Entry>)), dock, SLOT(entryOpened(QSharedPointer<Entry>)) );
            connect( dock, SIGNAL(closed()), SLOT(dockClosed()) );
            connect( this, SIGNAL(playbackStarted(QSharedPointer<Entry>)), dock, SLOT(playbackStarted(QSharedPointer<Entry>)) );
            connect( this, SIGNAL(playbackFinnished()), dock, SLOT(playbackFinnished()) );
            
            Qt::DockWidgetArea area = defaultAreaForDock(type);
            addDockWidget(defaultAreaForDock(type), dock );

            for( DockInfo &info : mDocks ) {
                if( dockWidgetArea(info.dock) == area ) {
                    tabifyDockWidget( info.dock, dock );
                }
            }

            DockInfo info;
                info.type = type;
                info.dock = dock;

            mDocks.push_back( info );

            mOpenDocks |= type;
            dockOpened( type );

            if( mEntry ) {
                dock->entryOpened( mEntry );
            }
        }
        return dock;
    }
    return nullptr;
}

void PlayerTab::closeDock( PlayerDockType dock )
{    
    for( auto iter=mDocks.begin(),end=mDocks.end(); iter != end; ++iter ) {
        if( iter->type == dock ) {
            iter->dock->deleteLater();
            mOpenDocks = mOpenDocks&~iter->type;
            mDocks.erase( iter );
            dockClosed( dock );
            return;
        }
    }
    Q_ASSERT( false && "Dock wasn't in the dock list!" );
}

bool PlayerTab::isDockOpen( PlayerDockType dock )
{
    return mOpenDocks&dock;
}


void PlayerTab::openUrl( QString url )
{
    QList<QSharedPointer<UrlOpener>> urlOpeners = mPlayer->getUrlOpeners();

    for( QSharedPointer<UrlOpener> urlOpener : urlOpeners ) {
        if( urlOpener->canOpenUrl(url) ) {
            QSharedPointer<Entry> entry = urlOpener->openUrl( url );
            if( entry ) {
                setEntry( entry );
                return;
            }
        }
    }

    qDebug() << "Failed to open url " << url;
}

void PlayerTab::entrySet( QSharedPointer<Entry> entry )
{
    disconnect( this, SLOT(entryChanged()) );
    mEntry = entry;

    if( mEntry ) {
        connect( mEntry.data(), SIGNAL(changed()), SLOT(entryChanged()) );
        entryChanged();
    }

    mHistoryFeed->content.push_back( entry );
    mHistoryFeed->retriveMore();
}

void PlayerTab::entryChanged()
{
    Q_ASSERT( mEntry );
    if( (mAutoOpenDocks&PD_ReleatedDock) == 0 && mEntry->properties["releative"].isValid() ) {
        PlayerDock *dock = openDock( PD_ReleatedDock );
        if( dock ) {
            mAutoOpenDocks |= PD_ReleatedDock;
        }
    }
    if( (mAutoOpenDocks&PD_DescriptionDoc) == 0 && mEntry->properties["description"].isValid() ) {
        PlayerDock *dock = openDock( PD_DescriptionDoc );
        if( dock ) {
            mAutoOpenDocks |= PD_DescriptionDoc;
        }
    }
    if( (mAutoOpenDocks&PD_UploadsDoc) == 0 && mEntry->properties["uploads"].isValid() ) {
        PlayerDock *dock = openDock( PD_UploadsDoc );
        if( dock ) {
            mAutoOpenDocks |= PD_UploadsDoc;
        }
    }
    if( (mAutoOpenDocks&PD_CommentDoc) == 0 && mEntry->properties["comments"].isValid() ) {
        PlayerDock *dock = openDock( PD_CommentDoc );
        if( dock ) {
            mAutoOpenDocks |= PD_CommentDoc;
        }
    }
    if( mEntry->properties["title"].isValid() ) {
        setWindowTitle( mEntry->properties["title"].toString() );
    }

    if( mVideoPlayer == nullptr && mEntry->properties["videoProvider"].isValid() ) {
        createVidoePlayer();
        mVideoPlayer->setEntry(mEntry);
    }
}

void PlayerTab::dockClosed()
{
    PlayerDock *dock = qobject_cast<PlayerDock*>(sender());
    Q_ASSERT( dock );

    for( auto iter=mDocks.begin(),end=mDocks.end(); iter != end; ++iter ) {
        if( iter->dock == dock ) {
            PlayerDockType type = iter->type;
            dock->deleteLater();
            mOpenDocks = mOpenDocks&~iter->type;
            mDocks.erase( iter );
            dockClosed( type );
            return;
        }
    }
    Q_ASSERT( false && "Dock wasn't in the dock list!" );
}

void PlayerTab::createVidoePlayer()
{
    mVideoPlayer = new GStreamerWidget(this);
    setCentralWidget( mVideoPlayer );

    connect( this, SIGNAL(setEntry(QSharedPointer<Entry>)), mVideoPlayer, SLOT(setEntry(QSharedPointer<Entry>)) );
    connect( mVideoPlayer, SIGNAL(playbackStarted(QSharedPointer<Entry>)), SIGNAL(playbackStarted(QSharedPointer<Entry>)) );
    connect( mVideoPlayer, SIGNAL(playbackFinnished()), SIGNAL(playbackFinnished()) );
}