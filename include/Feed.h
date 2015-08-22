#pragma once

#include <qobject.h>
#include <qvariant.h>

class Entry :
    public QObject
{
    Q_OBJECT
public:
    QVariantMap properties;

signals:
    void changed();
};

class Feed :
    public QObject 
{
    Q_OBJECT
public:
    QVariantMap properties;
    QList<QSharedPointer<Entry>> content;

    virtual bool hasMore() = 0;
public slots:
    virtual void retriveMore() = 0;

signals:
    void retiviedMore();
    void changed();
};

Q_DECLARE_METATYPE( QSharedPointer<Entry> )
Q_DECLARE_METATYPE( QSharedPointer<Feed> )