#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QTime>

#include "waitingspinnerwidget.h"
#include "error.h"
#include "utils.h"
#include "videoprocessor/videoprocessor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void setStyle(QString fname);

    void on_selectRegion_clicked();

    void on_darkTheme_toggled(bool checked);

    void loadSettings();

    void showError(QString message);

    void setCoordinates(QString slopOutPut);

    void on_selectionRectBorder_valueChanged(int arg1);

    void on_selectionRectVisible_toggled(bool checked);

    void on_captureCursor_toggled(bool checked);

    QString getDefaultValue(QString objectName);

    void on_startRec_clicked();

    QStringList prepareArgs();

    void on_decoration_toggled(bool checked);

    void on_followCursor_toggled(bool checked);

    void startRecordTimer();

    void on_stopRec_clicked();

    void stopRecordTimer();

    void on_captureRectBorder_valueChanged(int arg1);

protected slots:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    QSettings settings;

    QProcess *slopProcess = nullptr;

    QProcess *recordProcess = nullptr;

    Error * _error = nullptr;

    QTimer *recordTimer = nullptr;

    QTime time;

    VideoProcessor *videoProcessor = nullptr;

};

#endif // MAINWINDOW_H
