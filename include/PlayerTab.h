#pragma once

#include <QtWidgets/qmainwindow.h>

class PlayerDock;
class StreamingPlayer;
class GStreamerWidget;
class Entry;
class Feed;

class PlayerTab :
    public QMainWindow
{
    Q_OBJECT
public:
    enum PlayerDockType : unsigned int {
        PD_SearchDock       = (1 << 0),
        PD_ReleatedDock     = (1 << 1),
        PD_DescriptionDoc   = (1 << 2),
        PD_CommentDoc       = (1 << 3),
        PD_HistoryDoc       = (1 << 4),
        PD_UploadsDoc       = (1 << 5),
        PD_WatchLaterDock   = (1 << 6),

        PD_COUNT = 7
    };
    enum DockPage : unsigned int {
        DP_SearchPage = PD_SearchDock,
        DP_StartPage = DP_SearchPage,
    };

    static QString NameOfDock( PlayerDockType type );


public:
    PlayerTab( StreamingPlayer *player );

    void showDockPage( DockPage page );
    PlayerDock* openDock( PlayerDockType dock );
    void closeDock( PlayerDockType dock );

    bool isDockOpen( PlayerDockType dock );


    StreamingPlayer *getPlayer() {
        return mPlayer;
    }

    QSharedPointer<Feed> getHistory() {
        return mHistoryFeed;
    }

public slots:
    void openUrl( QString url );


signals:
    void setEntry( QSharedPointer<Entry> entry );

    void playbackStarted( QSharedPointer<Entry> entry );
    void playbackFinnished();

    void dockOpened( PlayerDockType dock );
    void dockClosed( PlayerDockType dock );

private slots:
    void entrySet( QSharedPointer<Entry> entry );
    void entryChanged();
    void dockClosed();

private:
    void createVidoePlayer();

private:
    struct DockInfo {
        PlayerDockType type;
        PlayerDock *dock;
    };

private:
    StreamingPlayer *mPlayer;
    QList<DockInfo> mDocks;
    unsigned int mOpenDocks = 0, mAutoOpenDocks = 0;
    GStreamerWidget *mVideoPlayer = nullptr;

    QSharedPointer<Entry> mEntry;
    QSharedPointer<Feed> mHistoryFeed;
};