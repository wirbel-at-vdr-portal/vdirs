/* vdirs - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 */
#pragma once
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <tuple>
#include <condition_variable>
#include <stdexcept>
#include "fops.h"

typedef std::tuple<std::string, std::string, bool> CopyData;
typedef std::tuple<std::string, std::string, std::string, std::string, bool> ImportData;

void CopyWork(CopyData& d) {
  if (std::get<2>(d))
     MoveFile(std::get<0>(d), std::get<1>(d));
  else
     CopyFile(std::get<0>(d), std::get<1>(d));
}

void ImportWork(ImportData& d) {
  std::string videodir = std::get<0>(d);
  std::string Disk     = std::get<1>(d);
  std::string TopSrc   = std::get<2>(d);
  std::string Dir      = std::get<3>(d);
  bool DryRun          = std::get<4>(d);

  std::string Dest(videodir + '/' + Dir);
  std::string Src(TopSrc    + '/' + Dir);

  if (!DirectoryExists(Dest))
     MakeDirectory(Dest, true, DryRun);

  for(auto e:DirEntries(Src)) {
     std::string from(Src + '/' + e);
     std::string to(Dest  + '/' + e);
     if (IsFile(from)) {
        if (IsVideoFile(from)) {
           std::string linkdest = Disk + '/' + FlatPath(Dir + '/' + e);
           std::cerr << "SymLink(" << to << " -> " << linkdest << ")" << std::endl;           
           SymLink(to, linkdest, DryRun);
           std::cerr << "MoveFile(" << from << ", " << linkdest << ")" << std::endl;
           if (!DryRun)
              MoveFile(from, linkdest);
           }
        else {
           std::cerr << "MoveFile(" << from << ", " << to << ")" << std::endl;
           if (!DryRun)
              MoveFile(from, to);
           }
        }
     else if (IsDirectory(from)) {
        ImportData d = std::make_tuple(videodir, Disk, TopSrc, Dir + '/' + e, DryRun);
        ImportWork(d);
        }
     }
  std::cerr << "Remove(" << Src << ")" << std::endl;  
  ::Remove(Src, DryRun);
  std::cerr << "--done.--" << std::endl;
}

/*******************************************************************************
 * // constructor
 * WorkQueue<item_type> q(
 *    [](item_type& item) { detached_nonsequence_work(item); }   );
 *
 * // push job
 * q.Push(std::move(item));
 ******************************************************************************/

template<typename T, typename F, typename Q = std::queue<T>>
class WorkQueue: Q, std::mutex, std::condition_variable {
private:
  size_t capacity;
  bool destroying;
  std::vector<std::thread> Threads;

  void RunTask(F task) {
    std::unique_lock<std::mutex> UniqueLock(*this);
    while(true) {
       if (not Q::empty()) {
          T item { std::move(Q::front()) };
          Q::pop();
          notify_one();
          UniqueLock.unlock();
          task(item);
          UniqueLock.lock();
          }
       else
         if (destroying) break;
         else wait(UniqueLock);
       }
    }

public:
  WorkQueue(F task, size_t Capacity = 0) : capacity(Capacity), destroying(false) {
    if (capacity == 0)
       capacity = std::thread::hardware_concurrency();

    for(size_t i = 0; i < capacity; i++)
       Threads.emplace_back(static_cast<void (WorkQueue::*)(F)>(&WorkQueue::RunTask), this, task);
    }
  WorkQueue(WorkQueue&&) = default;
  WorkQueue& operator=(WorkQueue&&) = delete;
  ~WorkQueue() {
      {
        std::lock_guard<std::mutex> LockGuard(*this);
        destroying = true;
        notify_all();
      }
    for(auto&& t:Threads) t.join();
    }
  void /*operator()*/Push(T&& value) {
    std::unique_lock<std::mutex> UniqueLock(*this);
    while(Q::size() == capacity) wait(UniqueLock);
    Q::push(std::forward<T>(value));
    notify_one();
    }
};
