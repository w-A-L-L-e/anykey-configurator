/*=============================================================================
author        : Walter Schreppers
filename      : mainwindow.cpp
created       : 3/6/2017
modified      :
version       :
copyright     : Walter Schreppers
bugreport(log):
=============================================================================*/

#include "mainwindow.h"
#include "ui_anykey.h"
#include <iostream>
#include <qbytearray.h>
#include <qdebug.h>
#include <qdir.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qprocess.h>
#include <quuid.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

// used with checkShowWindow but
#include <QSharedMemory>
#include <QSystemSemaphore>

#ifdef __APPLE__
#include "launchagent.h"
#endif

using namespace std;

// exe name of anykey_save
#define ANYKEY_SAVE "anykey_save"
#define ANYKEY_TYPE_DAEMON "anykey_crd"
#define CONFIGURATOR_VERSION "2.3.2"

// constructor checks license and shows
// alternate gui if license check fails to register.
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    licenseRegistered = false;
    trayIconActivated = false;
    anykeycrd = new QProcess();

    this->setMinimumWidth(425);
    ui->setupUi(this);

    // Set status and progress bar
    // create objects for the label and progress bar
    statusLabel = new QLabel(this);

    // we might re-introduce this for upgrading right now
    // all is fast enough to not need one
    // statusProgressBar = new QProgressBar(this);
    // statusProgressBar->setTextVisible(false);

    // add the two controls to the status bar
    ui->statusbar->addPermanentWidget(statusLabel, 0);
    // ui->statusbar->addPermanentWidget(statusProgressBar,1);
    ui->mainProgressBar->hide(); // hide progressbar until something is running...

    ui->titleLabel->setText("");
    setStatus("Checking license...");

    // read license file here!!!
    if (!checkLicense()) {
        ui->titleLabel->setText("Enter activation code");
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        ui->showPassword->hide();
        ui->generateButton->hide();
        ui->advancedSettingsToggle->hide();
        ui->copyProtectToggle->hide();
        ui->generateLength->hide();
        ui->addReturn->hide();
        ui->daemonAutoType->hide();
        ui->saveButton->setText("Register");
        setStatus("");
        hideAdvancedItems();
        ui->passwordEdit->setFocus();
        ui->menubar->hide();
    }
    else {
        showRegisteredControls();
        // brings window to front if a start of app is required.
        // it does this by periodically checking ipc/sharedMemory
        windowTimer = new QTimer(this);
        connect(windowTimer, SIGNAL(timeout()), this, SLOT(checkShowWindow()));
        windowTimer->start(800);
    }
}

// destructor stops the anykey_crd daemon also now.
MainWindow::~MainWindow()
{
    stopAnykeyCRD();
    // if (trayIconActivated) {
    if (windowTimer) delete windowTimer;
    if (trayIcon) delete trayIcon;
    if (trayIconMenu) delete trayIconMenu;
    if (minimizeAction) delete minimizeAction;
    if (restoreAction) delete restoreAction;
    if (quitAction) delete quitAction;
        // delete maximizeAction; //deprecated
#ifdef __APPLE__
    if (typeAction) delete typeAction;
    if (autostartAction) delete autostartAction;
#endif
    if (statusLabel) delete statusLabel;
    // delete statusProgressBar; //deprecated
    // }
    if (anykeycrd) delete anykeycrd;
    delete ui;
}

// keep users settings in config file on disk so they persist after reboot.
void MainWindow::writeGuiControls()
{
    QString home_path = QCoreApplication::applicationDirPath();
    QString fileName(home_path + "/anykey.cfg");
    QFile config_file(fileName);
    if (!config_file.open(QIODevice::WriteOnly)) {
        QString msg = QString(tr("Unable to save configuration to file: %1")).arg(fileName);
        QMessageBox::critical(this, tr("AnyKey could not save configuration"), msg, QMessageBox::Ok);
        return;
    }

    QString controls = "";
    controls += "add_return=" + QString::number(ui->addReturn->isChecked()) + ";";
    controls += "copy_protect=" + QString::number(ui->copyProtectToggle->isChecked()) + ";";
    controls += "auto_type=" + QString::number(ui->daemonAutoType->isChecked()) + ";";
    controls += "advanced_settings=" + QString::number(ui->advancedSettingsToggle->isChecked()) + ";";

    QTextStream outputStream(&config_file);
    outputStream << controls << endl;

    config_file.close();

    qDebug() << "Write anykey.cfg success" << endl;
}

