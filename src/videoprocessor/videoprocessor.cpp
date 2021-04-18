#include "videoprocessor.h"
#include "ui_videoprocessor.h"

#include <QPropertyAnimation>
#include <QScreen>
#include <QFileDialog>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDesktopServices>
#include <QSplitter>

VideoProcessor::VideoProcessor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoProcessor)
{
    ui->setupUi(this);
    this->setWindowTitle(QApplication::applicationName()+" v"+QApplication::applicationVersion());
    this->setWindowIcon(QIcon(":/icons/app/icon-256.png"));
    setStyle(":/qbreeze/"+settings.value("theme","dark").toString()+".qss");

    QSplitter *split1 = new QSplitter;
    split1->setObjectName("split1");

    split1->addWidget(ui->playerGroup);
    split1->addWidget(ui->clipGroupBox);
    split1->setStretchFactor(0,1);
    split1->setOrientation(Qt::Horizontal);
    connect(split1,&QSplitter::splitterMoved,[=](int pos, int index){
        Q_UNUSED(pos);
        Q_UNUSED(index);
        if(consoleHidden_() == false){
            consoleWidget->setGeometry(ui->playerWidget->rect());
        }else{
            consoleWidget->resize(ui->playerWidget->rect().size());
            consoleWidget->move(QPoint(ui->playerWidget->rect().x(), ui->playerWidget->rect().y()-ui->playerWidget->rect().height()));
        }
    });

    ui->splitZoneLayout->addWidget(split1);

    ui->url->addAction(QIcon("://icons/links-line.png"),QLineEdit::LeadingPosition);
    ui->volumeWidget->hide();
    ui->cancel->hide();
    ui->url->hide();

    QString path = settings.value("destination",
                   QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                                  +"/"+QApplication::applicationName()).toString();
    QFileInfo pathInfo(path);
    if(pathInfo.exists()==false){
        QDir dir;
        if(dir.mkpath(path)){
            ui->location->setText(path);
        }
    }else{
      ui->location->setText(path);
    }

    init_player();

    rsH = new RangeSlider(Qt::Horizontal, RangeSlider::Option::DoubleHandles, nullptr);

    connect(rsH,&RangeSlider::lowerValueChanged,[=](int lValue)
    {
        lValue = lValue/1000;
        int seconds = (lValue) % 60;
        int minutes = (lValue/60) % 60;
        int hours = (lValue/3600) % 24;
        QTime time(hours, minutes,seconds);
        ui->startDur->setText(time.toString());
        updateDuration();
    });

    connect(rsH,&RangeSlider::upperValueChanged,[=](int uValue)
    {
        uValue = uValue/1000;
        int seconds = (uValue) % 60;
        int minutes = (uValue/60) % 60;
        int hours = (uValue/3600) % 24;
        QTime time(hours, minutes,seconds);
        ui->endDur->setText(time.toString());
        updateDuration();
    });

    rsH->SetRange(0,100);

    ui->rangeLayout->addWidget(rsH);

    ui->scale->setValue(settings.value("width","400").toInt());
    ui->loop->setChecked(settings.value("loop",true).toBool());
    ui->loopCount->setEnabled(settings.value("loop",true).toBool());
    ui->loopCount->setValue(settings.value("loopCount",0).toInt());
    ui->infiniteLoop->setEnabled(settings.value("loopCount",0).toInt()!=0);
    ui->infiniteLoop->setEnabled(settings.value("loop",true).toBool()==true);


    consoleButton = ui->consoleButton;
    consoleButton->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    consoleButton->setMouseTracking(true);
    consoleButton->setToolTip("Hide console");
    consoleButton->setIconSize(QSize(10,10));
    consoleButton->setStyleSheet("border:none");
    connect(consoleButton,&controlButton::clicked,[=](){
        if(consoleHidden_()==true){
            showConsole();
        }else{
            hideConsole();
        }
    });

    connect(this,&VideoProcessor::consoleVisibilityChanged,[=](const bool isVisible){
            consoleButton->setIcon(isVisible ? QIcon(":/icons/others/arrow-down-s-line-cropped.png"):QIcon(":/icons/others/arrow-up-s-line-cropped.png"));
            consoleButton->setToolTip(isVisible ? "Show console":"Hide console");
    });

    setConsoleHidden(false);
    showStatus("<html><head/><body style='color:lightgray'><p align='center'><br>Welcome to "
               +utils::toCamelCase(QApplication::applicationName())
               +"</p><p align='center'>version : "+QApplication::applicationVersion()
               +"</p><p align='center'>Developed by- Keshav Bhatt</p><p align='center'><a style='color: lightgray' href='mailto:keshavnrj@gmail.com?subject="+
               utils::toCamelCase(QApplication::applicationName())+"'>keshavnrj@gmail.com</a></p>"
               "<p align='center'><a style='color: lightgray' href='https://snapcraft.io/search?q=keshavnrj'>More Applications</a></p></body></html>");
}

