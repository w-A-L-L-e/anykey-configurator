/*=============================================================================
author        : Walter Schreppers
filename      : launchagent.h
created       : 2/7/2019 at 00:41:32
modified      : 
version       : 
copyright     : Walter Schreppers
bugreport(log): 
=============================================================================*/

#ifndef LAUNCHAGENT_H
#define LAUNCHAGENT_H

#include <string>
using namespace std;

class LaunchAgent {

  public:
    //constructor & destructor
    //==========================
    LaunchAgent();
    ~LaunchAgent();

    //public members
    //==============
    void install();
    void uninstall();

  private:
    //private members:
    //================
    void init();
    string runCommand(const string& command);

    //private locals:
    //===============
    string autoStartPlist();
    string autoRestartPlist();

}; //end of class LaunchAgent

#endif

