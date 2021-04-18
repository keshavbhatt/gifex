#ifndef VideoProcessor_H
#define VideoProcessor_H

#include <QSettings>
#include <QFile>
#include <QDebug>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTime>
#include <QToolTip>
#include <QProcess>
#include <QPushButton>

#include "videoprocessor/seekslider.h"
#include "error.h"
#include "rangeslider.h"
#include "waitingspinnerwidget.h"
#include "ui_console.h"
#include "utils.h"
#include "controlbutton.h"
#include "screenshot.h"


namespace Ui {
class VideoProcessor;
}

class VideoProcessor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool consoleHidden_ READ consoleHidden_ WRITE setConsoleHidden NOTIFY consoleVisibilityChanged)

public:
    explicit VideoProcessor(QWidget *parent = nullptr);
    ~VideoProcessor();

public slots:
    void setSource(QString sourceUrl);

signals:
    void consoleVisibilityChanged(const bool isVisible);

private slots:

    void setConsoleHidden(bool hidden);

    void setStyle(QString fname);

    void updateDuration();

    void showError(QString message);

    void init_player();

    void on_url_textChanged(const QString &arg1);

    void start();

    void on_pickStart_clicked();

    void on_pickEnd_clicked();

    void on_play_clicked();

    void on_minusOneSecLower_clicked();

    void on_plusOneSecLower_clicked();

    void on_minusOneSecUpper_clicked();

    void on_plusOneSecUpper_clicked();

    void on_volume_valueChanged(int value);

    void on_moveToFrameLower_clicked();

    void on_moveToFrameUpper_clicked();

    void on_preview_clicked();


    void on_vIcon_clicked();


    void resizeFix();

    void playMedia(QString url);

    void on_changeLocationButton_clicked();

    void on_location_textChanged(const QString &arg1);

    void on_clip_clicked();

    QString getCodec();

    void on_cancel_clicked();

    void showConsole();

    void hideConsole();

    void showStatus(QString message);

    bool clipOptionChecker();

    void on_screenshotLowerFrame_clicked();

    void on_screenshotUpperFrame_clicked();

    void takeScreenshot();



    void on_fps_valueChanged(int arg1);

    void on_scale_valueChanged(int arg1);

    void on_loop_toggled(bool checked);

    void on_loopCount_valueChanged(int arg1);

    void on_resetScale_clicked();

    void on_infiniteLoop_clicked();

    void on_halfScale_clicked();

protected slots:
    void resizeEvent(QResizeEvent *event);

    void closeEvent(QCloseEvent *event);
private:
    Ui::VideoProcessor *ui;

    Ui::consoleUi consoleUi;

    QWidget *consoleWidget = nullptr;

    QSettings settings;

    RangeSlider *rsH;

    QMediaPlayer *player = nullptr;

    Error * _error = nullptr;

    WaitingSpinnerWidget *_loader = nullptr;

    int tempVolume;

    QProcess *ffmpegProcess = nullptr;

    QProcess *screenshotProcess = nullptr;

    bool isPlayingPreview = false;

    bool consoleHidden_() const;

    bool consoleHidden;

    controlButton *consoleButton = nullptr;

    QString currentFileName;

    Screenshot *screenshot = nullptr;

};

#endif // VideoProcessor_H
