/*=============================================================================
author        : Walter Schreppers
filename      : launchagent.cpp
created       : 2/7/2019 at 00:41:32
modified      : 
version       : 
copyright     : Walter Schreppers
bugreport(log): 
=============================================================================*/

#include "launchagent.h"
#include <iostream>
#include <fstream>
#include <string>

#include <unistd.h> // execvp()
#include <stdio.h>  // perror()
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE

using namespace std;

/*-----------------------------------------------------------------------------
name        : init
description : initialize private locals
parameters  : 
return      : void
exceptions  : 
algorithm   : trivial
-----------------------------------------------------------------------------*/
void LaunchAgent::init(){
}


/*-----------------------------------------------------------------------------
name        : LaunchAgent
description : constructor
parameters  : 
return      : 
exceptions  : 
algorithm   : trivial
-----------------------------------------------------------------------------*/
LaunchAgent::LaunchAgent(){
  init();
}


/*-----------------------------------------------------------------------------
name        : ~LaunchAgent
description : destructor
parameters  : 
return      : 
exceptions  : 
algorithm   : trivial
-----------------------------------------------------------------------------*/
LaunchAgent::~LaunchAgent(){

}


/*-----------------------------------------------------------------------------
name        : runCommand
description : Allows running shell like scripting commands with popen
parameters  : string command
return      : output from command as string
exceptions  : 
algorithm   : use popen
-----------------------------------------------------------------------------*/
string LaunchAgent::runCommand( const string& command ){
  FILE *pstream;
  if(  ( pstream = popen( command.c_str(), "r" ) ) == NULL ) return "";

  string Line;
  char buf[255];

  while( fgets(buf, sizeof(buf), pstream) !=NULL){
    Line += buf;
  }
  pclose(pstream);

  return Line;
}


/*-----------------------------------------------------------------------------
name        : autoRestartPlist 
description : Make configurator launch at boot but also re-launch if it crashes
              this is DEPRECATED as it doesn't allow closing either. 
              we keep the code for a future project ;)
parameters  : 
return      : plist content as string 
exceptions  : 
algorithm   : 
-----------------------------------------------------------------------------*/
string LaunchAgent::autoRestartPlist(){
  string plist = "";

  plist += "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n";
  plist += "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"> \n";
  plist += "<plist version=\"1.0\"> \n";
  plist += "  <dict> \n";
  plist += "    <key>Label</key> \n";
  plist += "    <string>org.anykey.configurator</string> \n";
  plist += "    <key>LimitLoadToSessionType</key> \n";
  plist += "    <array> \n";
  plist += "      <string>Aqua</string> \n";
  plist += "    </array> \n";
  plist += "    <key>ProgramArguments</key> \n";
  plist += "    <array> \n";
  plist += "      <string>/Applications/AnyKey.app/Contents/MacOS/AnyKey</string> \n";
  plist += "    </array> \n";
  plist += "    <key>KeepAlive</key> \n";
  plist += "    <true/> \n";
  plist += "    <key>Disabled</key> \n";
  plist += "    <false/> \n";
  plist += "  </dict> \n";
  plist += "</plist> \n";

  return plist;
}


/*-----------------------------------------------------------------------------
name        : autoSartPlist 
description : plist file for launchctl to autostart configurator on boot
              also this is MacOs specific!
parameters  : 
return      : plist content as string 
exceptions  : 
algorithm   : 
-----------------------------------------------------------------------------*/
string LaunchAgent::autoStartPlist(){
  string plist = "";

  plist += "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n";
  plist += "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"> \n";
  plist += "<plist version=\"1.0\"> \n";
  plist += "  <dict> \n";
  plist += "    <key>Label</key> \n";
  plist += "    <string>org.anykey.configurator</string> \n";
  plist += "    <key>LimitLoadToSessionType</key> \n";
  plist += "    <array> \n";
  plist += "      <string>Aqua</string> \n";
  plist += "    </array> \n";
  plist += "    <key>ProgramArguments</key> \n";
  plist += "    <array> \n";
  plist += "      <string>/Applications/AnyKey.app/Contents/MacOS/AnyKey</string> \n";
  plist += "      <string>minimized</string> \n";
  plist += "    </array> \n";
  plist += "    <key>KeepAlive</key> \n";
  plist += "    <false/> \n";
  plist += "    <key>RunAtLoad</key> \n";
  plist += "    <true/> \n";
  plist += "  </dict> \n";
  plist += "</plist> \n";

  return plist;
}


/*-----------------------------------------------------------------------------
name        : install
description : write to plist file and load it with launchctl. After doing
              this once then each reboot it's executed. 
              (Similar to windows like the HKLM/.../run registry key)
              And also this is MacOs specific!
parameters  : 
return      : 
exceptions  : 
algorithm   : 
-----------------------------------------------------------------------------*/
void LaunchAgent::install(){
#ifdef __APPLE__
  //truncate any old content and write in local lib folder (like we see other apps do
  //examples are pulsesecure postgres agent etc.
  ofstream lf("~/Library/LaunchAgents/org.anykey.configurator.plist", ofstream::trunc);
  lf << autoStartPlist() << endl;
  lf.close();

  string res = runCommand("launchctl load ~/Library/LaunchAgents/org.anykey.configurator.plist"); 
#else
  cerr << "Unsupported platform detected for launchctl. This is Mac OS only!"<<endl;
#endif
}


/*-----------------------------------------------------------------------------
name        : uninstall
description : make configurator not autostart again on next reboot. remove
              plist file and unload it with launchctl
              Mac os specific code, give error on other platforms!
parameters  : 
return      : plist content as string 
exceptions  : 
algorithm   : 
-----------------------------------------------------------------------------*/
void LaunchAgent::uninstall(){
#ifdef __APPLE__
  string res = runCommand("launchctl unload ~/Library/LaunchAgents/org.anykey.configurator.plist"); 

  //remove the plist file
  res = runCommand("rm -f ~/Library/LaunchAgents/org.anykey.configurator.plist");
#else
  cerr << "Unsupported platform detected for launchctl. This is Mac OS only!"<<endl;
#endif
}

