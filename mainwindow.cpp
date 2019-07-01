#include "mainwindow.h"
#include "ui_anykey.h"
#include <iostream>
#include <qprocess.h>
#include <qbytearray.h>
#include <qdebug.h>
#include <qdir.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <quuid.h> 

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

using namespace std;

//exe name of anykey_save
#define ANYKEY_SAVE "anykey_save"
#define ANYKEY_TYPE_DAEMON "anykey_crd"

bool MainWindow::checkLicense() {
    if(licenseRegistered){
        return true;
    }
    qDebug()<<"checking license..."<<endl;
    //run this ./anykey_save -license
    // returns 'ok' or 'invalid license'
    QString home_path = QCoreApplication::applicationDirPath();

    QProcess anykey_save;
    anykey_save.setProcessChannelMode(QProcess::MergedChannels);
    anykey_save.setWorkingDirectory(home_path);
    anykey_save.start(home_path+"/"+ANYKEY_SAVE, QStringList() << "-license" );
    // above works in windows, here on prev mac version we used:
    // anykey_save.start(home_path+"/../Helpers/"+ANYKEY_SAVE, QStringList() << "-license" );


    if (!anykey_save.waitForStarted()){
        QMessageBox::critical(this, tr("AnyKey"),
                                                tr("Installation error\n"
                                                "The anykey_save utility is not found in it's location:")+
                                                home_path+
                                                tr("\nPlease try re-installing the AnyKey software."),
                                                QMessageBox::Ok );
        setStatus("Installation error!");
        return false;
    }

    if(!anykey_save.waitForFinished()){
        //setStatus("Write error:"+anykey_save.errorString());
        setStatus("Error while calling anykey savetool error 9000!");
        return false;
    }

    QByteArray save_output = anykey_save.readAll();
    QString output_str = QString(save_output);

    //setStatus("license check output: "+output_str);
    if( output_str.contains("invalid") ){
        return false;
    }

    if( output_str.contains("ok")){
        licenseRegistered = true;
        return true;
    }

    return false;
}

void MainWindow::updateStatusSlot(){

    QString msg = QString("    ")+this->statusMessage;

    qDebug()<<" -> statusbar message: "<<msg<<endl;

    //ui->statusbar->showMessage( msg, 3 ); //also does not show our messag :(
    ui->statusbar->showMessage( msg, 3 ); //also does not show our messag :(

}

void MainWindow::setStatus(const QString& msg){
    qDebug()<<"status: "<<msg<<endl;
    statusBar()->showMessage(msg, 5000);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    licenseRegistered = false;
    anykeycrd = new QProcess();

    this->setMinimumWidth(425);
    ui->setupUi(this);

    // Set status and progress bar
    // create objects for the label and progress bar
    statusLabel = new QLabel(this);
    //statusProgressBar = new QProgressBar(this);
    // make progress bar text invisible
    //statusProgressBar->setTextVisible(false);

    // add the two controls to the status bar
    ui->statusbar->addPermanentWidget(statusLabel,0);
    //ui->statusbar->addPermanentWidget(statusProgressBar,1);


    ui->titleLabel->setText("");
    setStatus("Checking license...");

    // read license file here!!!
    if( !checkLicense() ){
        ui->titleLabel->setText("Enter activation code");
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        ui->showPassword->hide();
        ui->generateButton->hide();
        ui->advancedSettingsToggle->hide();
        ui->copyProtectToggle->hide();
        ui->generateLength->hide();
        ui->addReturn->hide();
        ui->saveButton->setText("Register");
        setStatus("");
        hideAdvancedItems();
        ui->passwordEdit->setFocus();
        ui->menubar->hide();
    }
    else{
        showRegisteredControls();
        //runCommand("read_settings\n"); //whenever anykey is inserted,read its settings
    }

}

