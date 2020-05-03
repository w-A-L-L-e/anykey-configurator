/*=============================================================================
author        : Walter Schreppers
filename      : main.cpp
created       : 3/6/2017
modified      :
version       :
copyright     : Walter Schreppers
bugreport(log):
=============================================================================*/

#include "mainwindow.h"
#include <QApplication>

// these are used for single startup
#include <QMessageBox>
#include <QSharedMemory>
#include <QSystemSemaphore>

#include <iostream>

// The sharedmemory ipc stuff is a bit messy, we might refactor this into a seperate class later on
// for now it works really well so we're keeping it in. It basically only allows one configurator to
// be started and also signals a running configurator to pop to front when this one exits (after detecting
// it's not the first one started)
// This is tightly coupled with mainwindow method checkShowWindow. And basically moving common code into
// a seperate class like we did on mac for launchctl would be nicer here...
int main(int argc, char *argv[])
{
    // make windows with retina or 4k still look good
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    QSystemSemaphore semaphore("AnyKey Configurator Semaphore", 1); // create semaphore
    semaphore.acquire(); // Raise the semaphore, barring other instances to work with shared memory

//#ifndef Q_OS_WIN32
#ifndef _WIN32
    // in linux / unix shared memory is not freed when the application terminates abnormally,
    // so you need to get rid of the garbage
    QSharedMemory nix_fix_shared_memory("AnyKey Configurator Shared");
    if (nix_fix_shared_memory.attach()) {
        nix_fix_shared_memory.detach();
    }
#endif

    QSharedMemory sharedMemory("AnyKey Configurator Shared"); // Create a copy of the shared memory
    bool is_running;                                          // variable to test the already running application

    if (sharedMemory.attach()) { // We are trying to attach a copy of the shared memory
                                 // To an existing segment
        is_running = true;       // If successful, it determines that there is already a running instance

        // put an S for show -> this will be detected by the other running configurator!
        sharedMemory.lock();
        char *data = (char *)sharedMemory.data();
        data[0] = 'S'; // show window on other instance
        sharedMemory.unlock();
    }
    else {
        sharedMemory.create(1); // Otherwise allocate 1 byte of memory
        is_running = false;     // And determines that another instance is not running

        // put an ' ' to signal nothing to itself upon startup
        sharedMemory.lock();
        char *data = (char *)sharedMemory.data();
        data[0] = ' '; // have empty byte ready for other app
        sharedMemory.unlock();
    }
    semaphore.release();

    // If you already run one instance of the application, then we inform the user about it
    // and complete the current instance of the application
    if (is_running) {
      std::cout << "The application was already started before. Exiting and bringing other instance to front... " << std::endl;
      return 1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}
