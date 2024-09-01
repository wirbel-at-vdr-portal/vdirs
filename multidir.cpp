/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <tuple>
#include <cstdio>      /* remove() */
#include <cmath>       /* lround() */
#include <cstdint>     /* uint8_t */
#include <sys/types.h> /* stat() */
#include <sys/stat.h>  /* stat() */
#include <unistd.h>    /* stat() */
#include <sys/statvfs.h>
#include "multidir.h"
#include "vdirs.h"
#include "fops.h"
#include "workqueue.h"


extern class cPluginVdirs* PluginVdirs;
const size_t mebibyte = 0x100000;
const size_t gibibyte = 0x40000000;
#define      tebibyte   0x10000000000 /* debug only; otherwise not used. */


/*******************************************************************************
 * class DiskInfo
 * Holds info about one of the mounted Disks.
 ******************************************************************************/
class DiskInfo {
public:
  std::string Path;
  size_t Free;
  size_t Total;
  size_t Used;
public:
  DiskInfo(std::string path) : Path(path), Free(0), Total(0), Used(0) {}

  bool GetSpace() {
     struct statvfs s;
     if (statvfs(Path.c_str(), &s)) return false;
     Free  = s.f_bsize * s.f_bavail;
     Total = s.f_bsize * s.f_blocks;
     Used  = Total - Free;
     return true;
     }
};


/*******************************************************************************
 * class Equalizer
 * Manages several video disks.
 ******************************************************************************/
class Equalizer {
friend MultiVideoDir;
private:
  std::string alphabet;
  std::string Prefix;
  std::string DiskSeq;
  std::vector<std::string> DiskChars;
  std::vector<class DiskInfo*> Disks;
  size_t DiskUsePerChar[256];
  WorkQueue<CopyData, void(*)(CopyData&)>* BgTask;
  WorkQueue<ImportData, void(*)(ImportData&)>* BgImportTask;

  void Reset() { for(int i=0; i<256; i++) DiskUsePerChar[i] = 0; }
  void InitDisks();
  void Add(std::string Path);
  void BgCopy(std::string From, std::string To);
  void BgMove(std::string From, std::string To);
  void BgImport(std::string videodir, std::string Disk, std::string Src, std::string Dir);

public:
  Equalizer(std::string DiskPrefix, std::string Seq);
  ~Equalizer();
  std::string SplitEqual();
  void        DiskSpace(size_t& Free, size_t& Used);
  size_t      NumDisks() { return Disks.size(); }
  void        Initialize();
  void        Equalize(bool forced = false);
  std::string Storage(char c);
  static char CharMapping(std::string s);
  bool        ValidSequence() { return Disks.size() == DiskChars.size(); }
};


Equalizer::Equalizer(std::string DiskPrefix, std::string Seq) :
    alphabet("0123456789abcdefghijklmnopqrstuvwxyz"),
    Prefix(DiskPrefix), DiskSeq(Seq)
{
  Reset();
  Initialize();
  InitDisks();
  BgTask = new WorkQueue<CopyData, void(*)(CopyData&)>(&CopyWork);
  BgImportTask = new WorkQueue<ImportData, void(*)(ImportData&)>(&ImportWork);
}

Equalizer::~Equalizer() {
  delete BgTask;
  delete BgImportTask;
}

void Equalizer::BgCopy(std::string From, std::string To) {
  BgTask->Push(std::move(std::make_tuple(From, To, false)));
}

void Equalizer::BgMove(std::string From, std::string To) {
  BgTask->Push(std::move(std::make_tuple(From, To, true)));
}

void Equalizer::BgImport(std::string videodir, std::string Disk, std::string Src, std::string Dir) {
  BgImportTask->Push(std::move(std::make_tuple(videodir, Disk, Src, Dir, false)));
}

// ok. 20180127
void Equalizer::InitDisks() {
  Disks.reserve(256);

  for(size_t i = 0; i < 256; i++) {
     std::string d = Prefix + std::to_string(i);
     if (!DirectoryExists(d)) break;
     Disks.push_back(new DiskInfo(d));
     }
  Disks.shrink_to_fit();
}

// ok. 20180127
void Equalizer::Initialize() {
  DiskChars.clear();
  DiskChars.reserve(DiskSeq.size());

  for(size_t i = 0; i < DiskSeq.size(); i++) {
     size_t p1 = alphabet.find(DiskSeq[i]);
     size_t p2 = (i+1) < DiskSeq.size()? alphabet.find(DiskSeq[i+1]) : 1 + alphabet.find('z');
     DiskChars.push_back(alphabet.substr(p1, p2 - p1));
     }
}

