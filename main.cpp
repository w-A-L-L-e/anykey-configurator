#include "mainwindow.h"
#include <QApplication>

//these are used for single startup
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>

#include <iostream>
using namespace std;


int main(int argc, char *argv[])
{
    // make windows with retina or 4k still look good
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    QSystemSemaphore semaphore("AnyKey Configurator Semaphore", 1);  // create semaphore
    semaphore.acquire(); // Raise the semaphore, barring other instances to work with shared memory

//#ifndef Q_OS_WIN32
#ifndef _WIN32
    // in linux / unix shared memory is not freed when the application terminates abnormally,
    // so you need to get rid of the garbage
    QSharedMemory nix_fix_shared_memory("AnyKey Configurator Shared");
    if(nix_fix_shared_memory.attach()){
        nix_fix_shared_memory.detach();
    }
#endif

    QSharedMemory sharedMemory("AnyKey Configurator Shared");  // Create a copy of the shared memory
    bool is_running;            // variable to test the already running application

    if (sharedMemory.attach()){ // We are trying to attach a copy of the shared memory
                                // To an existing segment
        is_running = true;      // If successful, it determines that there is already a running instance
    }else{
        sharedMemory.create(1); // Otherwise allocate 1 byte of memory
        is_running = false;     // And determines that another instance is not running
    }
    semaphore.release();


    // If you already run one instance of the application, then we inform the user about it
    // and complete the current instance of the application
    if(is_running){
      cout << "The application is already running. Exiting this instance." << endl;;
      return 1;
    }


    MainWindow w;
    w.show();

    return a.exec();
}

