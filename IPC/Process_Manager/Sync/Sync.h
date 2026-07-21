#ifndef _SYNC_H
#define _SYNC_H

#include <semaphore.h>

class Sync 
{
public:

  typedef enum 
  {
    en_TYPE_INTER_PROCESS = 0, 
    en_TYPE_INTER_THREAD,      
  } EN_TYPE;


  typedef enum 
  {
    en_STS_SUCCESS = 0, 
    en_STS_TIMEOUT,     
    en_STS_ERR_OTHER,   
  } EN_STS;

  static constexpr int C_TIME_INFINITE = -1;   
  static constexpr size_t C_MAX_NAME_LEN = 32;
  Sync();
  Sync(const char *name, EN_TYPE type);
  ~Sync();
  EN_STS Lock(int timeout = C_TIME_INFINITE);
  void Unlock(void);
  Sync(const Sync &rhs) = delete;
  Sync &operator=(const Sync &rhs) = delete;

private:
  sem_t *pLock_;                 
  EN_TYPE type_;                  
  char name_[C_MAX_NAME_LEN + 1]; 
  pthread_t firstThread_;         
  int cnt_;                       
};


class SyncAuto 
{
public:

  explicit SyncAuto(Sync *_object) : ksync_(_object) 
  {
    if (0 != ksync_) 
    {
      ksync_->Lock();
    }
  }

  ~SyncAuto() 
  {
    if (0 != ksync_) 
    {
      ksync_->Unlock();
    }
  }

  SyncAuto(const SyncAuto &rhs) = delete;
  SyncAuto &operator=(const SyncAuto &rhs) = delete;

private:
  Sync *ksync_; 
};

#endif 
