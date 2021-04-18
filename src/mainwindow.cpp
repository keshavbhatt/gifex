#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextDocument>
#include <QTime>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(QApplication::applicationName()+" v"+QApplication::applicationVersion());
    this->setWindowIcon(QIcon(":/icons/app/icon-256.png"));
    loadSettings();
    ui->stopRec->hide();
    foreach (QLineEdit *edit, ui->recordGroupBox->findChildren<QLineEdit*>()) {
        connect(edit,&QLineEdit::textChanged,[=](QString str){
            int value = str.trimmed().simplified().toInt();
            settings.setValue(edit->objectName(),value);
        });
    }
    if(settings.value("geometry").isValid()){
        restoreGeometry(settings.value("geometry").toByteArray());
        if(settings.value("windowState").isValid()){
            restoreState(settings.value("windowState").toByteArray());
        }else{
            QScreen* pScreen = QGuiApplication::screenAt(this->mapToGlobal({this->width()/2,0}));
            QRect availableScreenSize = pScreen->availableGeometry();
            this->move(availableScreenSize.center()-this->rect().center());
        }
    }else{
        this->resize(this->minimumSizeHint());
    }
}

void MainWindow::loadSettings()
{
    setStyle(":/qbreeze/"+settings.value("theme","dark").toString()+".qss");
    ui->darkTheme->blockSignals(true);
    ui->darkTheme->setChecked(settings.value("theme","dark").toString()=="dark"?true:false);
    ui->darkTheme->blockSignals(false);

    ui->selectionRectBorder->setValue(settings.value("selectionRectBorder",3).toInt());
    ui->captureRectBorder->setValue(settings.value("captureRectBorder",3).toInt());

    ui->decoration->setChecked(settings.value("decoration",true).toBool());
    ui->captureCursor->setChecked(settings.value("captureCursor",true).toBool());
    ui->selectionRectVisible->setChecked(settings.value("selectionRectVisible",true).toBool());
    ui->followCursor->setChecked(settings.value("followCursor",false).toBool());


    foreach (QLineEdit *edit, ui->recordGroupBox->findChildren<QLineEdit*>()) {
            QString  value = settings.value(edit->objectName(),getDefaultValue(edit->objectName())).toString();
            edit->setText(value);
    }
}