void MainWindow::readGuiControls()
{
    QString home_path = QCoreApplication::applicationDirPath();
    QString fileName(home_path + "/anykey.cfg");
    QFile config_file(fileName);
    if (config_file.open(QIODevice::ReadOnly)) {

        QString result = config_file.readAll();
        QStringList settings = result.split(";");
        int add_return = 0, copy_protect = 0, auto_type = 0, advanced_settings = 0;

        for (int i = 0; i < settings.size(); i++) {
            QString kv = settings.at(i).toLocal8Bit();
            QStringList keyvalue = kv.split("=");
            if (keyvalue.at(0).contains("add_return")) add_return = keyvalue.at(1).toInt();
            if (keyvalue.at(0).contains("copy_protect")) copy_protect = keyvalue.at(1).toInt();
            if (keyvalue.at(0).contains("auto_type")) auto_type = keyvalue.at(1).toInt();

            // for now always retusrn advanced settings as closed.
            // this because it triggers a read settings when inserted an anykey without copyprotect
            // it sometimes fails (reading setting during typing...)
            // if( keyvalue.at(0).contains("advanced_settings")) advanced_settings = keyvalue.at(1).toInt();
        }

        config_file.close();

        qDebug() << "Read anykey.cfg success"
                 << " add_return=" << add_return << " copy_protect=" << copy_protect << " auto_type=" << auto_type << endl;

        if (add_return > 0)
            ui->addReturn->setChecked(true);
        else
            ui->addReturn->setChecked(false);

        if (copy_protect > 0)
            ui->copyProtectToggle->setChecked(true);
        else
            ui->copyProtectToggle->setChecked(false);

        if (auto_type > 0)
            ui->daemonAutoType->setChecked(true);
        else
            ui->daemonAutoType->setChecked(false);

        if (advanced_settings > 0)
            ui->advancedSettingsToggle->setChecked(true);
        else
            ui->advancedSettingsToggle->setChecked(false);
    }
    else {
        qDebug() << "config file not found" << endl;
    }
}

// This on_save clicked has double functionality before registration
// the save button is the 'register' button.
// afterwards if licens check ok then we use it to save the
// new passwords. Again we might refactor in future. for now it just works
void MainWindow::on_saveButton_clicked()
{
    if (!checkLicense()) {
        setStatus("Registering activation code : " + ui->passwordEdit->text());
        bool licenseOk = MainWindow::registerLicense(ui->passwordEdit->text());

        if (!licenseOk) {
            setStatus("Invalid activation code");
            return;
        }
        else {                    // activation ok!
            if (checkLicense()) { // double check if license is now valid
                showRegisteredControls();
                ui->titleLabel->setText("Insert your AnyKey then type a password and click on save:");
                ui->passwordEdit->setEchoMode(QLineEdit::Password);
                ui->saveButton->setText("Save");
                setStatus("Registration successfull. AnyKeyConfigurator activated!");
                ui->passwordEdit->setText("");

                // upon first registration disable c.p until salt secret is linked
                ui->copyProtectToggle->show();
                ui->copyProtectToggle->setEnabled(false);
                ui->copyProtectToggle->setChecked(false);
                ui->daemonAutoType->show();
                ui->daemonAutoType->setChecked(false);

#ifdef __APPLE__
                // make configurator auto start again upon reboot
                LaunchAgent anykeyAutostarter;
                anykeyAutostarter.install();
                if (anykeyAutostarter.is_installed()) {
                    autostartAction->setText(tr("Disable Autostart"));
                }
#endif
                return;
            }
            else {
                setStatus("Invalid license");
                return;
            }
        }
    }

    QString password = ui->passwordEdit->text();

    if (password.length() > 0) {
        setStatus("Saving password...");
        QString output_str = anykeySavePassword(password);
        // error handling is now in readCRD as this is refactored to use anykey_crd
        ui->passwordEdit->setFocus();
    }
    else {
        setStatus("Skipping empty password, writing addReturn and copyProtect flags...");
        anykeySaveSettings();
        runCommand("read_settings\n");
        // setStatus("Saved flags and read settings. (Skipped empty pass)");
    }

    // possibly later version do this after each confirm...
    writeGuiControls();
}

void MainWindow::on_actionClose_triggered()
{
    QApplication::quit();
    // This now calls the destructor which will stop anykey_crd etc.
}

// you need a valid license code and this downloads a valid license file once.
// after this is installed no more online requests are needed and this license.json
// is checked upon each startup of the app instead
bool MainWindow::checkLicense()
{
    if (licenseRegistered) {
        return true;
    }
    qDebug() << "checking license..." << endl;

    QString home_path = QCoreApplication::applicationDirPath();

    // run  home_path/anykey_save -license
    // returns 'ok' or 'invalid license'
    QProcess anykey_save;
    anykey_save.setProcessChannelMode(QProcess::MergedChannels);
    anykey_save.setWorkingDirectory(home_path);
    anykey_save.start(home_path + "/" + ANYKEY_SAVE, QStringList() << "-license");
    // above works in windows, here on prev mac version we used:
    // anykey_save.start(home_path+"/../Helpers/"+ANYKEY_SAVE, QStringList() << "-license" );

    if (!anykey_save.waitForStarted()) {
        QMessageBox::critical(this, tr("AnyKey"),
                              tr("Installation error\n"
                                 "The anykey_save utility is not found in it's location:") +
                                  home_path + tr("\nPlease try re-installing the AnyKey software."),
                              QMessageBox::Ok);
        setStatus("Installation error!");
        return false;
    }

    if (!anykey_save.waitForFinished()) {
        setStatus("Error while calling anykey savetool error 9000!");
        return false;
    }

    QByteArray save_output = anykey_save.readAll();
    QString output_str = QString(save_output);

    if (output_str.contains("invalid")) {
        return false;
    }

    if (output_str.contains("ok")) {
        licenseRegistered = true;
        return true;
    }

    return false;
}

// show message in status bar. sprinkled around a lot to
// give user nice feedback of what is happening
void MainWindow::setStatus(const QString &msg)
{
    qDebug() << "status: " << msg << endl;
    statusBar()->showMessage(msg, 5000);
}

