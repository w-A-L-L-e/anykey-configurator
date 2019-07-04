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
    // constructor & destructor
    //==========================
    LaunchAgent();
    ~LaunchAgent();

    // public members
    //==============
    void install();
    void uninstall();
    string getPath()
    {
        return plist_path;
    }
    bool is_installed();

  private:
    // private members:
    //================
    void init();
    string runCommand(const string &command);
    string getHomeDir();

    // private locals:
    //===============
    string autoStartPlist();
    string autoRestartPlist();

    string plist_path; // stores home_dir/plist_file_path

}; // end of class LaunchAgent

#endif
