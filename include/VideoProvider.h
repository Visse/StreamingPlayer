#pragma once

#include <qobject.h>

enum VideoType {
    VT_AudioOnly,
    VT_VideoOnly,
    VT_Normal
};

struct Format {
    VideoType type = VT_Normal;
    QString url, filetype;
    QString title, id;

    int bitrate = 0;
    unsigned int width = 0, height = 0;
};

class VideoProvider :
    public QObject
{
    Q_OBJECT
public:
    virtual QList<Format> getFormats() = 0;

signals:
    void changed();

};

Q_DECLARE_METATYPE(QSharedPointer<VideoProvider>);