// if another app is started it detects this one is running and closes
// however we do want to bring this one to front (that way only one configurator stays running)
// and it shows it back to front when you execute another.
// this is done with a timer look in constructor where we call this every 800 msec
void MainWindow::checkShowWindow()
{
    bool showWindow = false;
    QSharedMemory sharedMemory("AnyKey Configurator Shared");
    if (!sharedMemory.attach()) {
        return;
    }

    sharedMemory.lock();
    const char *data = (const char *)sharedMemory.constData();
    if (sharedMemory.size() > 0) {
        // Finding 'S' this means a second anykey configurator started and was killed
        if (data[0] == 'S') {
            showWindow = true;
            // set it back to ' ' so that next reads don't start showing window again
            char *wdata = (char *)sharedMemory.data();
            wdata[0] = ' ';
        }
    }
    sharedMemory.unlock();

    if (showWindow) {
        qDebug() << "Bringing configurator to front, someone tried starting a new instance." << endl;
        showConfigurator();
    }
}

// Main communication with our device is done through the anykey_crd daemon
// this allows reading+writing messages back and forth to our usb device.
void MainWindow::readCRD()
{
    QString output = anykeycrd->readAllStandardOutput();
    QStringList lines = output.split("\n");

    for (int i = 0; i < lines.length(); i++) {
        QString line = lines.at(i);
        if (line.length() == 0) continue;
        if (line != "cmd: ") { // only print interesting lines on debug
            qDebug() << "READ: '" << line << "'";
        }

        // enable controls when anykey inserted
        if (line.contains("connected")) {
            if (ui->daemonAutoType->isChecked()) {
                showMessage("AnyKey detected", "Sending challenge response with PC secret...", 2);
            }
            else {
                showMessage("AnyKey detected", "AnyKey is allowed to type...", 2);
            }

            ui->saveButton->setEnabled(true);
            ui->applyAdvancedSettingsBtn->setEnabled(true);
            ui->typeButton->setEnabled(true);
            ui->formatEepromButton->setEnabled(true);
            ui->updateSaltButton->setEnabled(true);
        }

        // first connect -> do autotype if enabled
        if (line.contains("connected cmd:")) {
            if (ui->daemonAutoType->isChecked()) {
                anykeycrd->write("type\n");
                setStatus("AnyKey detected doing CR for typing password...");
            }
            else {
                setStatus("AnyKey detected");
                anykeycrd->write("none\n");
            }
            line = "";
            ui->passwordEdit->selectAll();
            ui->passwordEdit->setFocus();
        }
        else if (line.contains("cmd:")) {
            if (commands_queue.length() > 0) {
                if (commands_queue.at(0).contains("read_settings")) setStatus("Reading settings...");
                anykeycrd->write(commands_queue.at(0).toLocal8Bit());
                commands_queue.removeFirst();
            }
            else {
                anykeycrd->write("\n"); // none or empty string skips command
            }
            line = "";
        }

        if (line.contains("reset_response=Whiping eeprom (FF)eeprom cleared")) {
            setStatus("AnyKey reset to factory settings. Remove and re-insert");
            showMessage("Factory reset succeeded", "Please re-insert your AnyKey to reset and read new settings.");
            line = "";
        }

        if (line.contains("settings=")) {
            if (!line.contains("kbd_conf=")) {
                setStatus("ERROR: could not read settings.");
                showMessage("Read settings failed", "Try toggling advanced settings to re-read or re-insert your AnyKey and try again.");
            }
            else {
                anykeyParseSettings(line);
                line = ""; // clear line so next ifs don't catch false cmd
            }
        }

        if (line.startsWith("salt saved")) {
            setStatus("Salt saved");
            line = "";
        }
        if (line.startsWith("salt=")) {
            setStatus("device linked again");
            runCommand("read_settings\n");
            line = "";
        }

        if (line.startsWith("config saved")) {
            setStatus("Configuration saved");
            line = "";
        }
        if (line.startsWith("password saved")) {
            setStatus("Password saved");
            ui->passwordEdit->selectAll();
            ui->passwordEdit->setFocus();
            line = "";
        }
        if (line.startsWith("flags saved")) {
            setStatus("Flags saved");
            ui->passwordEdit->selectAll();
            ui->passwordEdit->setFocus();
            line = "";
        }

        // handle some errors that might be returned
        if (line.contains("ERROR: Invalid CR")) {
            showMessage("Invalid Challenge Response Secret",
                        "Please re-insert your AnyKey and try again. Or configure your Secret in advanced settings.");
            line = "";
        }
        if (line.contains("ERROR: password not saved")) {
            showMessage("Password Save Error!", "Please re-insert your AnyKey and try again.");
            setStatus("ERROR during save. Please try again!");
            line = "";
        }

        // salt == secret (used with Generate and Update Secret buttons)
        if (line.startsWith("salt saved to device")) {
            parseWriteSaltResponse(line);
            line = "";
        }

        // handle challenge response typing of password
        if (line.contains("id=")) { //"id=363739"
            setStatus("CP id received");
            ui->passwordEdit->setFocus();
            showMessage("AnyKey autotype", "AnyKey protected challenge response...");
        }
        if (line.contains("challenge =")) {
            setStatus("CP handling challenge response...");
            // ui->copyProtectToggle->setChecked(true);
        }
        if (line.contains("done")) {
            setStatus("AnyKey typed it's password after a CR");
            line = "";
        }

        // disable controls when anykey removed
        if (line.startsWith("disconnect")) {
            setStatus("Anykey disconnected");
            ui->saveButton->setEnabled(false);
            ui->applyAdvancedSettingsBtn->setEnabled(false);
            ui->typeButton->setEnabled(false);
            ui->formatEepromButton->setEnabled(false);
            ui->updateSaltButton->setEnabled(false);

            line = "";
        }

        if (line.startsWith("wrong password")) {
            setStatus("Read secret failed");
            showMessage("Wrong password given for Read Secret",
                        "You need to give the original saved password in the password field.\nThen click on Read Secret.");
            line = "";
        }

        if (line.startsWith("firmware_upgrade=")) {
            parseFirmwareUpgrade(line);
            line = "";
        }
    }
}