// ok. 20180127
std::string Equalizer::SplitEqual() {
  size_t n = Disks.size();
  size_t d = (0.5 + alphabet.size()) / n;

  DiskChars.clear();
  DiskChars.reserve(Disks.size());
  DiskSeq.clear();

  for(size_t i = 0; i < n; i++) {
     std::string s = alphabet.substr(i * d, d);
     DiskChars.push_back(s);
     DiskSeq += s[0];
     }
  return DiskSeq;
}

// ok. 20180127
void Equalizer::DiskSpace(size_t& Free, size_t& Used) {
  Free = 0;
  Used = 0;

  for(auto disk:Disks) {
     if (!disk->GetSpace()) break;
     Free += disk->Free;
     Used += disk->Used;
     }
}

// ok. 20180127
std::string  Equalizer::Storage(char c) {
  for(size_t i = 0; i < DiskChars.size(); i++)
     if (DiskChars[i].find(c) != std::string::npos) return Disks[i]->Path;
  return Disks[0]->Path;
}

// ok. 20180127
char Equalizer::CharMapping(std::string s) {
  if (s.size() < 1) return '0';
  const char* c = s.c_str(), *p = c;

  if (*p == '/') { if (s.size() < 2) return '0'; p++; }
  if (*p == '%') { if (s.size() < 2) return '0'; p++; }
  
  switch((unsigned char)*p) {
     case 0x00: return '0';
     case 0xC2: return CharMapping(p+2);
     case 0xC3:
         switch((unsigned char)*++p) {
            case 0xA0 ... 0xA6: return 'a';
            case 0x80 ... 0x86: return 'a';
            case 0xA7:          return 'c';
            case 0x87:          return 'c';
            case 0xA8 ... 0xAB: return 'e';
            case 0x88 ... 0x8B: return 'e';
            case 0xAC ... 0xAF: return 'i';
            case 0x8C ... 0x8F: return 'i';
            case 0xB0:          return 'd';
            case 0x90:          return 'd';
            case 0xB1:          return 'n';
            case 0x91:          return 'n';
            case 0xB2 ... 0xB6: return 'o';
            case 0x92 ... 0x96: return 'o';
            case 0xB7:          return CharMapping(p+2);
            case 0x97:          return CharMapping(p+2);
            case 0xB8:          return 'o';
            case 0x98:          return 'o';
            case 0xB9 ... 0xBC: return 'u';
            case 0x99 ... 0x9C: return 'u';
            case 0xBD:          return 'y';
            case 0x9D:          return 'y';
            case 0xBE:          return 't';
            case 0x9E:          return 't';
            case 0x9F:          return 's';
            case 0xBF:          return 'y';
            }
     case 0xC4 ... 0xDF: return CharMapping(p+2);
     case 0xE0 ... 0xEF: return CharMapping(p+3);
     case 0xF0 ... 0xF4: return CharMapping(p+4);
     case '0' ... '9': return *p;
     case 'a' ... 'z': return *p;
     case 'A' ... 'Z': return *p + 32;
     default: return CharMapping(++p);     
     }
  return '0'; /* never reached. */
}

void Equalizer::Add(std::string Path) {
  uint8_t MappedChar;
  for(auto f:DirEntries(Path)) {
     MappedChar = (uint8_t) CharMapping(f);
     DiskUsePerChar[MappedChar] += FileSize(f);
     }
}
  
void Equalizer::Equalize(bool forced) {
  bool RunningShort = forced;
  double Goal = 0;
  Reset();
  for(auto disk:Disks) {
     disk->GetSpace();
     Goal += disk->Free;
     Add(disk->Path);
     if (disk->Free < 100*gibibyte)
        RunningShort = true;
     }
  if (!RunningShort) return;
  Goal /= Disks.size();

  DiskSeq.clear();
  DiskChars.clear();
  std::string letters;
  letters.reserve(alphabet.size());
  size_t last = std::string::npos;

  for(auto disk:Disks) {
     double left = disk->Total;
     for(char c:alphabet) {
        uint8_t u = (uint8_t) c;
        if (last != std::string::npos and u <= last)
           continue;

        double CharUse = DiskUsePerChar[u];
        std::cerr << "char " << c << ":" << CharUse << std::endl;

        if (left < CharUse)
           break;
        if (left >= Goal or !letters.size()) {
           letters += c;
           left -= CharUse;
           last = u;
           }
        else {
           if (c != 'z' and left > DiskUsePerChar[u+1]) {
              double delta  = std::abs(left - Goal);
              double delta2 = std::abs(left - Goal - DiskUsePerChar[u+1]);
              if (delta2 < delta) {
                 CharUse = DiskUsePerChar[++u];
                 letters += alphabet[alphabet.find(c) + 1];
                 left -= CharUse;
                 last = u;
                 }
              }
           break;
           }
        }
     if (letters.size()) {
        DiskSeq.push_back(letters[0]);
        DiskChars.push_back(letters);
        letters.clear();
        }
     else
        std::cerr << __PRETTY_FUNCTION__ << ": hmm.. letters.size() == 0" << std::endl;
     std::cerr << disk->Path << ": " << DiskSeq.back() << std::endl;
     }
}


                              





