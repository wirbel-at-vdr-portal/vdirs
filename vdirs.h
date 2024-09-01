/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#pragma once

#include <vdr/plugin.h>
#include "multidir.h"

class cPluginVdirs : public cPlugin {
private:
  MultiVideoDir* impl;
  std::string mountprefix;
  std::string DiskSeq;
  bool balance;
public:
  cPluginVdirs(void);
  virtual ~cPluginVdirs() {}
  virtual const char *Version(void);
  virtual const char *Description(void);
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int count, char* args[]);
  virtual bool Initialize(void) { return true; }
  virtual bool Start(void);
  virtual void Stop(void) {}
  virtual void Housekeeping(void) {}
  virtual void MainThreadHook(void) {}
  virtual cString Active(void) { return NULL; }
  virtual time_t WakeupTime(void) { return 0; }
  virtual const char *MainMenuEntry(void)  { return NULL; }
  virtual cOsdObject *MainMenuAction(void) { return NULL; }
  virtual cMenuSetupPage *SetupMenu(void)  { return NULL; }
  virtual bool SetupParse(const char* Name, const char* Value);
  virtual bool Service(const char *Id, void *Data = NULL) { return false; }
  virtual const char **SVDRPHelpPages(void) { return impl->SVDRPHelpPages(); }
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
     std::string Opt;
     if (Option) Opt = Option;
     return impl->SVDRPCommand(std::string(Command), Opt, ReplyCode);
     }
};