void MainWindow::parseFirmwareUpgrade(const QString &line)
{
    qDebug() << "parsing:" << line << endl;
}

void MainWindow::startAnykeyCRD()
{
    // we handle both stdout and stderr in same readCRD slot
    // in future version we might split this when the method
    // grows too much. For now it's maintainable with mergedchannels.
    QString home_path = QCoreApplication::applicationDirPath();
    anykeycrd->setProcessChannelMode(QProcess::MergedChannels);
    anykeycrd->setWorkingDirectory(home_path);
    anykeycrd->start(home_path + "/" + ANYKEY_TYPE_DAEMON, QStringList() << "-gui");

    connect(anykeycrd, SIGNAL(readyReadStandardOutput()), this, SLOT(readCRD()));
    anykeycrd->waitForStarted();
    ui->daemonStatusLabel->setText("running");
}

void MainWindow::stopAnykeyCRD()
{
    anykeycrd->kill();
    anykeycrd->waitForFinished();
    ui->daemonStatusLabel->setText("stopped");
}

void MainWindow::showRegisteredControls()
{
    ui->titleLabel->setText("Insert your AnyKey then type a password and click on save:");
    setStatus("");
    ui->passwordEdit->setEchoMode(QLineEdit::Password);
    // ui->passwordEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    ui->showPassword->show();
    ui->generateButton->show();
    ui->generateLength->show();
    ui->addReturn->show();
    ui->advancedSettingsToggle->show();
    ui->menubar->show();
    ui->copyProtectToggle->show();
    ui->copyProtectToggle->setEnabled(true); // on second start allow all controls regardless of c.p.
    ui->daemonAutoType->show();
    ui->daemonAutoType->setEnabled(true);

    readGuiControls();

    if (!ui->advancedSettingsToggle->isChecked()) {
        hideAdvancedItems();
    }
    else {
        this->on_advancedSettingsToggle_clicked(true);
    }

    ui->saveButton->setText("Save");
    ui->generateLength->setValue(20);

    createActions();
    createTrayIcon();
    setIcon(0);
    trayIcon->show();
    trayIconActivated = true;

    // challenge response and other requests to anykey
    startAnykeyCRD();
    qDebug() << "startAnyKeyCRD..." << endl;

    // ui->passwordEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    ui->passwordEdit->setFocus();

    QStringList args = QCoreApplication::arguments();
    if ((args.length() > 1) && args.at(1).contains("minimized")) {
        QTimer::singleShot(10, this, SLOT(hide()));
    }
}

void MainWindow::showConfigurator()
{
    this->showNormal();
    this->show();
    this->raise(); // bring on top for Mac OS
#ifdef _WIN32
    this->activateWindow(); // appWindow->requestActivate(); -> for Windows
#endif

    // whenever anykey is inserted,read its settings
    runCommand("read_settings\n");
}

void MainWindow::typePasswordAgain()
{
    bPendingPasswordType = true;
    showMessage("Type password", "Inserted AnyKey will do Challenge Response and type password after window loses focus", 8);
}

void MainWindow::appFocusChanged(Qt::ApplicationState state)
{
    if (bPendingPasswordType) {
        // Inactive means lost focus, active means you bring back to front
        if (state == Qt::ApplicationState::ApplicationInactive) {
            qDebug() << "now typing password!" << endl;
            on_typeButton_clicked(); // only if app inactive!
        }
        bPendingPasswordType = false; // type once only
    }
}

void MainWindow::toggleAutostart()
{
    qDebug() << "toggleAutostart..." << flush;
#ifdef __APPLE__
    LaunchAgent anykeyLA;

    if (anykeyLA.is_installed()) {
        qDebug() << "uninstalling..." << endl;
        anykeyLA.uninstall();
        autostartAction->setText(tr("Enable autostart"));
    }
    else {
        qDebug() << "installing..." << endl;
        anykeyLA.install();
        autostartAction->setText(tr("Disable autostart"));
    }
#endif
}

// menu items on tray icon!
void MainWindow::createActions()
{
    // for the shortcuts to work we also add these to regular menu
    // that way if window is hidden we can still use shortcuts (this works
    // verry well on mac os, not perfectly in windows. but with right click all is good there...).

    restoreAction = new QAction(tr("&Open AnyKey Configurator"), this);
#ifdef _WIN32
    restoreAction->setShortcut(Qt::CTRL + Qt::Key_O); // shows but is not triggered with shortcut
#else
    restoreAction->setShortcut(Qt::CTRL + Qt::Key_Comma); // shows but is not triggered with shortcut
#endif
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showConfigurator()));

    minimizeAction = new QAction(tr("&Minimize"), this);
    minimizeAction->setShortcut(Qt::CTRL + Qt::Key_M); // this is mostly for ref
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    // maximizeAction = new QAction(tr("Ma&ximize"), this);
    // connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