/*******************************************************************************
 * class MultiVideoDir
 * The actual implementation of multiple video dirs.
 ******************************************************************************/
MultiVideoDir::MultiVideoDir(std::string Prefix, std::string Seq, bool Balancing) :
   videodir(cVideoDirectory::Name()), mountprefix(Prefix), balance(Balancing), debug(false) {

  eq = new Equalizer(Prefix, Seq);
  if (!eq->ValidSequence())
     SetupStore("DiskSeq", eq->SplitEqual().c_str());
}


const char** MultiVideoDir::SVDRPHelpPages() {
 static const char* HelpPages[] = {
    "BALANCE\n"
    "    Startet manuellen Ausgleich des Festplattenplatzes der beteiligten\n",
    "    Disk Partitionen.",
    "IMPORT_NEXT <PATH>\n"
    "    Gibt den den naechsten Ordner unterhalb von PATH zurueck, welchen\n"
    "    der Befehl IMPORT_ONE importieren würde. Zur Benutzung siehe dem\n"
    "    Plugin beiliegendes shell script 'import.sh'\n"
    "      Bsp: $svdrp plug vdirs IMPORT_NEXT /mnt/oldvideo",
    "IMPORT_ONE <PATH>\n"
    "    Importiert den naechsten Ordner unterhalb von PATH in den Video\n"
    "    Ordner. Vor dem Ausfuehren sollte mit IMPORT_ONE_DRYRUN ein\n"
    "    Trockenlauf vorgenommen werden.\n"
    "    ACHTUNG: VERSCHIEBT diesen Ordner, kein KOPIEREN des Ordners!!!",
    "IMPORT_ONE_DRYRUN <PATH>\n"
    "    wie IMPORT_ONE, aber es werden nur die betreffenden Meldungen aus-\n"
    "    gegeben und keine Veraenderungen am Dateisystem vorgenommen.",
    NULL
    };
  return HelpPages;
}

const char* MultiVideoDir::SVDRPCommand(std::string Command, std::string Option, int& ReplyCode) {
  static char b[2048];
  ReplyCode = 900;

  if (Command == "BALANCE") {
     Balance();
     return "BALANCE";
     }
  else if (Command == "REGISTER")   {
     if (Option.size() == 0) return "missing arg";
     Register(Option);
     return "REGISTER";
     }
  else if (Command == "IMPORT_ONE") {
     if (Option.size() == 0) return "missing arg";
     Import(Option, true, false);
     return "IMPORT_ONE";
     }
  else if (Command == "IMPORT_ONE_DRYRUN") {
     if (Option.size() == 0) return "missing arg";
     Import(Option, true, true);
     return "IMPORT_ONE_DRYRUN";
     }
  else if (Command == "IMPORT_NEXT") {
     if (Option.size() == 0) return "missing arg";
     *b = 0;
     for(auto e:DirEntries(Option)) {
        std::string n(Option + "/" + e);
        if (IsDirectory(n)) {
           strcpy(b, n.c_str());
           break;
           }
        }
     return b;
     }
  else if (Command == "DEBUG") {
     if (debug) {
        debug = false;
        return "DEBUG=OFF";
        }
     else {
        debug = true;
        return "DEBUG=ON";
        }
     }

  return NULL;
}

void MultiVideoDir::SetupStore(const char* Name, const char* Value) {
  PluginVdirs->SetupStore(Name, Value);
}


/* Returns the total amount (in MB) of free disk space for recording.
 * If UsedMB is given, it returns the amount of disk space in use by
 * existing recordings (or anything else) on that disk.
 */
// ok. 20180127
int MultiVideoDir::FreeMB(int* UsedMB) {
  size_t Free, Used;
  eq->DiskSpace(Free, Used);
  if (UsedMB) *UsedMB = (0.5 + Used) / mebibyte;
  return (0.5 + Free) / mebibyte;
}


/* Creates a symlink from FileName to location on other disk.
 *  Filename is full path incl. *.ts, beginning with videodir
 */
bool MultiVideoDir::Register(std::string FileName) {
  if (debug) std::cout << "Register(" << FileName << ")" << std::endl;

  /* check if 'FileName' is located on videodir */
  if (FileName.find(videodir) != 0) {
     if (debug) std::cout << "ERROR: not in videodir" << std::endl;
     return false;
     }

  std::string s = FileName.substr(videodir.size() + 1);
  char c = eq->CharMapping(s);

  std::string dest = eq->Storage(c) + '/' + FlatPath(s);

  if (debug) std::cout << "dest = " << dest << std::endl;
  return SymLink(FileName, dest);
}


