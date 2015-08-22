#include "GenericUrlOpener.h"
#include "GenericVideoProvider.h"


#include <qprocess.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>


GenericUrlOpener::GenericUrlOpener( QObject *parent ) :
    UrlOpener(parent)
{}
    
bool GenericUrlOpener::canOpenUrl( QString url )
{
    return true;
}

QSharedPointer<Entry> GenericUrlOpener::openUrl( QString url )
{
    QStringList args;
    args << "--no-cache-dir" << "--dump-json" << url;
    

    
    QProcess *process = new QProcess(this);
    connect( process, SIGNAL(finished(int)), SLOT(processExited()) );
    process->start( "youtube-dl", args );
    
    QSharedPointer<Entry> entry( new Entry );
    mPendingEntries.insert( process, entry );

    return entry;
}

void GenericUrlOpener::processExited()
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    QSharedPointer<Entry> entry = mPendingEntries.take( process );

    QVariantMap content = QJsonDocument::fromJson(process->readAll()).toVariant().toMap();

    entry->properties["title"] = content["title"];
    entry->properties["description"] = content["description"];
    entry->properties["author"] = content["uploader"];
    entry->properties["viewCount"] = content["view_count"];
    entry->properties["thumbnail"] = content["thumbnail"];

    
    QList<Format> formats;
    if( content["formats"].isValid() ) {
        QVariantList formatList = content["formats"].toList();
        for( auto iter=formatList.begin(), end=formatList.end(); iter != end; ++iter ) {
            QVariantMap d = iter->toMap();
            Format format;
                format.type = VT_Normal;
                format.title = d["format"].toString();
                format.bitrate = d["tbr"].toInt();
                format.width = d["width"].toInt();
                format.height= d["height"].toInt();
                format.url = d["url"].toString();
                format.filetype = d["ext"].toString();
                format.id = d["format_id"].toString();

            formats.push_back( format );
        }
    }
    else if( content["format"].isValid() && content["url"].isValid() ) {
        Format format;
            format.title = content["format"].toString();
            format.url = content["url"].toString();
            format.id = content["format_id"].toString();
        formats.push_back( format );
    }


    if( formats.size() > 0 ) {
        QSharedPointer<VideoProvider> videoProvider( new GenericVideoProvider(formats) );
        entry->properties["videoProvider"] = qVariantFromValue( videoProvider );
    }
    entry->changed();
}