#ifdef _WIN32
    typeAction = new QAction(tr("&Type Password"), this);
    typeAction->setShortcut(Qt::CTRL + Qt::Key_T);
#else
    typeAction = new QAction(tr("&Type Password"), this);
    typeAction->setShortcut(Qt::CTRL + Qt::Key_T);
#endif

    // we use boolean to track a previous typePasswordAgain signal
    bPendingPasswordType = false;
    connect(typeAction, SIGNAL(triggered()), this, SLOT(typePasswordAgain()));

    // Detect window focus loss to only type after losing focus with this:
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(appFocusChanged(Qt::ApplicationState)));

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

#ifdef __APPLE__
    LaunchAgent anykeyLA;
    autostartAction = new QAction(tr("Disable autostart"), this);
    connect(autostartAction, SIGNAL(triggered()), this, SLOT(toggleAutostart()));
    if (!anykeyLA.is_installed()) autostartAction->setText(tr("Enable Autostart"));
#endif
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(minimizeAction);
#ifdef __APPLE__
    // in windows type action only works with the window opened and then alt-tab to new window
    // on apple/macos we can do it with a minimized window
    trayIconMenu->addAction(typeAction);
#endif
    // trayIconMenu->addAction(maximizeAction);

    trayIconMenu->addSeparator();
#ifdef __APPLE__
    trayIconMenu->addAction(autostartAction);
#endif
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    // minimize into tray. unless we're still in registration mode
    // then we truly exit here (trayIcon was not yet activated)
    if (trayIconActivated && trayIcon->isVisible()) {
        hide();
        ui->passwordEdit->setText(""); // clear any stored pw strings now!
        event->ignore();               // ignore close event and stay running in background
    }
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // qDebug()<<"Trayicon trigger"<<endl;
#ifdef _WIN32
        // on mac single click opens menu. windows: right click
        showConfigurator();
#endif
        break;
    case QSystemTrayIcon::DoubleClick:
        // for windows double clicker people and also mac people a nice shortcut ;)
        showConfigurator();
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

// shows notification style message (windows on right bottom, mac on top right)
void MainWindow::showMessage(const QString &title, const QString &msg, int seconds)
{
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(1);
    trayIcon->showMessage(title, msg, icon, seconds * 1000);
}

void MainWindow::setIcon(int)
{
    QIcon icon(":anykey_app_logo_256.png");
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("AnyKey Configurator v" + QString(CONFIGURATOR_VERSION));

    // also set the window icon to same
    setWindowIcon(icon);
}

bool MainWindow::registerLicense(QString activation_code)
{
    // create custom temporary event loop on stack
    QEventLoop eventLoop;
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    setStatus("Contacting anykey.shop...");

    // the HTTP request
    QNetworkRequest req(QUrl(QString("http://anykey.shop/activate/" + activation_code + ".json")));
    // QNetworkRequest req( QUrl( QString("http://localhost:3000/activate/"+activation_code+".json") ) );

    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        setStatus("Request OK, saving license...");

        // success: write reply license.json
        QString license_content = reply->readAll();

        QString home_path = QCoreApplication::applicationDirPath();
        QString fileName(home_path + "/license.json");

        QFile license_file(fileName);
        if (!license_file.open(QIODevice::WriteOnly)) {
            QString msg = QString(tr("Unable to save license to file: %1")).arg(fileName);
            QMessageBox::critical(this, tr("AnyKey could not save license"), msg, QMessageBox::Ok);
            return false;
        }

        // write license_content into license_file
        QTextStream outputStream(&license_file);
        outputStream << license_content;

        license_file.close();
        delete reply;

        // all a-ok return true!!!
        return true;
    }
    else {
        setStatus("REQUEST FAILED!");
        delete reply;
        return false;
    }
}

// anykey_save tool, might become deprecated some day if anykey_crd
// implements all protocol/firmware features
QString MainWindow::anykeySave(const QStringList &arguments)
{
    QString home_path = QCoreApplication::applicationDirPath();
    QProcess anykey_save;
    anykey_save.setProcessChannelMode(QProcess::MergedChannels);
    anykey_save.setWorkingDirectory(home_path);
    anykey_save.start(home_path + "/" + ANYKEY_SAVE, arguments);

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    if (!anykey_save.waitForStarted()) {
        QMessageBox::critical(this, tr("AnyKey"),
                              tr("Installation error\n"
                                 "The anykey_save utility is not found in it's location:") +
                                  home_path + tr("\nPlease try re-installing the AnyKey software."),
                              QMessageBox::Ok);
        setStatus("Installation error!");
        return "";
    }

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    // show error if saving to anykey failed somehow
    if (!anykey_save.waitForFinished()) {
        setStatus("Write error:" + anykey_save.errorString());
        return "";
    }

    QByteArray save_output = anykey_save.readAll();
    QString output_str = QString(save_output);

    return output_str;
}

QString MainWindow::anykeySavePassword(const QString &password)
{
    setStatus("Saving password...");
    QString result = "password_saved";

    if (ui->addReturn->isChecked()) {
        if (ui->copyProtectToggle->isChecked()) {
            runCommand("write_password_cps:" + password + "\n");
        }
        else {
            runCommand("write_password_s:" + password + "\n");
        }
    }
    else { // skip return
        if (ui->copyProtectToggle->isChecked()) {
            runCommand("write_password_cpn:" + password + "\n");
        }
        else {
            runCommand("write_password_n:" + password + "\n");
        }
    }

    return result;
}