void VideoProcessor::setSource(QString sourceUrl)
{
    ui->url->setText(sourceUrl);
    QTimer::singleShot(100,[=](){
          resizeFix();
          start();
    });
}


void VideoProcessor::init_player()
{
    //init media player
    player = new QMediaPlayer(this);

    QVideoWidget* videoWidget = new QVideoWidget;
    videoWidget->setObjectName("videoWidget");
    videoWidget->setMinimumSize(210,140);
    videoWidget->setMouseTracking(false);
    videoWidget->setAttribute(Qt::WA_Hover, false);
    videoWidget->setStyleSheet("background-color:black");
    player->setVideoOutput(videoWidget);
    ui->playerLayout->addWidget(videoWidget);
    videoWidget->show();

    //init console Ui
    consoleWidget = new QWidget(ui->playerWidget);
    consoleUi.setupUi(consoleWidget);
    connect(consoleUi.textBrowser,&QTextBrowser::anchorClicked,[=](const QUrl link){
        qWarning()<<"clicked"<<link;
        QProcess *xdg_open = new QProcess(this);
        xdg_open->start("xdg-open",QStringList()<<link.toString());
        if(xdg_open->waitForStarted(1000) == false){
                //try using QdesktopServices
                bool opened = QDesktopServices::openUrl(link);
                if(opened==false){
                    consoleUi.textBrowser->append("<br><i style='color:red'>Failed to open '"+link.toString()+"'</i>");
                    qWarning()<<"failed to open url"<<link;
                }
        }
        connect(xdg_open, static_cast<void (QProcess::*)
                (int,QProcess::ExitStatus)>(&QProcess::finished),
                [this,xdg_open]
                (int exitCode,QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode);
            Q_UNUSED(exitStatus);
            xdg_open->close();
            xdg_open->deleteLater();
        });
    });


    // loader is the child of wall_view
    _loader = new WaitingSpinnerWidget(ui->loader,true,false);
    _loader->setRoundness(70.0);
    _loader->setMinimumTrailOpacity(15.0);
    _loader->setTrailFadePercentage(70.0);
    _loader->setNumberOfLines(10);
    _loader->setLineLength(8);
    _loader->setLineWidth(2);
    _loader->setInnerRadius(2);
    _loader->setRevolutionsPerSecond(3);
    _loader->setColor(QColor("#1e90ff"));

    connect(player,&QMediaPlayer::volumeChanged,[=](int volume){
       ui->vl->setText((QString::number(volume).length()<2 ? "0":"")+QString::number(volume));
       ui->volume->setValue(volume);
       settings.setValue("volume",volume);
    });

    player->setVolume(settings.value("volume",50).toInt());

    connect(ui->playerSeekSlider,&seekSlider::setPosition,[=](QPoint localPos){
        ui->playerSeekSlider->blockSignals(true);
        int pos = ui->playerSeekSlider->minimum() + ((ui->playerSeekSlider->maximum()-ui->playerSeekSlider->minimum()) * localPos.x()) / ui->playerSeekSlider->width();
        QPropertyAnimation *a = new QPropertyAnimation(ui->playerSeekSlider,"value");
        a->setDuration(150);
        a->setStartValue(ui->playerSeekSlider->value());
        a->setEndValue(pos);
        a->setEasingCurve(QEasingCurve::Linear);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        player->setPosition(pos*1000);
        ui->playerSeekSlider->blockSignals(false);
    });

    connect(ui->playerSeekSlider,&seekSlider::showToolTip,[=](QPoint localPos){
            int pos = ui->playerSeekSlider->minimum() + ((ui->playerSeekSlider->maximum()-ui->playerSeekSlider->minimum()) * localPos.x()) / ui->playerSeekSlider->width();
            int seconds = (pos) % 60;
            int minutes = (pos/60) % 60;
            int hours = (pos/3600) % 24;
            QTime time(hours, minutes,seconds);
            QToolTip::showText(ui->playerSeekSlider->mapToGlobal(localPos), "Seek: "+time.toString());
         });

    connect(player,&QMediaPlayer::durationChanged,[=](qint64 dur){
        dur = dur/1000;
        int seconds = (dur) % 60;
        int minutes = (dur/60) % 60;
        int hours = (dur/3600) % 24;
        QTime time(hours, minutes,seconds);
        ui->duration->setText(time.toString());
        ui->playerSeekSlider->setMaximum(dur);
        rsH->setMaximum(dur*1000);
    });

    connect(player,&QMediaPlayer::positionChanged,[=](qint64 pos){
       pos = pos/1000;
       int seconds = (pos) % 60;
       int minutes = (pos/60) % 60;
       int hours = (pos/3600) % 24;
       QTime time(hours, minutes,seconds);
       ui->position->setText(time.toString());
       ui->playerSeekSlider->setValue(pos);
       if(isPlayingPreview == true){
           ui->preview->setIcon(QIcon(":/icons/pause-line.png"));
           if(pos == rsH->GetUpperValue()/1000){
               player->blockSignals(true);
               player->pause();
               ui->play->setIcon(QIcon(":/icons/play-line.png"));
               ui->preview->setIcon(QIcon(":/icons/play-line.png"));
               isPlayingPreview = false;
               player->blockSignals(false);
           }
       }
    });

    connect(player,&QMediaPlayer::stateChanged,[=](QMediaPlayer::State state){
        if (state == QMediaPlayer::PlayingState || state == QMediaPlayer::StoppedState)
            _loader->stop();
        ui->play->setIcon(QIcon(state == (QMediaPlayer::PlayingState||QMediaPlayer::BufferedMedia) ? ":/icons/pause-line.png":":/icons/play-line.png"));
        if(isPlayingPreview)
        ui->preview->setIcon(QIcon(state == (QMediaPlayer::PlayingState||QMediaPlayer::BufferedMedia) ? ":/icons/pause-line.png":":/icons/play-line.png"));
    });

    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
          [=](QMediaPlayer::Error error){
        Q_UNUSED(error);
        QString error_string = player->errorString();
        showError(error_string);

        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
    });

    connect(player,&QMediaPlayer::mediaStatusChanged,[=](QMediaPlayer::MediaStatus mediastate){
//        qDebug()<<mediastate;
        (mediastate == QMediaPlayer::BufferingMedia
                || mediastate == QMediaPlayer::LoadingMedia
                || mediastate == QMediaPlayer::StalledMedia
                || mediastate == QMediaPlayer::LoadingMedia)
                ?  _loader->start() : _loader->stop();
    });
}

