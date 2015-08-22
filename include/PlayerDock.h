#pragma once

#include <QtWidgets/qdockwidget.h>

class Entry;
class PlayerTab;


class PlayerDock :
    public QDockWidget
{
    Q_OBJECT
public:
    PlayerDock( PlayerTab *tab );

public slots:
    virtual void entryOpened( QSharedPointer<Entry> entry ) = 0;

    virtual void playbackStarted( QSharedPointer<Entry> entry ) {}
    virtual void playbackEnded() {}

signals:
    void openUrl( QString url );
    void closed();

protected:
    virtual void closeEvent( QCloseEvent * event );

protected:
    PlayerTab *mTab;
    QWidget *mContent;
};

PlayerDock* CreateSeachDock( PlayerTab *tab );
PlayerDock* CreateReleatedDock( PlayerTab *tab );
PlayerDock* CreateDescriptionDock( PlayerTab *tab );
PlayerDock* CreateCommentDock( PlayerTab *tab );
PlayerDock* CreateHistoryDock( PlayerTab *tab );
PlayerDock* CreateUploadsDock( PlayerTab *tab );
PlayerDock* CreateWatchLaterDock( PlayerTab *tab );

