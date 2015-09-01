#pragma once


#include <qdialog.h>

#include "VideoProvider.h"

class QTextBrowser;
class QLineEdit;
class QComboBox;


class CreateCustomFormatDialog :
    public QDialog
{
    Q_OBJECT
public:
    CreateCustomFormatDialog( QWidget *parent, QList<Format> formats );

signals:
    void createFormat( QString name, QString videoFormat, QString audioFormat );
    
private slots:
    void onAccept();

    void videoFormatSelectionChanged();
    void audioFormatSelectionChanged();

private:
    const QList<Format> mFormats;

    QLineEdit *mFormatName;

    QComboBox *mVideoFormatList,
              *mAudioFormatList;

    QTextBrowser *mVideoFormatInfo,
                 *mAudioFormatInfo;
};