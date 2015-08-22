#pragma once

#include <qobject.h>

#include "Feed.h"


class UrlOpener :
    public QObject
{
public:
    UrlOpener( QObject *parent ) :
        QObject(parent)
    {}

    virtual bool canOpenUrl( QString url ) = 0;
    virtual QSharedPointer<Entry> openUrl( QString url ) = 0;
};