void VideoProcessor::hideConsole()
{
    qDebug()<<"hideConsolecalled"<<sender();
    if(consoleHidden_())
        return;
    QPropertyAnimation *animation = new QPropertyAnimation(consoleWidget, "pos");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->setStartValue(QPoint(consoleWidget->x(), consoleWidget->y()));
    animation->setEndValue(QPoint(consoleWidget->x(), (consoleWidget->y())-consoleWidget->height()));
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    consoleButton->setEnabled(animation->state()!=QPropertyAnimation::Running);
    connect(animation,&QPropertyAnimation::finished,[=](){
        consoleButton->setEnabled(animation->state()!=QPropertyAnimation::Running);
        setConsoleHidden(true);
    });
    qDebug()<<"2hideConsolecalled"<<sender();
}

void VideoProcessor::showConsole()
{
    if(!consoleHidden_())
        return;
    QPropertyAnimation *animation = new QPropertyAnimation(consoleWidget, "pos");
    animation->setDuration(500);
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->setStartValue(QPoint(consoleWidget->x(), consoleWidget->y()));
    animation->setEndValue(QPoint(consoleWidget->x(), (consoleWidget->y())+consoleWidget->height()));
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    consoleButton->setEnabled(animation->state()!=QPropertyAnimation::Running);
    connect(animation,&QPropertyAnimation::finished,[=](){
       consoleButton->setEnabled(animation->state()!=QPropertyAnimation::Running);
       setConsoleHidden(false);
    });
    qDebug()<<"showconsolecalled"<<sender();
}

void VideoProcessor::resizeFix()
{
    this->resize(this->width()+1,this->height());
}

