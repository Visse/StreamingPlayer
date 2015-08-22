#pragma once

#include <qobject.h>
#include <qpointer.h>

class Feed;

class SearchProvider :
    public QObject
{
    Q_OBJECT
public:
    SearchProvider( QObject *parent = nullptr ) :
        QObject(parent) 
    {}

    virtual QSharedPointer<Feed> search( QString searchTerm ) = 0;
};