void MainWindow::readCRD(){
    QString output = anykeycrd->readAllStandardOutput();
    QStringList lines = output.split("\n");

    for(int i=0;i<lines.length();i++){
        QString line = lines.at(i);
        if(line.length()==0) continue;
        if(line!="cmd: "){ //only print interesting lines on debug
            qDebug() << "READ: '" << line << "'";
        }
        if(line.contains("connected")){
            //qDebug() << "line='"<<line<<"'"<<endl;
            ui->saveButton->setEnabled(true);
            ui->applyAdvancedSettingsBtn->setEnabled(true);
            ui->typeButton->setEnabled(true);
            ui->formatEepromButton->setEnabled(true);
            ui->updateSaltButton->setEnabled(true);
        }

        if(line.contains("connected cmd:")){ //first connect
            if(ui->daemonAutoType->isChecked()){
                anykeycrd->write("type\n");
                setStatus("AnyKey detected doing CR for typing password...");
            }
            else{
                setStatus("AnyKey detected");
                anykeycrd->write("none\n");
            }
            line="";
            ui->passwordEdit->selectAll();
            ui->passwordEdit->setFocus();
        }
        else if(line.contains("cmd:")){
            if(commands_queue.length() > 0){
                if(commands_queue.at(0).contains("read_settings")) setStatus("Reading settings...");
                anykeycrd->write(commands_queue.at(0).toLocal8Bit());
                commands_queue.removeFirst();
            }
            else{
                anykeycrd->write("\n"); //none or empty string skips command
            }
            line="";
        }

        if( line.contains("reset_response=Whiping eeprom (FF)eeprom cleared")){
            setStatus("AnyKey reset to factory settings. Remove and re-insert");
            showMessage("Factory reset succeeded", "Please re-insert your AnyKey to reset and read new settings.");
            line="";
        }

        if( line.contains("settings=") ){
            if(!line.contains("kbd_conf=")){
                setStatus("ERROR: could not read settings.");
                showMessage("Read settings failed","Try toggling advanced settings to re-read if that does not work, re-insert your AnyKey and try again.");
            }
            else{
                anykeyParseSettings(line);
                line=""; //clear line so next ifs don't catch false cmd
            }
        }

        if(line.startsWith("salt saved")){
            setStatus("Salt saved");
            line="";
        }
        if(line.startsWith("salt=")){
            setStatus("device linked again");
            runCommand("read_settings\n");
            line="";
        }

        if( line.startsWith("config saved")){
            setStatus("Configuration saved");
            line="";
        }
        if( line.startsWith("password saved")){
            setStatus("Password saved");
            ui->passwordEdit->selectAll(); //setText(""); //clear it again
            ui->passwordEdit->setFocus();
            line="";
        }
        if( line.startsWith("flags saved")){
            setStatus("Flags saved");
            ui->passwordEdit->selectAll(); //select all allows delete when wanted
            ui->passwordEdit->setFocus();
            line="";
        }
        if( line.contains("ERROR: Invalid CR")){
            showMessage("Invalid Challenge Response",
                        "Please re-insert your AnyKey and try again. Or configure your salt.");
            line="";
        }
        if( line.contains("ERROR: password not saved")){
            showMessage("Save error", "Please re-insert your AnyKey and try again.");
            setStatus("ERROR during save. Please try again!");
            line="";
        }
        if( line.startsWith("salt saved to device")){
            parseWriteSaltResponse(line);
            line="";
        }

        if(line.contains("id=")){ //"id=363739"
            setStatus("CP id received");
            ui->passwordEdit->setFocus();
            showMessage("AnyKey autotype","AnyKey protected challenge response...");
        }
        // " challenge=Cf87b2afc8057e90c6e8a684f52febd53da97ddd3a91ddc28b1b0b52d193024  [679] response=f7081b0635a0ef78ccf180c7e364079dc4cf4a29f6609503d46a37b3ae0250c4"
        if(line.contains("challenge =")){
            setStatus("CP handling challenge response...");
            //ui->copyProtectToggle->setChecked(true);
        }
        if(line.contains("done")){
            setStatus("AnyKey typed it's password after a CR");
            line="";
        }

        if(line.startsWith("disconnect")){
            setStatus("Anykey disconnected");
            ui->saveButton->setEnabled(false);
            ui->applyAdvancedSettingsBtn->setEnabled(false);
            ui->typeButton->setEnabled(false);
            ui->formatEepromButton->setEnabled(false);
            ui->updateSaltButton->setEnabled(false);

            line="";
        }

        if(line.startsWith("wrong password")){
            setStatus("Read salt failed");
            showMessage("Wrong password given for readsalt", "You need to give the original saved password in the password field. \nThen click on readsalt to recover or link a foreign device.");
            line="";
        }
    }
}

void MainWindow::startAnykeyCRD()
{
    QString home_path = QCoreApplication::applicationDirPath();
    anykeycrd->setProcessChannelMode(QProcess::MergedChannels);
    anykeycrd->setWorkingDirectory(home_path);
    anykeycrd->start(home_path+"/"+ANYKEY_TYPE_DAEMON, QStringList() << "-gui" );

    connect(anykeycrd, SIGNAL(readyReadStandardOutput()), this, SLOT(readCRD()));
    //connect(anykeycrd, SIGNAL(readyW))
    anykeycrd->waitForStarted();
    ui->daemonStatusLabel->setText("running");
}

void MainWindow::stopAnykeyCRD(){
    anykeycrd->kill();
    anykeycrd->waitForFinished();
    ui->daemonStatusLabel->setText("stopped");
}