void VideoProcessor::closeEvent(QCloseEvent *event)
{
    //clear locatemp dir
    QDir(utils::returnPath("localTemp")).removeRecursively();
    QWidget::closeEvent(event);
}

void VideoProcessor::resizeEvent(QResizeEvent *event)
{
    if(consoleHidden_() == false){
        consoleWidget->setGeometry(ui->playerWidget->rect());
    }else{
        consoleWidget->resize(ui->playerWidget->rect().size());
        consoleWidget->move(QPoint(ui->playerWidget->rect().x(), ui->playerWidget->rect().y()-ui->playerWidget->rect().height()));
    }
    QWidget::resizeEvent(event);
}

//read
bool VideoProcessor::consoleHidden_() const
{
    return consoleHidden;
}


void VideoProcessor::updateDuration()
{
    int dur = rsH->GetUpperValue() - rsH->GetLowerValue();
    dur = dur/1000;
    if(dur > 0)
    {
        int seconds = (dur) % 60;
        int minutes = (dur/60) % 60;
        int hours = (dur/3600) % 24;
        QTime time(hours, minutes,seconds);
        ui->clip_duration->setText(time.toString());
        ui->preview->setEnabled(true);
        ui->clip->setEnabled(true);
    }else
    {
        ui->clip_duration->setText("invalid");
        ui->preview->setEnabled(false);
        ui->clip->setEnabled(false);
    }
}


void VideoProcessor::showError(QString message)
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

void VideoProcessor::setStyle(QString fname)
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

VideoProcessor::~VideoProcessor()
{
    if(ffmpegProcess!=nullptr){
        ffmpegProcess->blockSignals(true);
        ffmpegProcess->close();
        ffmpegProcess->disconnect();
        ffmpegProcess->deleteLater();
    }
    delete ui;
}

void VideoProcessor::setConsoleHidden(bool hidden)
{
    consoleHidden = hidden;
    emit consoleVisibilityChanged(hidden);
}

void VideoProcessor::on_url_textChanged(const QString &arg1)
{
    if(arg1.isEmpty()==false){
        settings.setValue("lastUrl",arg1);
    }
}

void VideoProcessor::start()
{
    if(clipOptionChecker()) return;

    isPlayingPreview = false;
    currentFileName = "";
    ui->clipname->clear();
    rsH->SetRange(0,100);
    player->stop();

    QString resouceUrl = ui->url->text().trimmed().simplified();
    consoleUi.textBrowser->clear();
    //handle local files
    if( resouceUrl.at(0) == "/"){
        resouceUrl.prepend("file://");
    }

    if(resouceUrl.contains("file://")){
        #ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(Qt::BusyCursor);
        #endif

        QString ext = QFileInfo(resouceUrl).completeSuffix();
        //clear localTemp
        QDir(utils::returnPath("localTemp")).removeRecursively();
        //copy file to temp location with name reanamed
        QString tempPath = utils::returnPath("localTemp");
        QString temFileName = utils::generateRandomId(10)+"."+ext;
        QString srcFileName = QString(ui->url->text().trimmed().simplified()).remove("file://");
        QString destFileName = tempPath+temFileName;
        if(QFileInfo(srcFileName).exists()){
            if(QFile::copy(srcFileName,destFileName)){
                #ifndef QT_NO_CURSOR
                    QApplication::restoreOverrideCursor();
                #endif
                int dotPos2 = destFileName.lastIndexOf(".",-1);
                //change resource file to temp resouce.
                resouceUrl = "file://"+destFileName;
                currentFileName = QUrl(destFileName.left(dotPos2)).fileName();
            }
        }else{
            #ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
            #endif
        }

       // hideConsole();
        if(!currentFileName.isEmpty()){
            ui->clipname->setText(currentFileName+".gif");
        }else{
            ui->clipname->setText(utils::generateRandomId(10)+".gif");
        }
        playMedia(resouceUrl);
    }
}

void VideoProcessor::playMedia(QString url){

    player->setMedia(QUrl(url));
    player->play();
    hideConsole();
}

void VideoProcessor::on_pickStart_clicked()
{
    if(clipOptionChecker()) return;
    int pos = player->position();
    ui->startDur->setText(QString::number(pos));
    rsH->SetLowerValue(pos);
}

void VideoProcessor::on_pickEnd_clicked()
{
    if(clipOptionChecker()) return;
    int pos = player->position();
    ui->endDur->setText(QString::number(pos));
    rsH->SetUpperValue(pos);
}

