#pragma once

#include "UrlOpener.h"

#include <qsharedpointer.h>


class QProcess;

class GenericUrlOpener :
    public UrlOpener
{
    Q_OBJECT
public:
    GenericUrlOpener( QObject *parent = nullptr );
    
    virtual bool canOpenUrl( QString url ) override;
    virtual QSharedPointer<Entry> openUrl( QString url ) override;

private slots:
    void processExited( int exitCode );

private:
    QMap<QProcess*,QSharedPointer<Entry>> mPendingEntries;
};