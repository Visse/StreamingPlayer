#pragma once

#include <QtWidgets\qwidget.h>

class Entry;
class StreamingPlayer;

class Tile : 
    public QWidget 
{
    Q_OBJECT
public:
    Tile( QWidget *parent );


signals:
    void mouseEntered();
    void mouseLeft();
    void openUrl( QString url );

protected:
    virtual void enterEvent( QEvent *event ) override;
    virtual void leaveEvent( QEvent *event ) override;
};

Tile* createTile( StreamingPlayer *player, QWidget *parent, QSharedPointer<Entry> entry );