void VideoProcessor::on_play_clicked()
{
    if(clipOptionChecker()) return;
    hideConsole();
    if(isPlayingPreview){
        isPlayingPreview = false;
        ui->preview->setIcon(QIcon(":/icons/play-line.png"));
    }
    if(player->state()!=QMediaPlayer::PausedState){
        player->pause();
    }
    else if(player->state()==QMediaPlayer::PausedState){
        player->play();
    }
}

void VideoProcessor::on_minusOneSecLower_clicked()
{
    if(clipOptionChecker()) return;
    int rsL = rsH->GetLowerValue();
    rsH->SetLowerValue(rsL-1000);
}

void VideoProcessor::on_plusOneSecLower_clicked()
{
    if(clipOptionChecker()) return;
    int rsL = rsH->GetLowerValue();
    rsH->SetLowerValue(rsL+1000);
}

void VideoProcessor::on_minusOneSecUpper_clicked()
{
    if(clipOptionChecker()) return;
    int rsU = rsH->GetUpperValue();
    rsH->SetUpperValue(rsU-1000);
}

void VideoProcessor::on_plusOneSecUpper_clicked()
{
    if(clipOptionChecker()) return;
    int rsU = rsH->GetUpperValue();
    rsH->SetUpperValue(rsU+1000);
}

void VideoProcessor::on_volume_valueChanged(int value)
{
    player->setVolume(value);
    if(value>0) tempVolume = value;
    ui->vIcon->setIcon(value == 0 ? QIcon(":/icons/volume-mute-line.png"):QIcon(":/icons/volume-up-line.png"));
}

void VideoProcessor::on_moveToFrameLower_clicked()
{
    if(clipOptionChecker()) return;
    hideConsole();
    player->setPosition(rsH->GetLowerValue());
    player->pause();
}

void VideoProcessor::on_moveToFrameUpper_clicked()
{
    if(clipOptionChecker()) return;
    hideConsole();
    player->setPosition(rsH->GetUpperValue());
    player->pause();
}

bool VideoProcessor::clipOptionChecker(){
    bool check = false;
    if(ffmpegProcess != nullptr && ffmpegProcess->state() == QProcess::Running){
        showError("This option is not available while 'Clip' process is running.");
        check = true;
    }else{
        check = false;
    }
    return check;
}

void VideoProcessor::on_preview_clicked()
{
    if(clipOptionChecker()) return;
    hideConsole();
    if(isPlayingPreview && player->state() == QMediaPlayer::PlayingState)
    {
        isPlayingPreview = false;
        player->pause();
        ui->preview->setIcon(QIcon(":/icons/play-line.png"));
    }else
    {
        isPlayingPreview = true;
        on_moveToFrameLower_clicked();
        player->play();
    }
}



void VideoProcessor::on_vIcon_clicked()
{
    ui->volume->setValue(ui->volume->value()>0
                         ? 0:tempVolume);
}