void MainWindow::showRegisteredControls(){
    ui->titleLabel->setText("Insert your AnyKey then type a password and click on save:");
    setStatus("");
    ui->passwordEdit->setEchoMode(QLineEdit::Password);
    //ui->passwordEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    ui->showPassword->show();
    ui->generateButton->show();
    ui->generateLength->show();
    ui->addReturn->show();
    ui->advancedSettingsToggle->show();
    ui->menubar->show();
#ifdef _WIN32
    //todo change actionOpen text
    //QAction* openAction = menuBar()->findChild<QMenu*>("menuAnyKey")->findChild<QAction*>("actionOpen");
    //openAction->setShortcut(Qt::CTRL + Qt::Key_O);
#else
    // on mac it should be qt::ctrl+qt::key_comma ...
#endif

    hideAdvancedItems();

    ui->copyProtectToggle->setDisabled(true);

    ui->saveButton->setText("Save");
    ui->generateLength->setValue(20);

    createActions();
    createTrayIcon();
    setIcon(0);
    trayIcon->show();

    startAnykeyCRD(); //challenge response
    qDebug() << "startAnyKeyCRD..." <<endl;

    //ui->passwordEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    ui->passwordEdit->setFocus();

    QStringList args = QCoreApplication::arguments();
    if( ( args.length()>1 ) && args.at(1).contains( "minimized") ){
      QTimer::singleShot ( 10, this, SLOT(hide() ) );
    }
}

void MainWindow::showConfigurator(){
    this->showNormal();
    this->show();
    this->raise(); //bring on top for Mac OS
#ifdef _WIN32
    this->activateWindow(); // appWindow->requestActivate(); -> for Windows
#endif

    runCommand("read_settings\n"); //whenever anykey is inserted,read its settings
}

void MainWindow::typePasswordAgain(){
    //qDebug() << "clicked type password again!"<<endl;
    bPendingPasswordType = true;
    showMessage("Type password",
                "Inserted AnyKey will do Challenge Response and type password after window loses focus", 8);
}

void MainWindow::appFocusChanged(Qt::ApplicationState state){
    //qDebug() << "Application state is now = "<< state;
    if( bPendingPasswordType ){
        //Inactive means lost focus, active means you bring back to front
        if(state == Qt::ApplicationState::ApplicationInactive){
            qDebug() << "now typing password!"<<endl;
            on_typeButton_clicked(); //only if app inactive!
        }
        bPendingPasswordType = false; //type once only
    }

    //QObject::connect(qApp, &QGuiApplication::applicationStateChanged, this, [=](Qt::ApplicationState state){
    //    qDebug() << "Application state is now = "<< state;
    //});
}

// menu items on tray icon!
void MainWindow::createActions()
{
	
    restoreAction = new QAction(tr("&Open AnyKey Configurator"), this);
#ifdef _WIN32
	restoreAction->setShortcut(Qt::CTRL + Qt::Key_O); //shows but is not triggered with shortcut
#else
    restoreAction->setShortcut(Qt::CTRL + Qt::Key_Comma); //shows but is not triggered with shortcut
#endif
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showConfigurator()));

    minimizeAction = new QAction(tr("&Minimize"), this);
    minimizeAction->setShortcut(Qt::CTRL + Qt::Key_M); //this is mostly for ref
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    //maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

    //for shortcuts to work we also add these to regular menu
    //that way if window is hidden
    //When typeaction is used, start typing pass after windw
    //loses focus
    bPendingPasswordType = false;

#ifdef _WIN32
    typeAction = new QAction(tr("Type &Password"), this);
    typeAction->setShortcut(Qt::CTRL + Qt::Key_P);
#else
    typeAction = new QAction(tr("&Type Password"), this);
    typeAction->setShortcut(Qt::CTRL + Qt::Key_T);
#endif
	connect(typeAction, SIGNAL(triggered()),this,SLOT( typePasswordAgain() ));
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState )), this, SLOT(appFocusChanged(Qt::ApplicationState )));

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}
void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(typeAction);

    //first one doesn't work, second one only if dropdown is visible, shortcuts do however work in edit menu
    //trayIconMenu->addAction(tr("&Type Password"),this,SLOT( typePasswordAgain()), QKeySequence(Qt::CTRL + Qt::Key_T ));

    //trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::on_actionType_password_triggered()
{
    typePasswordAgain();
    ui->passwordEdit->setFocus();
}

void MainWindow::on_actionOpen_triggered()
{
   showConfigurator();
}


void MainWindow::on_actionHide_triggered()
{
    hide();
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (trayIcon->isVisible()) {
        /* QMessageBox::information(this, tr("Systray"),
                                 tr("The program will keep running in the "
                                    "system tray. To terminate the program, "
                                    "choose <b>Quit</b> in the context menu "
                                    "of the system tray entry."));*/
        hide();
        ui->passwordEdit->setText(""); //clear any stored pw strings now!
        event->ignore();
    }
    else{
        qDebug() << "Exiting because tray icon is invisible..." << endl;
    }
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            //this->restoreAction->toggle();
            break;
        case QSystemTrayIcon::DoubleClick:
            //hide();
            //iconComboBox->setCurrentIndex((iconComboBox->currentIndex() + 1)
            //                              % iconComboBox->count());
            break;
        case QSystemTrayIcon::MiddleClick:

            break;
        default:
            ;
        }
}