QString MainWindow::getDefaultValue(QString objectName)
{
    if(objectName=="x"||objectName=="y"){
        return "0";
    }else{
        return "400";
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("geometry",saveGeometry());
    settings.setValue("windowState", saveState());
    //clear locatemp dir
    QDir(utils::returnPath("localTempCap")).removeRecursively();
    QMainWindow::closeEvent(event);
}


void MainWindow::setStyle(QString fname)
{
    QFile styleSheet(fname);
    if (!styleSheet.open(QIODevice::ReadOnly))
    {
        qWarning("Unable to open file");
        return;
    }
    qApp->setStyleSheet(styleSheet.readAll());
    styleSheet.close();
}

MainWindow::~MainWindow()
{
    if(slopProcess != nullptr){
        slopProcess->blockSignals(true);
        slopProcess->close();
        slopProcess->disconnect();
        slopProcess->deleteLater();
        slopProcess->blockSignals(false);
        slopProcess = nullptr;
    }
    delete ui;
}

void MainWindow::on_selectRegion_clicked()
{
    if(slopProcess != nullptr){
        slopProcess->blockSignals(true);
        slopProcess->close();
        slopProcess->disconnect();
        slopProcess->deleteLater();
        slopProcess->blockSignals(false);
        slopProcess = nullptr;
    }
    slopProcess= new QProcess(this);
    connect(slopProcess, static_cast<void (QProcess::*)
            (int,QProcess::ExitStatus)>(&QProcess::finished),
            [this]
            (int exitCode,QProcess::ExitStatus exitStatus)
    {
        Q_UNUSED(exitStatus);
        if(exitCode==0)
        {
            QString output = slopProcess->readAll().trimmed().simplified();
            qDebug()<<output;
            setCoordinates(output);
        }else
        {
            showError("Process exited with code "+QString::number(exitCode)+"\n"+slopProcess->readAll());
        }
    });
    QStringList slopArgs;
    slopArgs<<"-b"<<QString::number(settings.value("selectionRectBorder",3).toInt());
    slopArgs<<"--nodecorations"<<QString(settings.value("decoration",true).toBool()?"0":"1");
    slopArgs<<"-c"<<"0.3,0.7,0.9,1";
    slopProcess->start("slop",slopArgs);
}

void MainWindow::setCoordinates(QString slopOutPut)
{
    //409x212+0+0
    int indexOfPlus = slopOutPut.indexOf("+",0);
        QString hw  = slopOutPut.left(indexOfPlus);
        QString xy  = slopOutPut.right(slopOutPut.count()-(hw.count()+1));
         QString x  = xy.split("+").first();
         QString y  = xy.split("+").last();
     QString width  = hw.split("x").first();
    QString height  = hw.split("x").last();

    ui->x->setText(x);ui->y->setText(y);
    ui->width->setText(width);ui->height->setText(height);
}

void MainWindow::on_darkTheme_toggled(bool checked)
{
    settings.setValue("theme",checked?"dark":"light");
    setStyle(":/qbreeze/"+settings.value("theme",QString(checked?"dark":"light")).toString()+".qss");
    if(settings.value("theme","dark").toString()=="dark"){
        QPalette p;
        p.setColor(QPalette::PlaceholderText, QColor("#898b8d"));
        foreach (QLineEdit *edit, this->findChildren<QLineEdit*>()) {
            edit->setPalette(p);
        }
    }
}

void MainWindow::showError(QString message)
{
    //init error
    _error = new Error(this);
    _error->setAttribute(Qt::WA_DeleteOnClose);
    _error->setWindowTitle(QApplication::applicationName()+" | Error dialog");
    _error->setWindowFlag(Qt::Dialog);
    _error->setWindowModality(Qt::NonModal);
    _error->setError("An Error ocurred while processing your request!",
                     message);
    _error->show();
}

void MainWindow::on_startRec_clicked()
{
    QStringList  recordArgs = prepareArgs();
    if(recordProcess != nullptr){
        recordProcess->blockSignals(true);
        recordProcess->close();
        recordProcess->disconnect();
        recordProcess->deleteLater();
        recordProcess->blockSignals(false);
        recordProcess = nullptr;
    }
    recordProcess= new QProcess(this);
    connect(recordProcess,&QProcess::readyRead,[=](){
        QString output = recordProcess->readAll();
        qDebug()<<output;
    });
    connect(recordProcess, static_cast<void (QProcess::*)
            (int,QProcess::ExitStatus)>(&QProcess::finished),
            [this]
            (int exitCode,QProcess::ExitStatus exitStatus)
    {
        stopRecordTimer();
        ui->stopRec->hide();
        ui->startRec->show();

        Q_UNUSED(exitStatus);
        if(exitCode==0)
        {
            videoProcessor = new VideoProcessor(this);
            videoProcessor->setAttribute(Qt::WA_DeleteOnClose);
            videoProcessor->setWindowTitle(QApplication::applicationName()+" | Process Recording");
            videoProcessor->setWindowFlag(Qt::Dialog);
            videoProcessor->setWindowModality(Qt::ApplicationModal);
            videoProcessor->setSource(utils::returnPath("localTempCap")+"tmp.mkv");
            videoProcessor->show();

        }else
        {
            showError("Process exited with code "+QString::number(exitCode)+"\n"+recordProcess->readAll());
        }

    });
    recordProcess->start("ffmpeg",recordArgs);
    if(recordProcess->waitForStarted()){
        ui->startRec->hide();
        ui->stopRec->show();
        startRecordTimer();
    }
}

void MainWindow::on_stopRec_clicked()
{
    if(recordProcess!=nullptr)
    {
        if(recordProcess->write("\nq")!=-1){
            stopRecordTimer();
        }else{
            showError("An error occured while stopping the record process.");
        }
    }
}

void MainWindow::stopRecordTimer()
{
    if(recordTimer!=nullptr){
        recordTimer->blockSignals(true);
        recordTimer->deleteLater();
        recordTimer->blockSignals(false);
        recordTimer = nullptr;
    }
}

void MainWindow::startRecordTimer()
{
    if(recordTimer!=nullptr){
        recordTimer->blockSignals(true);
        recordTimer->deleteLater();
        recordTimer->blockSignals(false);
        recordTimer = nullptr;
    }
    recordTimer = new QTimer(this);
    recordTimer->setInterval(1000);
    connect(recordTimer,&QTimer::timeout,[=]()
    {
        int secs = time.elapsed() / 1000;
                int mins = (secs / 60) % 60;
                int hours = (secs / 3600);
                secs = secs % 60;
                QString elapsed = QString("(%1:%2:%3) Stop Recording").arg(hours, 2, 10, QLatin1Char('0'))
                .arg(mins, 2, 10, QLatin1Char('0'))
                .arg(secs, 2, 10, QLatin1Char('0'));
                ui->stopRec->setText(elapsed);

    });
    recordTimer->start();
    time.start();
}

QStringList MainWindow::prepareArgs()
{
    //-c:v libx264rgb -crf 0 -preset ultrafast output.mkv
    QStringList arg;
    arg<<"-video_size"<<settings.value("width",400).toString()+"x"+settings.value("height",400).toString();
    arg<<"-draw_mouse"<<QString(settings.value("captureCursor",true).toBool()?"1":"0");
    if(settings.value("followCursor",false).toBool()){
        arg<<"-follow_mouse"<<"centered";
    }
    arg<<"-framerate"<<"60";
    arg<<"-f"<<"x11grab";
    arg<<"-grab_x"<<settings.value("x","0").toString();
    arg<<"-grab_y"<<settings.value("y","0").toString();
    arg<<"-show_region"<<QString(settings.value("selectionRectVisible",true).toBool()?"1":"0");
    arg<<"-region_border"<<QString::number(settings.value("captureRectBorder",3).toInt());
    arg<<"-i"<<":0.0";
    /* use looseless fast encoding
        -crf 0 tells x264 to encode in lossless mode;
        -preset ultrafast advises it to do so fast.
        Note the use of libx264rgb rather than libx264;
        the latter would do a lossy conversion from RGB to yuv444p.*/
    arg<<"-c:v"<<"libx264rgb"<<"-crf"<<"0"<<"-preset"<<"ultrafast";
    arg<<utils::returnPath("localTempCap")+"tmp.mkv";
    arg<<"-y";
    return arg;
}




void MainWindow::on_selectionRectBorder_valueChanged(int arg1)
{
    settings.setValue("selectionRectBorder",arg1);
}


void MainWindow::on_selectionRectVisible_toggled(bool checked)
{
    settings.setValue("selectionRectVisible",checked);
}

void MainWindow::on_captureCursor_toggled(bool checked)
{
    settings.setValue("captureCursor",checked);
}

void MainWindow::on_decoration_toggled(bool checked)
{
    settings.setValue("decoration",checked);
}

void MainWindow::on_followCursor_toggled(bool checked)
{
    settings.setValue("followCursor",checked);
}


void MainWindow::on_captureRectBorder_valueChanged(int arg1)
{
     settings.setValue("captureRectBorder",arg1);
}