void MainWindow::anykeyFactoryReset()
{
    runCommand("format_eeprom\n");
}

void MainWindow::runCommand(const QString &command)
{
    // check if command is not already on queue
    for (int i = 0; i < commands_queue.length(); i++) {
        if (commands_queue.at(i) == command) return;
    }

    // add it to queue so that we execute it once anykey is connected
    commands_queue << command;
}

// read settings from anykey returns something like this
// "version=2.5; kbd_conf=222; kbd_delay_start=255; kbd_delay_key=255; kbd_layout=255; bootlock=255; id=ffffff; device_mapped=1\n"
// here we parse it and set the advanced setting controls etc.
// in case device_mapped is 0 it means it has a salt (probably from other pc) and we allow
// reading it if you know the password to link it on other pc.
void MainWindow::anykeyParseSettings(const QString &response)
{
    QString result = response;
    if (result.contains("ERROR: ")) {
        setStatus(result);
        ui->advancedSettingsToggle->setChecked(false);

        if (result.contains("ERROR: could not find a connected")) {
            result += "\nPlease re-insert your AnyKey and try again.";
            QMessageBox::warning(this, tr("AnyKey"), result, QMessageBox::Ok);

            setStatus("ERROR: Could not read settings");
            ui->copyProtectToggle->setChecked(false);
            ui->copyProtectToggle->setEnabled(false);
        }
        return;
    }

    result = result.split("settings=").at(1);

    QString firmware_version = "";
    int kbd_delay_start = 1;
    int kbd_delay_key = 1;
    int keyboard_layout = 0;
    int kbd_config = 0;
    int bootlock = 255;
    int deviceInMapping = 1;
    QString deviceId = "";

    QStringList settings = result.split(";");
    for (int i = 0; i < settings.size(); i++) {
        QString kv = settings.at(i).toLocal8Bit();
        QStringList keyvalue = kv.split("=");
        if (keyvalue.at(0).contains("version")) firmware_version = keyvalue.at(1);
        if (keyvalue.at(0).contains("kbd_conf")) kbd_config = keyvalue.at(1).toInt();
        if (keyvalue.at(0).contains("kbd_delay_start")) kbd_delay_start = keyvalue.at(1).toInt();
        if (keyvalue.at(0).contains("kbd_delay_key")) kbd_delay_key = keyvalue.at(1).toInt();
        if (keyvalue.at(0).contains("kbd_layout")) keyboard_layout = keyvalue.at(1).toInt();
        if (keyvalue.at(0).contains("bootlock")) bootlock = keyvalue.at(1).toInt();
        if (keyvalue.at(0).contains("id")) deviceId = keyvalue.at(1);
        if (keyvalue.at(0).contains("device_mapped")) deviceInMapping = keyvalue.at(1).toInt();
    }

    // 0 -> belgian french azerty
    // 5 -> qwerty
    if (keyboard_layout == 255) keyboard_layout = 5; // default value 5 when factory reset;
    if (kbd_delay_key == 255) kbd_delay_key = 0;
    if (kbd_delay_start == 255) kbd_delay_start = 0;

    ui->keyboardLayoutBox->setCurrentIndex(keyboard_layout);
    ui->keypressDelaySlider->setValue(kbd_delay_key);
    ui->startupDelaySlider->setValue(kbd_delay_start);

    if (bootlock == 187) {
        ui->bootloaderStatus->setText("locked");
    }
    if ((bootlock == 255) || (bootlock == 186) || (bootlock == 185)) {
        ui->bootloaderStatus->setText("unlocked");
    }

    if (kbd_config == 222) {
        ui->copyProtectToggle->setChecked(false);
    }
    if (kbd_config == 220) {
        ui->copyProtectToggle->setChecked(true);
    }

    ui->deviceId->setText(deviceId);
    if (deviceId == "ffffff") {
        ui->deviceId->setText("Blank");
        ui->saltStatus->setText("Unprotected");
        ui->copyProtectToggle->setEnabled(false);
        ui->copyProtectToggle->setChecked(false); // when blank, copy protect is false
        ui->updateSaltButton->setEnabled(true);
        ui->updateSaltButton->setText("Create Secret");
        // ui->daemonAutoType->setChecked(false); // is confusing when it twitches on/off->however should be off when there is no c.p.
        ui->typeButton->setEnabled(false); // disable the CR typing until salt is set
    }
    else {
        ui->deviceId->setText(deviceId);
        if (deviceInMapping == 1) {
            ui->saltStatus->setText("Secured");
            ui->copyProtectToggle->setEnabled(true);
            ui->updateSaltButton->setEnabled(true);
            ui->updateSaltButton->setText("Change Secret");
            ui->typeButton->setEnabled(true);
        }
        else {
            ui->saltStatus->setText("Unknown");
            ui->copyProtectToggle->setEnabled(false);
            ui->updateSaltButton->setEnabled(true);
            ui->updateSaltButton->setText("Read Secret");
            ui->typeButton->setEnabled(false);
        }
    }

    ui->firmwareVersion->setText(firmware_version);
    if (newerFirmwareAvailable(firmware_version)) {
        ui->upgradeFirmwareButton->setEnabled(true);
        setStatus("AnyKey ready, please upgrade firmware!");
        showMessage("AnyKey firmware outdated", "Please click on 'Upgrade Firmware' to update your AnyKey firmware");
    }
    else {
        setStatus("AnyKey ready");
    }
}