/* Renames a directory / mark as deleted.
 *   From:  full dir path incl. '*.rec'
 *   To:    full dir path incl. '*.del' */
bool MultiVideoDir::Rename(std::string From, std::string To) {
  if (debug) std::cout << "Rename(" << From << "," << To << ")" << std::endl;
  return ::Rename(From, To);
}


/* Moves a directory / change its Name.
 *    From:  full dir path incl. '*.rec'
 *    To:    full dir path incl. '*.rec' */
bool MultiVideoDir::Move(std::string From, std::string To) {
  ::Rename(From, To);

  std::string subdir = To.substr(videodir.size() + 1);
  std::string nextdisk(eq->Storage(eq->CharMapping(subdir)));

  for(auto e:DirEntries(To)) {
     auto linkname = To + '/' + e;
     if (IsSymlink(linkname) and IsVideoFile(linkname)) {
        std::string linkdest = LinkDest(linkname);
        std::string current_disk = linkdest.substr(0, linkdest.rfind('/'));
        std::string newdest(nextdisk + '/' + FlatPath(subdir));

        if (current_disk == nextdisk)
           ::Rename(linkdest, newdest);
        else
           eq->BgMove(linkdest, newdest);
        ::Remove(linkname);
        SymLink(linkname, newdest);
        }
     }
  return true;
}


/* Removes the directory with the given Name and everything it contains.
 * Name is a full path name that begins with the name of the video directory.
 * Returns true if the operation was successful.*/
bool MultiVideoDir::Remove(std::string Name) {
  if (debug) std::cout << "Remove(" << Name << ")" << std::endl;
  if (IsFile(Name)) {
     if (debug) std::cout << "IsFile = true" << std::endl;
     return ::Remove(Name);
     }
  else if (IsSymlink(Name)) {
     if (debug) {
        std::cout << "IsSymlink = true; -> Remove(" << LinkDest(Name)
                  << ") && Remove(" << Name << ")" << std::endl;
        }
     return ::Remove(LinkDest(Name)) and ::Remove(Name);
     }
  else if (IsDirectory(Name)) {
     if (debug) std::cout << "IsDirectory = true" << std::endl;
     for(auto s:DirEntries(Name))
        Remove(Name + '/' + s);
     return ::Remove(Name);
     }

  std::cerr << __PRETTY_FUNCTION__ << "cannot remove " << Name << std::endl;
  return false;
}


/* Recursively remove all subdirs w/o videos.
 * IgnoreFiles: nullterminated list of low-prio files; their presence is to be
 * ignored if they are there, on dir removal they are to be removed.
 */
void MultiVideoDir::Cleanup(const char* IgnoreFiles[]) {
  if (debug) {
     std::cout << "Cleanup(IgnoreFiles = '";
     const char** ig = IgnoreFiles;
     while(*ig) {
        if (FileExists(*ig)) {
           std::cout << *ig << std::endl;
           if (*++ig) std::cout << ",";
           }
        else
           ig++;
        }
     std::cout << "')" << std::endl;
     }

  cVideoDirectory::Cleanup(IgnoreFiles);
}


/* returns true, if deleting file 'Name' would release disk space on
 * the video dirs for new recordings. */
bool MultiVideoDir::Contains(std::string Name) {
  if (debug) std::cout << "Contains(" << Name << ")" << std::endl;
  if (IsSymlink(Name)) {
     if (debug) {
        std::cout << "IsSymlink = true; result = ";
        if (LinkDest(Name).find(mountprefix) == 0)
           std::cout << "true" << std::endl;
        else
           std::cout << "false" << std::endl;
        }
     return LinkDest(Name).find(mountprefix) == 0;
     }
  else if (IsFile(Name)) {
     if (debug) {
        std::cout << "IsFile = true; result = ";
        if (Name.find(mountprefix) == 0)
           std::cout << "true" << std::endl;
        else
           std::cout << "false" << std::endl;
        }
     return Name.find(mountprefix) == 0;
     }
  return false;
}

void MultiVideoDir::Balance() {
  eq->Equalize();
}

void MultiVideoDir::Import(std::string Path, bool One, bool DryRun) {
  for(auto e:DirEntries(Path))
     if (IsDirectory(Path + '/' + e)) {
        char c = eq->CharMapping(e);
        if (DryRun) {
           ImportData d = std::make_tuple(videodir, eq->Storage(c), Path, e, true);
           ImportWork(d);
           }
        else
           eq->BgImport(videodir, eq->Storage(c), Path, e);
        if (One) return;
        }

}