void MainWindow::showMessage(const QString& title, const QString& msg, int seconds)
{
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(1);
    trayIcon->showMessage(title, msg, icon, seconds * 1000);
}

void MainWindow::setIcon(int)
{
    //QIcon icon = iconComboBox->itemIcon(index);
    //trayIcon->setToolTip(iconComboBox->itemText(index));
    //QIcon icon(":/images/heart.svg");

    QIcon icon( ":anykey_app_logo_256.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);

    trayIcon->setToolTip("AnyKey Configurator v2.0");
}


MainWindow::~MainWindow()
{
    stopAnykeyCRD();
    delete anykeycrd;
    delete ui;
}

bool MainWindow::registerLicense( QString activation_code ){

        // create custom temporary event loop on stack
        QEventLoop eventLoop;

        //QString activation_url = QString("http://anykey.shop/activate/"+activation_code+".json");
        // "quit()" the event-loop, when the network request "finished()"
        QNetworkAccessManager mgr;
        QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

        setStatus("Contacting anykey.shop...");

        // the HTTP request
        QNetworkRequest req( QUrl( QString("http://anykey.shop/activate/"+activation_code+".json") ) );
        //QNetworkRequest req( QUrl( QString("http://localhost:3000/activate/"+activation_code+".json") ) );

        QNetworkReply *reply = mgr.get(req);
        eventLoop.exec(); // blocks stack until "finished()" has been called

        if (reply->error() == QNetworkReply::NoError) {

            setStatus("Request OK, saving license...");

            //success: write reply license.json
            QString license_content = reply->readAll();

            QString home_path = QCoreApplication::applicationDirPath();
            QString fileName(home_path+"/license.json");
            //setStatus("RESP: "+license_content+" PATH: "+fileName); //debug line


            QFile license_file(fileName);
            if( !license_file.open(QIODevice::WriteOnly) )
             {
               QString msg = QString( tr("Unable to save license to file: %1") ).arg(fileName);
               QMessageBox::critical(this, tr("AnyKey could not save license"), msg, QMessageBox::Ok );
               return false;
             }

            //write license_content into license_file
            QTextStream outputStream(&license_file);
            outputStream << license_content;

            license_file.close();

            delete reply;

            //all a-ok return true!!!
            return true;
        }
        else {
            //failure
            // FOR DEBUG, LATER REMOVE SETTEXT HERE, JUST RETURN FALSE!
            //setStatus("ERROR: "+ reply->errorString());
            setStatus("REQUEST FAILED!");
            delete reply;
            return false;
        }
}


QString MainWindow::anykeySave( const QStringList& arguments )
{
    QString home_path = QCoreApplication::applicationDirPath();
    QProcess anykey_save;
    anykey_save.setProcessChannelMode(QProcess::MergedChannels);
    anykey_save.setWorkingDirectory(home_path);
    anykey_save.start(home_path+"/"+ANYKEY_SAVE, arguments );

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    if (!anykey_save.waitForStarted()){
        QMessageBox::critical(this, tr("AnyKey"),
                                                tr("Installation error\n"
                                                "The anykey_save utility is not found in it's location:")+
                                                home_path+
                                                tr("\nPlease try re-installing the AnyKey software."),
                                                QMessageBox::Ok );
        setStatus("Installation error!");
        return "";
    }

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    // show error if saving to anykey failed somehow
    if(!anykey_save.waitForFinished()){
        setStatus("Write error:"+anykey_save.errorString());
        return "";
    }

    QByteArray save_output = anykey_save.readAll();
    QString output_str = QString(save_output);

    return output_str;
}



QString MainWindow::anykeySavePassword(const QString& password)
{
    setStatus("Saving password...");
    QString result="password_saved"; //TODO better errorhandling here!

    if(ui->addReturn->isChecked()){
        if(ui->copyProtectToggle->isChecked()){
            //result = anykeySave( QStringList() << "-cps" << password );
            runCommand("write_password_cps:"+password+"\n");
        }
        else{
            //result = anykeySave( QStringList() << "-s" << password );
            runCommand("write_password_s:"+password+"\n");
        }
    }
    else{ //skip return
        if(ui->copyProtectToggle->isChecked()){
            //result = anykeySave( QStringList() << "-cpn" << password );
            runCommand("write_password_cpn:"+password+"\n");
        }
        else{
            //result = anykeySave( QStringList() << "-n" << password );
            runCommand("write_password_n:"+password+"\n");
        }
    }

    return result;
}

void MainWindow::anykeyFactoryReset(){
    runCommand("format_eeprom\n");
}

void MainWindow::runCommand( const QString& command ){
    //if(commands_queue.length()>0){
    //check if command is not already on queue
    for(int i=0;i<commands_queue.length();i++){
            if( commands_queue.at(i) == command) return;
    }

    //add it to queue so that we execute it once anykey is connected
    commands_queue << command;
}


void MainWindow::anykeyParseSettings(const QString& response ){
    QString result = response;
    if(result.contains("ERROR: ")){
        setStatus(result);
        ui->advancedSettingsToggle->setChecked(false);

        if( result.contains("ERROR: could not find a connected") ){
            result += "\nPlease re-insert your AnyKey and try again.";
            QMessageBox::warning(this, tr("AnyKey"),
                                       result,
                                       QMessageBox::Ok );

            setStatus("ERROR: Could not read settings");
            ui->copyProtectToggle->setChecked(false);
            ui->copyProtectToggle->setEnabled(false);
        }
        return;
    }

    result = result.split("settings=").at(1);

    QString firmware_version = "";
    int kbd_delay_start = 1;
    int kbd_delay_key   = 1;
    int keyboard_layout = 0;
    int kbd_config = 0;
    int bootlock = 255;
    int deviceInMapping = 1;
    QString deviceId="";

    //qDebug() << result << endl;
    //"version=2.5; kbd_conf=222; kbd_delay_start=255; kbd_delay_key=255; kbd_layout=255; bootlock=255; id=ffffff;\n"

    QStringList settings = result.split(";");
    for(int i=0;i<settings.size();i++){
        QString kv = settings.at(i).toLocal8Bit();
        QStringList keyvalue = kv.split("=");
        if( keyvalue.at(0).contains("version")         ) firmware_version = keyvalue.at(1);
        if( keyvalue.at(0).contains("kbd_conf")        ) kbd_config = keyvalue.at(1).toInt();
        if( keyvalue.at(0).contains("kbd_delay_start") ) kbd_delay_start = keyvalue.at(1).toInt();
        if( keyvalue.at(0).contains("kbd_delay_key")   ) kbd_delay_key = keyvalue.at(1).toInt();
        if( keyvalue.at(0).contains("kbd_layout") )      keyboard_layout = keyvalue.at(1).toInt();
        if( keyvalue.at(0).contains("bootlock")        ) bootlock = keyvalue.at(1).toInt();
        if( keyvalue.at(0).contains("id")              ) deviceId = keyvalue.at(1);
        if( keyvalue.at(0).contains("device_mapped")   ) deviceInMapping = keyvalue.at(1).toInt();
    }

    // 0 -> belgian french azerty
    // 5 -> qwerty
    if( keyboard_layout == 255) keyboard_layout=5; //default value 5 when factory reset;
    if( kbd_delay_key == 255 ) kbd_delay_key = 0;
    if( kbd_delay_start == 255 ) kbd_delay_start = 0;

    ui->keyboardLayoutBox->setCurrentIndex(keyboard_layout);
    ui->keypressDelaySlider->setValue(kbd_delay_key);
    ui->startupDelaySlider->setValue(kbd_delay_start);

    if(bootlock==187){
        ui->bootloaderStatus->setText("locked");
    }
    if((bootlock==255)||(bootlock==186)||(bootlock==185)){
        ui->bootloaderStatus->setText("unlocked");
    }

    if(kbd_config == 222){
        ui->copyProtectToggle->setChecked(false);
    }
    if(kbd_config == 220){
        ui->copyProtectToggle->setChecked(true);
    }

    ui->deviceId->setText(deviceId);
    if( deviceId == "ffffff"){
       ui->deviceId->setText("Blank");
       ui->copyProtectToggle->setEnabled(false);
       ui->copyProtectToggle->setChecked(false); //make it type password on next save without cr!
       ui->updateSaltButton->setEnabled(true);
       ui->updateSaltButton->setText("Generate Salt");
       ui->saltStatus->setText("Unprotected");
       ui->daemonAutoType->setChecked(false);
       ui->typeButton->setEnabled(false); //disable the CR typing until salt is set
    }
    else{
       ui->deviceId->setText(deviceId);
       if(deviceInMapping == 1){
           ui->typeButton->setEnabled(true);
           ui->copyProtectToggle->setEnabled(true);
           ui->saltStatus->setText("Secured");
           ui->updateSaltButton->setEnabled(true);
           ui->updateSaltButton->setText("Update Salt");
       }
       else{
           ui->typeButton->setEnabled(false);
           ui->copyProtectToggle->setEnabled(false);
           ui->saltStatus->setText("Unknown");
           ui->updateSaltButton->setEnabled(true);
           ui->updateSaltButton->setText("Read Salt");
       }
    }

    ui->firmwareVersion->setText(firmware_version);
    setStatus("AnyKey ready");
}

void MainWindow::anykeySaveSettings()
{
    int activateCopyProtect=0;
    int activateReturnPress=0;
    if( ui->copyProtectToggle->isEnabled() ){
        if(ui->copyProtectToggle->isChecked()){
            activateCopyProtect=1;
        }
    }
    if(ui->addReturn->isChecked()){
        activateReturnPress=1;
    }

    QString write_flags = "write_flags:";
    write_flags+= QString::number(activateReturnPress)+" ";
    write_flags+= QString::number(activateCopyProtect)+" ";
    write_flags += "\n";

    qDebug()<<"runCommand="<<write_flags<<endl;
    runCommand( write_flags );
}

void MainWindow::anykeySaveAdvancedSettings()
{
    setStatus("Saving configuration...");

    if(ui->daemonAutoLock->isChecked()){
        runCommand( "autolock_on\n");
    }
    else{
        runCommand( "autolock_off\n");
    }

    int kbd_conf = 222;
    if(ui->copyProtectToggle->isChecked()){
        kbd_conf = 220;
    }
    int kbd_delay_start = ui->startupDelaySlider->value();
    int kbd_delay_key =   ui->keypressDelaySlider->value();
    int kbd_layout = ui->keyboardLayoutBox->currentIndex();

    /*
    QStringList arguments;
    arguments << "-saveadvancedsettings";
    arguments << QString::number(kbd_conf);
    arguments << QString::number(kbd_delay_start);
    arguments << QString::number(kbd_delay_key);
    arguments << QString::number(kbd_layout);
    */

    QString cmd = "write_settings:";
    cmd+= QString::number(kbd_conf)+" ";
    cmd+= QString::number(kbd_delay_start)+" ";
    cmd+= QString::number(kbd_delay_key)+" ";
    cmd+= QString::number(kbd_layout);

    runCommand( cmd + "\n");

    // stopAnykeyCRD();
    //qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}


// this is later returned. and we'll auto add it to our devices.map upon success
// in anykey_crd
void MainWindow::anykeyReadSalt(const QString& password )
{
    //QString result = anykeySave( QStringList() << "-readsalt" << password);
    //return result;
    runCommand("read_salt:"+password+"\n");
}

void MainWindow::parseWriteSaltResponse(const QString& result){
    //code is 3 bytes device_id+ 32 bytes as salt
    //QString result = anykeySave( QStringList() << "-salt" << code );
    //qDebug() << "result="<<result<<endl;
    if( result.contains("ERROR:") ){
        QMessageBox::warning(this, tr("AnyKey"),
                                                result,
                                                QMessageBox::Ok );

        setStatus("Saving salt failed.");
        return;
    }
    else{
        if( result.contains("salt saved to device")){
            QString devId = result.right(4).left(3);
            setStatus("Salt saved your new device id="+devId);
            ui->copyProtectToggle->setEnabled(true);
            ui->deviceId->setText(devId);
            ui->saltStatus->setText("secured");
            setStatus("New salt saved");
        }

    }
}

void MainWindow::anykeyUpdateSalt(const QString& code )
{
    //todo: double check if crd correctly re-reads devices.map here!
    setStatus("Saving salt and device id...");
    runCommand("write_salt:"+code+"\n");
}

void MainWindow::on_updateSaltButton_clicked()
{    
    if( ui->updateSaltButton->text().contains("Read Salt")){
        setStatus("Reading device salt using password...");
        anykeyReadSalt(ui->passwordEdit->text());
    }
    else{ // update or generate salt
        //TODO: future work we keep same device id
        // and only change salt. for now we re-generate everything
        QString code = randomString(35); //3 for dev id, 32 for salt
        anykeyUpdateSalt( code );
    }
}



void MainWindow::on_saveButton_clicked()
{
    if( !checkLicense() ){
        setStatus("Registering activation code : "+ui->passwordEdit->text());
        //QTimer::singleShot(200, this, SLOT(updateCaption())); -> todo...

        bool licenseOk = MainWindow::registerLicense(ui->passwordEdit->text());

        if (!licenseOk){
            setStatus("Invalid activation code");
            return;
        }
        else{ //activation ok!
            if( checkLicense() ){
                showRegisteredControls();
                ui->titleLabel->setText("Insert your AnyKey then type a password and click on save:");
                ui->passwordEdit->setEchoMode(QLineEdit::Password);
                ui->saveButton->setText("Save");
                setStatus("Registration successfull. AnyKeyConfigurator activated!");
                ui->passwordEdit->setText("");
                ui->copyProtectToggle->show();
                ui->copyProtectToggle->setEnabled(false);
                ui->copyProtectToggle->setChecked(false);
                return;
            }
            else{
                setStatus("Invalid license");
                return;
            }
        }
    }

    QString password = ui->passwordEdit->text();

    if( password.length() > 0 ){
        setStatus("Saving password...");

        QString output_str = anykeySavePassword( password );
        if( output_str.contains("License check failed!") ){
            /*QMessageBox::critical(this, tr("AnyKey"),
                                                    tr("Invalid License!\n"
                                                    "You need to activate this app using the activation code given."),
                                                    QMessageBox::Ok );*/

            setStatus("Invalid license or activation code!");
        }
        else if( output_str.contains("ERROR: could not find a connected") ){
            QMessageBox::warning(this, tr("AnyKey"),
                                                    tr("Could not find a connected AnyKey in any USB ports\n"
                                                    "Please insert your AnyKey and try again."),
                                                    QMessageBox::Ok );

            setStatus("Save failed, AnyKey not found");
            return;
        }
        else if( output_str.contains("ERROR:")){
            setStatus(output_str + "try re-inserting and/or saving again");
        }
        else{
            // if( output_str.contains("password saved")){
            // patch, sometimes it's empty then most likely its also saved correctly
            //setStatus("unknown error, try re-inserting and saving again");
        }
        ui->passwordEdit->setFocus();
    }
    else{
        setStatus("Skipping empty password, writing addReturn and copyProtect flags...");
        anykeySaveSettings();
        runCommand("read_settings\n");
        setStatus("Saved flags and read settings. (Skipped empty pass)");
    }
}

void MainWindow::on_actionClose_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered(){

    // this works but with the main window minimized this now closes entire app
    QMessageBox mbox;
    mbox.setText( tr("AnyKey Configurator\n"
                     "AnyKey is a product of AnyKey bvba.\n"
                     "Patented in Belgium BE1019719 (A3).\n\n"
                     "Version : 2.2 \n"
                     "Author  : Walter Schreppers"));
    mbox.exec();

    //TODO fix this by setting qapp closeevents to something else...
}

void MainWindow::on_showPassword_clicked(bool checked)
{
    if( checked ){
        //qDebug() <<"Show password";
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
    }
    else{
        //QLineEdit::Password is also possible
        ui->passwordEdit->setEchoMode(QLineEdit::Password);
    }
}

QString MainWindow::randomString(int password_length){
    QString randomPass="";
    while(randomPass.length() < password_length){
        randomPass+= QUuid::createUuid().toString();
    }
    randomPass.replace("{","");
    randomPass.replace("-","");
    randomPass.replace("}","");
    randomPass = randomPass.left(password_length);

    QString genPass = "";
    int pseudoRand=1;
    for(int i=0;i<randomPass.length(); i++){
      pseudoRand=(pseudoRand+3)%13;
      if(pseudoRand>5)
        genPass += randomPass.at(i).toUpper();
      else
        genPass += randomPass.at(i).toLower();
    }
    return genPass;
}

void MainWindow::on_generateButton_clicked()
{
    ui->passwordEdit->setText( randomString(ui->generateLength->value()) );
    ui->passwordEdit->setFocus();
}

void MainWindow::on_passwordEdit_textChanged(const QString&)
{
    //setStatus("");
}

void MainWindow::on_keyboardLayoutBox_currentIndexChanged(int )
{
  //qDebug() << "keyboardLayoutBox changed! index="<< index <<endl;
}

void MainWindow::hideAdvancedItems(){
  ui->keyboardLayoutBox         -> hide();
  ui->startupDelaySlider        -> hide();
  ui->keypressDelaySlider       -> hide();
  ui->restoreDefaultsBtn        -> hide();
  ui->applyAdvancedSettingsBtn  -> hide();
  ui->labelLayout         ->hide();
  ui->labelStartDelay     ->hide();
  ui->labelKeypressDelay  ->hide();
  ui->startupDelaySpin    ->hide();
  ui->keypressDelaySpin   ->hide();

  ui->firmwareVersion->hide();
  ui->firmwareVersionLabel->hide();
  ui->bootloaderLabel->hide();
  ui->bootloaderStatus->hide();
  ui->upgradeFirmwareButton->hide();
  ui->formatEepromButton->hide();

  ui->deviceId->hide();
  ui->deviceIdLabel->hide();
  ui->saltStatus->hide();
  ui->updateSaltButton->hide();

  ui->daemonLabel->hide();
  ui->daemonStatusLabel->hide();
  //ui->daemonAutostartCheck->hide();
  ui->daemonAutoType->hide();
  ui->daemonAutoLock->hide();
  ui->daemonRestartButton->hide();
  ui->typeButton->hide();

#ifdef __APPLE__
  this->setMinimumWidth(540);
  this->setMinimumHeight(178);
  this->setMaximumHeight(178);
#else //windows, linux
  /*this->setMinimumWidth(800); //520
  this->setMinimumHeight(237); //187
  this->setMaximumHeight(237);*/

  this->setMinimumWidth(540); 
  this->setMinimumHeight(178); 
  this->setMaximumHeight(178);

#endif
}

void MainWindow::on_advancedSettingsToggle_clicked(bool checked)
{
  if( checked ){
    runCommand("read_settings\n");
  }
  if(ui->advancedSettingsToggle->isChecked()){
    ui->keyboardLayoutBox         -> show();
    ui->startupDelaySlider        -> show();
    ui->keypressDelaySlider       -> show();
    ui->restoreDefaultsBtn        -> show();
    ui->applyAdvancedSettingsBtn  -> show();
    ui->labelLayout         ->show();
    ui->labelStartDelay     ->show();
    ui->labelKeypressDelay  ->show();
    ui->startupDelaySpin    ->show();
    ui->keypressDelaySpin   ->show();

    ui->firmwareVersion->show();
    ui->firmwareVersionLabel->show();
    ui->bootloaderLabel->show();
    ui->bootloaderStatus->show();
    ui->upgradeFirmwareButton->show();
    ui->upgradeFirmwareButton->setEnabled(false);
    ui->formatEepromButton->show();

    ui->daemonLabel->show();
    ui->daemonStatusLabel->show();
    //ui->daemonAutostartCheck->show();
    ui->daemonAutoType->show();
    ui->daemonAutoType->setEnabled(true);
    ui->daemonAutoLock->show();
    ui->daemonAutoLock->setEnabled(true);
    ui->daemonRestartButton->show();
    ui->typeButton->show();

    ui->deviceId->show();
    ui->deviceIdLabel->show();
    ui->saltStatus->show();
    ui->updateSaltButton->show();
#ifdef __APPLE__
    this->setMinimumWidth(540);
    this->setMinimumHeight(420);
    this->setMaximumHeight(420);
#else //windows and linux
	/*this->setMinimumWidth(800); //520 looks good on 200% and 150% scale
    this->setMinimumHeight(560); //380 but width 640 and height 420 is minimum for 100% scale on retina...
    this->setMaximumHeight(560);*/
	this->setMinimumWidth(540); 
    this->setMinimumHeight(380); 
    this->setMaximumHeight(380);
#endif
  }
  else{//hide items
    hideAdvancedItems();
  }

  ui->passwordEdit->setFocus();
}

void MainWindow::on_restoreDefaultsBtn_clicked()
{
    ui->keyboardLayoutBox->setCurrentIndex(4);
    ui->startupDelaySlider->setValue(1);
    ui->keypressDelaySlider->setValue(1);
}

void MainWindow::on_startupDelaySlider_valueChanged(int value)
{
    ui->startupDelaySpin->setValue(value);
}

void MainWindow::on_keypressDelaySlider_valueChanged(int value)
{
    ui->keypressDelaySpin->setValue(value);
}


void MainWindow::on_startupDelaySpin_valueChanged(int arg1)
{
    ui->startupDelaySlider->setValue(arg1);
}

void MainWindow::on_keypressDelaySpin_valueChanged(int arg1)
{
    ui->keypressDelaySlider->setValue(arg1);
}

void MainWindow::on_applyAdvancedSettingsBtn_clicked()
{
    anykeySaveAdvancedSettings();
}

// This is now called Factory Reset (label)
void MainWindow::on_formatEepromButton_clicked()
{
    QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "AnyKey Factory Reset", "This reset's your device to factory settings. \nWarning: It removes all passwords and salts saved on your AnyKey!\n Are you sure you want to do this?",
                                    QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "Ok was clicked";
    } else {
        qDebug() << "cancel clicked";
        return;
    }

    setStatus("Formatting anykey eeprom...");
    anykeyFactoryReset();

    qDebug()<<"TODO: Instead of closing, just set the settings as if it was default here without rereading!"<<endl;

    //close advanced settings
    ui->advancedSettingsToggle->setChecked(false);
    on_advancedSettingsToggle_clicked(false);

    //setStatus("done. Now take out re-insert your anykey!");
    // runCommand("read_settings\n"); //avoid firmware crash after reset we need to eject first
}

void MainWindow::on_upgradeFirmwareButton_clicked()
{
    qDebug() << "TODO: anykey_save -upgrade ..." << endl;
}


void MainWindow::on_daemonRestartButton_clicked()
{
    if( ui->daemonRestartButton->text() == "Stop"){
        setStatus("Stopping CR daemon...");
        stopAnykeyCRD();
        ui->daemonRestartButton->setText("Start");
        setStatus("");
    }
    else{
        setStatus("Starting CR daemon...");
        startAnykeyCRD();
        ui->daemonRestartButton->setText("Stop");
        setStatus("");
    }
}

void MainWindow::on_typeButton_clicked()
{
    runCommand("type\n");
    setStatus("Sending challenge response to type password...");
    ui->passwordEdit->selectAll();
    ui->passwordEdit->setFocus();
}



