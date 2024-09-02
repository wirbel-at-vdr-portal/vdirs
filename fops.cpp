/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>     /* memset() */
#include <cstdio>      /* remove() */
#include <sys/types.h> /* stat() */
#include <sys/stat.h>  /* stat() */
#include <unistd.h>    /* stat() */
#include <dirent.h>    /* opendir() */
#include "fops.h"

std::vector<std::string> split(const std::string& s, char d) {
   std::vector<std::string> e;
   delim_split(s, d, std::back_inserter(e));
   return e;
}

bool IsDirectory(std::string Name) {
  struct stat st;
  if (stat(Name.c_str(), &st))
     return false;
  return S_ISDIR(st.st_mode) == 1;
}

// 20240901: OK
bool IsSymlink(std::string Name) {
  struct stat st;
  if (lstat(Name.c_str(), &st))
     return false;
  return S_ISLNK(st.st_mode) == 1;
}

// 20240901: OK
bool IsFile(std::string Name) {
  struct stat st;
  if (lstat(Name.c_str(), &st))
     return false;
  return S_ISREG(st.st_mode) == 1;
}

bool EndsWith(std::string Name, std::string s) {
  if (Name.size() < s.size()) return false;
  return std::equal(s.rbegin(), s.rend(), Name.rbegin());
}

// ok. 20180128
bool IsVideoFile(std::string Name) {
  if (EndsWith(Name, ".ts"))
     return true;
  else if (EndsWith(Name, ".vdr")) {
     if      (EndsWith(Name, "index.vdr"))   return false; 
     else if (EndsWith(Name, "info.vdr"))    return false;
     else if (EndsWith(Name, "resume.vdr"))  return false;
     else if (EndsWith(Name, "marks.vdr"))   return false;
     else if (EndsWith(Name, "summary.vdr")) return false;
     return true;
     }
  return false;
}

std::string LinkDest(std::string Name) {
  static char linkdest[1024];
  /* readlink() does not append a terminating null byte to buf.
   * It will (silently) truncate the contents (to a length of
   * bufsiz characters), in case the buffer is too small to
   * hold all of the contents.
   */
  memset(linkdest, 0, sizeof(linkdest));
  if (readlink(Name.c_str(), linkdest, 1024 - 1) < 0) *linkdest = 0;
  return linkdest;
}



/*// this one is BUGGY. dont use until fix! 
void ReplaceAll(std::string& s, std::string from, std::string to) {
  size_t pos = 0, flen = from.size(), tlen = to.size();
  while((pos = s.find(from, pos)) != std::string::npos) {
     s.replace(pos, flen, to);
     pos += tlen;
     }
}*/

// ok. 20180128
std::string FlatPath(std::string Path) {
  std::string s(Path);
  size_t p = s.rfind(".rec/");

  if (p == std::string::npos)
     p = s.rfind(".del/");

  if (p != std::string::npos)
     s.erase(p, 4);

  std::replace(s.begin(), s.end(), '/', '~');

  // replace forbidden and/or ugly chars
  auto end = EndsWith(s,".ts")? s.end()-3:s.end()-4;
  std::replace(s.begin(),  end, '.', '_');

  std::replace(s.begin(), s.end(), '?', '_');
  std::replace(s.begin(), s.end(), '\'', '_');
  std::replace(s.begin(), s.end(), '*', '_');
  std::replace(s.begin(), s.end(), ':', '_');
  std::replace(s.begin(), s.end(), ';', '_');
  std::replace(s.begin(), s.end(), ',', '_');
  std::replace(s.begin(), s.end(), '<', '_');
  std::replace(s.begin(), s.end(), '>', '_');
  std::replace(s.begin(), s.end(), '!', '_');
  std::replace(s.begin(), s.end(), '?', '_');
  std::replace(s.begin(), s.end(), '\\','_');
  std::replace(s.begin(), s.end(), '|', '_');

  return s;
}

// ok. 20180126
std::vector<std::string> DirEntries(std::string Name) {
  std::vector<std::string> v;
  DIR* dir = opendir(Name.c_str());

  if (! dir)
     std::cerr << __PRETTY_FUNCTION__ << "cannot open " << Name << std::endl;
  else {
     for(struct dirent* dp = readdir(dir); dp; dp = readdir(dir)) {
        if (!strcmp(dp->d_name, ".") or
            !strcmp(dp->d_name, ".."))
          continue;
        v.push_back(dp->d_name);
        }
     closedir(dir);
     }
  return v;  
}

size_t FileSize(std::string Name) {
  struct stat st;
  if (stat(Name.c_str(), &st))
     return false;
  return st.st_size;
}

// ok. 20180128
bool MakeDirectory(std::string Name, bool Parents, bool DryRun) {
  if (DirectoryExists(Name))
     return true;

  if (!Parents) {
     if (DryRun) {
        std::cerr << __FUNCTION__ << "(" << Name << ",false)" << std::endl;
        return true;
        }
     else
        return mkdir(Name.c_str(),755) == 0;
     }

  std::string fullpath;

  for(auto dir:split(Name.substr(1), '/')) {
     fullpath += '/' + dir;
     if (!MakeDirectory(fullpath, false, DryRun))
        return false;
     }
  return true;
}

bool SymLink(std::string LinkName, std::string LinkDest, bool DryRun) {
  if (DryRun) {
     std::cerr << __FUNCTION__ << ": " << LinkName << " -> " << LinkDest << std::endl;
     return true;
     }
  else
     return symlink(LinkDest.c_str(), LinkName.c_str()) == 0;
}

bool FileExists(std::string Name) {
  return IsFile(Name);
}

bool DirectoryExists(std::string Name) {
  return IsDirectory(Name);
}

void CopyFile(std::string From, std::string To, bool DryRun) {
  if (DryRun)
     std::cerr << __FUNCTION__ << "(" << From << "," << To << ")" << std::endl;
  else {
     std::ifstream src(From.c_str(), std::ios::binary);
     std::ofstream dst(To.c_str()  , std::ios::binary);
     dst << src.rdbuf();
     }
}

void MoveFile(std::string From, std::string To, bool DryRun) {
  if (DryRun)
     std::cerr << __FUNCTION__ << "(" << From << "," << To << ")" << std::endl;
  else {
     CopyFile(From, To);
     if (FileSize(To) == FileSize(From))
        ::Remove(From);
     }
}

bool Remove(std::string Filename, bool DryRun) {
  if (DryRun){
     std::cerr << __FUNCTION__ << "(" << Filename << ")" << std::endl;
     return true;
     }
  else
     return remove(Filename.c_str()) == 0;
}

bool Rename(std::string From, std::string To) {
  return rename(From.c_str(), To.c_str()) == 0;
}