void VideoProcessor::on_changeLocationButton_clicked()
{
    if(clipOptionChecker()) return;
    QString path = settings.value("destination",
                   QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                   .toString()+"/"+QApplication::applicationName();

    QString destination = QFileDialog::getExistingDirectory(this, tr("Select destination directory"), path,QFileDialog::ShowDirsOnly);
    QFileInfo dir(destination);
    if(dir.isWritable()){
        ui->location->setText(destination);
    }else{
        showError("Destination directory is either not writable or user cancelled action.<br>Please choose a valid location.");
    }
}

void VideoProcessor::on_location_textChanged(const QString &arg1)
{
    if(arg1.isEmpty()==false){
        settings.setValue("destination",arg1);
        ui->clip->setEnabled(true);
    }else{
        ui->clip->setEnabled(false);
    }
}

void VideoProcessor::on_clip_clicked()
{
    player->pause();
    isPlayingPreview = false;
    ui->preview->setIcon(QIcon(":/icons/play-line.png"));

    if(ffmpegProcess!= nullptr)
    {
        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
        ffmpegProcess->blockSignals(true);
        ffmpegProcess->close();
        ffmpegProcess->disconnect();
        ffmpegProcess->deleteLater();
        ffmpegProcess->blockSignals(false);
        ffmpegProcess = nullptr;
    }
    consoleUi.textBrowser->clear();
    QString fps = settings.value("framerate","10").toString();
    QString scale = settings.value("gifScale","400").toString();
    QString loop = settings.value("loop",true).toBool()?settings.value("loopCount","0").toString():"-1";
    QStringList args;

    args<<"-c"<<"ffmpeg -ss "+ui->startDur->text().trimmed()+" -t "
          +ui->clip_duration->text().trimmed()+" -i \""
          +player->media().canonicalUrl().toString()+"\" -vf \"fps="+fps+",scale="+scale+":-1:flags="
            "lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" -loop "+loop+" \""+
             ui->location->text().trimmed()+"/"+ui->clipname->text().trimmed()+"\" -y";

    ffmpegProcess= new QProcess(this);
    ffmpegProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(ffmpegProcess,&QProcess::readyRead,[=](){
        QString output = ffmpegProcess->readAll();
        if(output.contains("frame= ",Qt::CaseInsensitive))
        consoleUi.textBrowser->append("<i style='color:skyblue'>"+output.replace("\n","<br>")+"</i>");
    });
    connect(ffmpegProcess, static_cast<void (QProcess::*)
            (int,QProcess::ExitStatus)>(&QProcess::finished),
            [this]
            (int exitCode,QProcess::ExitStatus exitStatus)
    {
        Q_UNUSED(exitStatus);
        if(exitCode==0)
        {
            consoleUi.textBrowser->append("<br><i style='color:lightgreen'>Download Finished.</i>\n");
            QString path = settings.value("destination",
                           QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                                          +"/"+QApplication::applicationName()).toString();
            QString filepath = path+"/"+ui->clipname->text();
            consoleUi.textBrowser->append("<br><a style='color:skyblue' href='file://"+path+"'>Open file location</a> "
                        "or <a style='color:skyblue' href='file://"+filepath+"'>Play file</a><br>");
        }else
        {
            consoleUi.textBrowser->append("<br><i style='color:red'>An error occured while processing your request</i>\n");
            showError("Process exited with code "+QString::number(exitCode)+"\n"+ffmpegProcess->readAll());
        }
        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
        ui->cancel->hide();
        ui->clip->show();
    });
    ffmpegProcess->start("bash",args);
    if(ffmpegProcess->waitForStarted(1000)){
        ui->clip->hide();
        ui->cancel->show();
        showConsole();
        #ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(Qt::BusyCursor);
        #endif
        consoleUi.textBrowser->clear();
        consoleUi.textBrowser->append("<i style='color:skyblue'>Initializing trimming process...</i>\n");
    }else{
        consoleUi.textBrowser->clear();
        consoleUi.textBrowser->append("<i style='color:red'>Failed to start trimming process.</i>\n");
    }
}

QString VideoProcessor::getCodec()
{
    QString code;
    if(settings.value("codec","mpeg").toString().contains("copy",Qt::CaseInsensitive)){
        code = " -c copy ";
    }else{
        code = " -c:v libx264 -c:a aac ";
    }
    qDebug()<<"used encoder:"<<code;
    return code;
}

void VideoProcessor::on_cancel_clicked()
{
    if(ffmpegProcess!= nullptr)
    {
        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
        ffmpegProcess->close();
        ffmpegProcess->disconnect();
        ffmpegProcess->deleteLater();
        ffmpegProcess = nullptr;
        ui->cancel->hide();
        ui->clip->show();
    }
}


void VideoProcessor::showStatus(QString message)
{
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    consoleUi.textBrowser->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
    a->setDuration(1000);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InCurve);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    consoleUi.textBrowser->setText(message);
    consoleUi.textBrowser->show();
}

void VideoProcessor::on_screenshotLowerFrame_clicked()
{
    this->takeScreenshot();

}

void VideoProcessor::on_screenshotUpperFrame_clicked()
{
    this->takeScreenshot();
}

