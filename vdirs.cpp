/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#include "vdirs.h"

static const char *VERSION        = "1.0.0";
static const char *DESCRIPTION    = "multiple video directories";

VDRPLUGINCREATOR(cPluginVdirs); // Don't touch this!




#include <string>
#include <iostream>
/*******************************************************************************
 * cPluginVdirs.
 *
 * Everything interesting goes to class MultiVideoDir,
 * but MultiVideoDir is created *later* as a plugin class. Also, the
 * MultiVideoDir may be destroyed *earlier*. Only early on startup needed
 * functionality during startup goes here, as MultiVideoDir class gets created
 * later.
 ******************************************************************************/

class cPluginVdirs* PluginVdirs;

cPluginVdirs::cPluginVdirs(void) : mountprefix("/mnt/video"), DiskSeq("0"), balance(true) {
  PluginVdirs = this;
}

/* called before MultiVideoDir constructor. */
const char* cPluginVdirs::Version(void) {
  return VERSION;
}

/* called before MultiVideoDir constructor. */
const char* cPluginVdirs::Description(void) {
  return DESCRIPTION;
}

/* called before MultiVideoDir constructor. */
const char* cPluginVdirs::CommandLineHelp() {
  return "  -m path,  --mount path   use disk mount prefix, ie. /video\n"
         "                           for /video0../videoN\n"
         "                           default: /mnt/video\n"
         "  -b <0,1>, --balance <0,1> allow or forbid disk balancing algo\n"
         "                           for reordering files on disks.\n"
         "                           1 = allow, any other value: forbid\n"
         "                           default: 1 (allow)\n";
}

/* called before MultiVideoDir constructor. */
bool cPluginVdirs::ProcessArgs(int count, char* args[]) {
  for(int i = 1; i < count; i++) {
     std::string option(args[i]);
     std::string value;
     bool has_argument = ((i+1) < count);

     if (has_argument)
        value = args[i+1];

     if ((option == "--mount") or (option == "-m")) {
        if (!has_argument) {
           std::cerr << PluginVdirs->Name() << ": ERROR: missing arg for mount" << std::endl;
           return false;
           }
        mountprefix = value;
        i++;
        }
     else if ((option == "--balance") or (option == "-b")) {
        if (!has_argument) {
           std::cerr << Name() << ": ERROR: missing arg for balance" << std::endl;
           return false;
           }
        balance = (value == "1");
        i++;
        }
     else {
        std::cerr << Name() << ": ERROR: unknown option '" << option << "'" << std::endl;
        return false;
        }
     }
  return true;
}

/* called before MultiVideoDir constructor. */
bool cPluginVdirs::SetupParse(const char* Name, const char* Value) {
  std::string s(Name);
  if (s == "DiskSeq") {
     DiskSeq = Value;
     return true;
     } 

  return false;
}

/* calls MultiVideoDir constructor. */
bool cPluginVdirs::Start(void) {
  impl = new MultiVideoDir(mountprefix, DiskSeq, balance);
  return true;
}