bool MainWindow::newerFirmwareAvailable(const QString &firmware_version)
{
    // TODO: download + check version in firmware.bin
    // compare and if current version is lower return true

    return true; // right now always enable upgrade button.
}

// Save settings with write_flags: ...
// used when you click save with empty pass
// quickly modifies addReturn and copyProtect flags
void MainWindow::anykeySaveSettings()
{
    int activateCopyProtect = 0;
    int activateReturnPress = 0;
    if (ui->copyProtectToggle->isEnabled()) {
        if (ui->copyProtectToggle->isChecked()) {
            activateCopyProtect = 1;
        }
    }
    if (ui->addReturn->isChecked()) {
        activateReturnPress = 1;
    }

    QString write_flags = "write_flags:";
    write_flags += QString::number(activateReturnPress) + " ";
    write_flags += QString::number(activateCopyProtect) + " ";
    write_flags += "\n";

    qDebug() << "runCommand=" << write_flags << endl;
    runCommand(write_flags);

    readGuiControls(); // store flags to disk also
}

// Save settings with write_settings: ...
void MainWindow::anykeySaveAdvancedSettings()
{
    setStatus("Saving configuration...");

    if (ui->daemonAutoLock->isChecked()) {
        runCommand("autolock_on\n");
    }
    else {
        runCommand("autolock_off\n");
    }

    int kbd_conf = 222;
    if (ui->copyProtectToggle->isChecked()) {
        kbd_conf = 220;
    }
    int kbd_delay_start = ui->startupDelaySlider->value();
    int kbd_delay_key = ui->keypressDelaySlider->value();
    int kbd_layout = ui->keyboardLayoutBox->currentIndex();

    QString cmd = "write_settings:";
    cmd += QString::number(kbd_conf) + " ";
    cmd += QString::number(kbd_delay_start) + " ";
    cmd += QString::number(kbd_delay_key) + " ";
    cmd += QString::number(kbd_layout);

    runCommand(cmd + "\n");
}

// this is later returned. and we'll auto add it to our devices.map upon success
// in anykey_crd. Basically a read_salt command now not only returns
// salt + devid but also updates the devices.map file so upon next C.R. it works
void MainWindow::anykeyReadSalt(const QString &password)
{
    runCommand("read_salt:" + password + "\n");
}

// called after write_salt with response of device
// show new device id and show secured in gui
void MainWindow::parseWriteSaltResponse(const QString &result)
{
    // code is 3 bytes device_id+ 32 bytes as salt
    // QString result = anykeySave( QStringList() << "-salt" << code );
    // qDebug() << "result="<<result<<endl;
    if (result.contains("ERROR:")) {
        QMessageBox::warning(this, tr("AnyKey"), result, QMessageBox::Ok);
        setStatus("Saving new secret failed.");
        return;
    }
    else {
        if (result.contains("salt saved to device")) {
            QString devId = result.right(4).left(3);
            setStatus("Salt saved your new device id=" + devId);
            ui->copyProtectToggle->setEnabled(true);
            ui->deviceId->setText(devId);
            ui->saltStatus->setText("Secured");
            setStatus("New secret saved");
            runCommand("read_settings\n");
        }
    }
}

void MainWindow::anykeyUpdateSalt(const QString &code)
{
    setStatus("Saving salt and device id...");
    runCommand("write_salt:" + code + "\n");
    runCommand("read_settings\n");
}

void MainWindow::on_updateSaltButton_clicked()
{
    if (ui->updateSaltButton->text().contains("Read Secret")) {
        setStatus("Reading device salt using password...");
        anykeyReadSalt(ui->passwordEdit->text());
    }
    else { // update or generate salt
        // TODO: future work we keep same device id
        // and only change salt. for now we re-generate everything
        QString code = randomString(35); // 3 for dev id, 32 for salt
        anykeyUpdateSalt(code);
    }
}

void MainWindow::on_actionAbout_triggered()
{
    // this works but with the main window minimized this now closes entire app
    QMessageBox mbox;
    mbox.setText(tr("AnyKey Configurator\n"
                    "AnyKey is a product of AnyKey bvba.\n"
                    "Patented in Belgium BE1019719 (A3).\n\n") +
                 QString("Version : ") + QString(CONFIGURATOR_VERSION) + " \n" + "Author  : Walter Schreppers\n");
    mbox.exec();

    // TODO: fix bug seen where if you minimize then it closes app
    // by setting qapp closeevents to something else...
}

void MainWindow::on_showPassword_clicked(bool checked)
{
    if (checked) {
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
    }
    else {
        ui->passwordEdit->setEchoMode(QLineEdit::Password);
    }
}

QString MainWindow::randomString(int password_length)
{
    QString randomPass = "";
    while (randomPass.length() < password_length) {
        randomPass += QUuid::createUuid().toString();
    }
    randomPass.replace("{", "");
    randomPass.replace("-", "");
    randomPass.replace("}", "");
    randomPass = randomPass.left(password_length);

    QString genPass = "";
    int pseudoRand = 1;
    for (int i = 0; i < randomPass.length(); i++) {
        pseudoRand = (pseudoRand + 3) % 13;
        if (pseudoRand > 5)
            genPass += randomPass.at(i).toUpper();
        else
            genPass += randomPass.at(i).toLower();
    }
    return genPass;
}