void VideoProcessor::takeScreenshot(){

    if(clipOptionChecker()) return;
    //get sender
    QPushButton *senderBtn = qobject_cast<QPushButton*>(sender());
    if(senderBtn == nullptr)
        return;
    bool isUpper = senderBtn->objectName().contains("UpperFrame",Qt::CaseInsensitive);
    player->pause();
    if(screenshotProcess!= nullptr)
    {
        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
        screenshotProcess->blockSignals(true);
        screenshotProcess->close();
        screenshotProcess->disconnect();
        screenshotProcess->deleteLater();
        screenshotProcess->blockSignals(false);
        screenshotProcess = nullptr;
    }

    QString format = "."+settings.value("sc_format","png").toString();
    QDir dir(utils::returnPath("screenshots"));
    dir.removeRecursively();//empty dir before to save space.
    QString tempScLocation = utils::returnPath("screenshots");
    QString fileLocation = tempScLocation+"/"+currentFileName+format;
    qDebug()<<fileLocation;
    QStringList args;
    args<<"-c"<<"ffmpeg -ss "+QString(isUpper ? ui->endDur->text().trimmed():ui->startDur->text().trimmed())+
          " -i \""+player->media().canonicalUrl().toString()+"\" -vframes 1 \""+
          fileLocation+"\" -y";
    screenshotProcess= new QProcess(this);
    connect(screenshotProcess,&QProcess::readyRead,[=](){
        QString output = screenshotProcess->readAll();
        showConsole();
        if(output.contains("frame= ",Qt::CaseInsensitive))
        consoleUi.textBrowser->append("<i style='color:skyblue'>"+output.replace("\n","<br>")+"</i>");
    });
    connect(screenshotProcess, static_cast<void (QProcess::*)
            (int,QProcess::ExitStatus)>(&QProcess::finished),
            [this,fileLocation]
            (int exitCode,QProcess::ExitStatus exitStatus)
    {
        Q_UNUSED(exitStatus);
        #ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
        #endif
            if(exitCode==0){
                //init screenshot
                screenshot = new Screenshot(this,fileLocation);
                screenshot->setWindowTitle(QApplication::applicationName()+" | Screenshot capture");
                screenshot->setAttribute(Qt::WA_DeleteOnClose);
                screenshot->setWindowModality(Qt::ApplicationModal);
                screenshot->setWindowFlag(Qt::Dialog);
                connect(screenshot,&Screenshot::savedScreenshot,[=](QString screenshotLocation){
                    consoleUi.textBrowser->append("\n<i style='color:lightgreen'>Screenshot taken hiding console in 10 seconds...</i>\n");
                    QTimer::singleShot(10000,this,SLOT(hideConsole()));
                    QString path = settings.value("sc_location",
                                   QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                                                  +"/"+QApplication::applicationName()).toString();
                    consoleUi.textBrowser->append("<br><a style='color:skyblue' href='file://"+path+"'>Open file location</a> "
                                "or <a style='color:skyblue' href='file://"+screenshotLocation+"'>Open file</a><br>");
                });
                connect(screenshot,&Screenshot::failedToSaveSc,[=](){
                    consoleUi.textBrowser->append("<br><i style='color:red'>Screenshot operation cancelled, hiding console in 4 seconds...</i>\n");
                    QTimer::singleShot(4000,this,SLOT(hideConsole()));
                });
                screenshot->show();
            }else{
                showError("Unable to take screenshot, process returned :\n\n"+screenshotProcess->readAllStandardError());
            }
    });
    screenshotProcess->start("bash",args);
    if(screenshotProcess->waitForStarted(1000)){
        showConsole();
        #ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(Qt::BusyCursor);
        #endif
        consoleUi.textBrowser->clear();
        consoleUi.textBrowser->append("<i style='color:skyblue'>Taking screenshot please wait...</i>\n");
    }else{
        showError("Unable to take screenshot, not able to start process");
    }
}


void VideoProcessor::on_fps_valueChanged(int arg1)
{
    settings.setValue("framerate",arg1);
}

void VideoProcessor::on_scale_valueChanged(int arg1)
{
    settings.setValue("gifScale",arg1);
}

void VideoProcessor::on_loop_toggled(bool checked)
{
    ui->loopCount->setEnabled(checked);
    settings.setValue("loop",checked);
    ui->infiniteLoop->setEnabled(checked);
}

void VideoProcessor::on_loopCount_valueChanged(int arg1)
{

    ui->infiniteLoop->setEnabled(arg1!=0);

    settings.setValue("loopCount",arg1);
}

void VideoProcessor::on_resetScale_clicked()
{
    ui->scale->setValue(settings.value("width",400).toInt());
}

void VideoProcessor::on_infiniteLoop_clicked()
{
    ui->loopCount->setValue(0);
    ui->infiniteLoop->setEnabled(false);
}

void VideoProcessor::on_halfScale_clicked()
{
    ui->scale->setValue(settings.value("width",400).toInt()/2);
}
