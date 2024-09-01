/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#pragma once
#include <string>
#include <vector>
#include <vdr/videodir.h>

class Equalizer;

/******************* Plugins.html (vdr-2.3.8) **********************************
 * The video directory
 * -------------------
 * By default VDR assumes that the video directory consists of one large volume,
 * on which it can store its recordings. If you want to distribute your
 * recordings over several physical drives, you can derive from cVideoDirectory.
 *
 * See the description in videodir.h for details.
 * You should create your derived video directory object in the Start() function
 * of your plugin. Note that the object has to be created on the heap (using
 * new), and you shall not delete it at any point (it will be deleted auto-
 * matically when the program ends).
 ******************************************************************************/
class MultiVideoDir : public cVideoDirectory {
friend class DiskUse;
private:
  std::string videodir;
  std::string mountprefix;
  Equalizer* eq;
  bool balance;
  bool debug;

  // private versions with c++11 strings
  bool Register(std::string FileName);
  bool Rename(std::string From, std::string To);
  bool Move(std::string From, std::string To);
  bool Remove(std::string Name);
  bool Contains(std::string Name);

  //void ImportVideo(std::string Disk, std::string TopSrc, std::string Dir, bool DryRun);
  void Balance();
public:
  MultiVideoDir(std::string Prefix, std::string Seq, bool Balancing);
  virtual ~MultiVideoDir() {}
  /**** VDRs cVideoDirectory Interface ******************************************/
  virtual int FreeMB(int* UsedMB = NULL);
  virtual bool Register(const char* FileName)                   { return Register(std::string(FileName)); }
  virtual bool Rename(const char* OldName, const char* NewName) { return Rename(std::string(OldName), std::string(NewName)); }
  virtual bool Move(const char* FromName, const char* ToName)   { return Move(std::string(FromName), std::string(ToName)); }
  virtual bool Remove(const char* Name)                         { return Remove(std::string(Name)); }
  virtual void Cleanup(const char* IgnoreFiles[] = NULL);
  virtual bool Contains(const char* Name)                       { return Contains(std::string(Name)); }
  /******************************************************************************/
  const char** SVDRPHelpPages();
  const char* SVDRPCommand(std::string Command, std::string Option, int& ReplyCode);
  void SetupStore(const char* Name, const char* Value);
  void Import(std::string Path, bool One, bool DryRun);
};