void MainWindow::on_generateButton_clicked()
{
    ui->passwordEdit->setText(randomString(ui->generateLength->value()));
    ui->passwordEdit->setFocus();
}

void MainWindow::on_passwordEdit_textChanged(const QString &)
{
    // setStatus("");
}

void MainWindow::on_keyboardLayoutBox_currentIndexChanged(int index)
{
    qDebug() << "keyboardLayoutBox index=" << index << endl;
}

void MainWindow::hideAdvancedItems()
{
    ui->keyboardLayoutBox->hide();
    ui->startupDelaySlider->hide();
    ui->keypressDelaySlider->hide();
    ui->restoreDefaultsBtn->hide();
    ui->applyAdvancedSettingsBtn->hide();
    ui->labelLayout->hide();
    ui->labelStartDelay->hide();
    ui->labelKeypressDelay->hide();
    ui->startupDelaySpin->hide();
    ui->keypressDelaySpin->hide();

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
    // ui->daemonAutostartCheck->hide();
    // ui->daemonAutoType->hide(); //this is now in regular settings
    ui->daemonAutoLock->hide();
    ui->daemonRestartButton->hide();
    ui->typeButton->hide();

#ifdef __APPLE__
    this->setMinimumWidth(600);
    this->setMinimumHeight(198); // 178 without mainProgressBar
    this->setMaximumHeight(198);
#else
    // windows, linux
    /*
        this->setMinimumWidth(800); //520
        this->setMinimumHeight(237); //187
        this->setMaximumHeight(237);
    */

    this->setMinimumWidth(600);
    this->setMinimumHeight(198); // 178 without mainProgressBar
    this->setMaximumHeight(198);
#endif
}

void MainWindow::on_advancedSettingsToggle_clicked(bool checked)
{
    if (checked) {
        runCommand("read_settings\n");
    }
    if (ui->advancedSettingsToggle->isChecked()) {
        ui->keyboardLayoutBox->show();
        ui->startupDelaySlider->show();
        ui->keypressDelaySlider->show();
        ui->restoreDefaultsBtn->show();
        ui->applyAdvancedSettingsBtn->show();
        ui->labelLayout->show();
        ui->labelStartDelay->show();
        ui->labelKeypressDelay->show();
        ui->startupDelaySpin->show();
        ui->keypressDelaySpin->show();

        ui->firmwareVersion->show();
        ui->firmwareVersionLabel->show();
        ui->bootloaderLabel->show();
        ui->bootloaderStatus->show();
        ui->upgradeFirmwareButton->show();
        ui->upgradeFirmwareButton->setEnabled(false);
        ui->formatEepromButton->show();

        ui->daemonLabel->show();
        ui->daemonStatusLabel->show();
        // ui->daemonAutostartCheck->show(); // moved to regular small window
        ui->daemonAutoType->show();
        // ui->daemonAutoType->setEnabled(true);

        // this is for a next release (as we still have bugs here)
        // ui->daemonAutoLock->show();
        // ui->daemonAutoLock->setEnabled(true);
        ui->daemonAutoLock->hide();

        ui->daemonRestartButton->show();
        ui->typeButton->show();

        ui->deviceId->show();
        ui->deviceIdLabel->show();
        ui->saltStatus->show();
        ui->updateSaltButton->show();
#ifdef __APPLE__
        this->setMinimumWidth(600);
        this->setMinimumHeight(440); // 420 without mainProgressBar
        this->setMaximumHeight(440);
#else
        // windows and linux
        /*
         this->setMinimumWidth(800); //520 looks good on 200% and 150% scale
         this->setMinimumHeight(580); //380 but width 640 and height 420 is minimum for 100% scale on retina...
         this->setMaximumHeight(580);
        */
        this->setMinimumWidth(600);
        this->setMinimumHeight(400); // 380 without mainProgressBar
        this->setMaximumHeight(400);
#endif
    }
    else { // hide items
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
    reply = QMessageBox::question(this, "AnyKey Factory Reset",
                                  "This reset's your device to factory settings. \nWarning: It removes all passwords and salts saved on "
                                  "your AnyKey!\n Are you sure you want to do this?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "Ok was clicked";
    }
    else {
        qDebug() << "Cancel clicked";
        return;
    }

    setStatus("Formatting anykey eeprom...");
    anykeyFactoryReset();

    // close advanced settings
    ui->advancedSettingsToggle->setChecked(false);
    on_advancedSettingsToggle_clicked(false);

    // after device reset copy protect is by default disabled.
    ui->copyProtectToggle->setEnabled(false);
}

void MainWindow::on_upgradeFirmwareButton_clicked()
{
    setStatus("Upgrading firmware...");
    runCommand("upgrade_firmware\n");
}

void MainWindow::on_daemonRestartButton_clicked()
{
    if (ui->daemonRestartButton->text() == "Stop") {
        setStatus("Stopping CR daemon...");
        stopAnykeyCRD();
        ui->daemonRestartButton->setText("Start");
        setStatus("");
    }
    else {
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

void MainWindow::on_daemonAutoType_toggled(bool checked)
{
    this->writeGuiControls();
}
