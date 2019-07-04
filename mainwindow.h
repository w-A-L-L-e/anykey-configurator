/*=============================================================================
author        : Walter Schreppers
filename      : mainwindow.h
created       : 3/6/2017
modified      :
version       :
copyright     : Walter Schreppers
bugreport(log):
=============================================================================*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QProcess>
#include <QProgressBar>
#include <QSystemTrayIcon>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setStatus(const QString &msg);

  protected:
    void closeEvent(QCloseEvent *event);

  private slots:
    void setIcon(int index);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    bool checkLicense();
    bool registerLicense(QString activation_code);
    QString anykeySave(const QStringList &arguments);
    QString anykeySavePassword(const QString &password);

    void showConfigurator();

    void checkShowWindow();
    void readCRD();
    void startAnykeyCRD();
    void stopAnykeyCRD();

    void runCommand(const QString &command);

    void showRegisteredControls(); // show all controls if registration checked
    void anykeyParseSettings(const QString &);
    void anykeySaveSettings();
    void anykeySaveAdvancedSettings();

    void anykeyFactoryReset();

    void anykeyReadSalt(const QString &password);
    void parseWriteSaltResponse(const QString &);
    void anykeyUpdateSalt(const QString &code);

    void readGuiControls();
    void writeGuiControls();
    void on_saveButton_clicked();

    void on_actionClose_triggered();
    void on_actionAbout_triggered();

    void on_showPassword_clicked(bool checked);

    QString randomString(int password_length);
    void on_generateButton_clicked();
    void on_passwordEdit_textChanged(const QString &arg1);
    void on_keyboardLayoutBox_currentIndexChanged(int index);

    void on_advancedSettingsToggle_clicked(bool checked);
    void hideAdvancedItems();

    void on_startupDelaySlider_valueChanged(int value);
    void on_keypressDelaySlider_valueChanged(int value);
    void on_restoreDefaultsBtn_clicked();
    void on_startupDelaySpin_valueChanged(int arg1);
    void on_keypressDelaySpin_valueChanged(int arg1);
    void on_applyAdvancedSettingsBtn_clicked();
    void on_formatEepromButton_clicked();
    void on_upgradeFirmwareButton_clicked();
    void on_updateSaltButton_clicked();
    void on_daemonRestartButton_clicked();

    void on_typeButton_clicked();
    void appFocusChanged(Qt::ApplicationState);

    void typePasswordAgain();
    void on_actionType_password_triggered();
    void on_actionOpen_triggered();
    void on_actionHide_triggered();
    void toggleAutostart();

  private:
    Ui::MainWindow *ui;

    void createTrayIcon();
    void createActions();
    void showMessage(const QString &title, const QString &msg, int seconds = 8);

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *typeAction;
    QAction *quitAction;
    QAction *autostartAction;

    QProcess *anykeycrd;
    bool licenseRegistered; // cache once it's registered
    bool trayIconActivated;

    // add references to Label and ProgressBar
    QLabel *statusLabel;
    QProgressBar *statusProgressBar;
    QString statusMessage;
    QStringList commands_queue;
    bool bPendingPasswordType;

    QTimer *windowTimer;
};

#endif // MAINWINDOW_H
