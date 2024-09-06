/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#pragma once
#include <string>
#include <vector>
#include <sstream>

bool IsDirectory(std::string Name);
bool IsSymlink(std::string Name);
bool IsFile(std::string Name);
bool IsVideoFile(std::string Name);
bool DirectoryExists(std::string Name);
bool EndsWith(std::string Name, std::string s);

bool SymLink(std::string LinkName, std::string LinkDest, bool DryRun = false);
std::string LinkDest(std::string Name);
std::string FlatPath(std::string Path);

std::vector<std::string> DirEntries(std::string Name);

void CopyFile(std::string From, std::string To, bool DryRun = false);
void MoveFile(std::string From, std::string To, bool DryRun = false);
bool Remove(std::string Filename, bool DryRun = false);
size_t FileSize(std::string Name);
bool MakeDirectory(std::string Name, bool Parents = true, bool DryRun = false);
bool Rename(std::string From, std::string To);
