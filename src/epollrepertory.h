#ifndef EPOLLREPERTORY_H
#define EPOLLREPERTORY_H

#include <sys/epoll.h>
#include <string.h>
#include <ctime>
#include <iostream>


//#define MAX_EVENTS 512

//struct FdMsg {
//    int type;
//    int fd;
//    int status;
//    //addr_data addr;
//    void (*call_back)(int fd, uint32_t events, void *arg);
//    uint32_t events;
//    void *arg;
//    bool operator < (const FdMsg &b) const {
//        return fd < b.fd;
//    }
//    bool operator == (const FdMsg &b) const{
//        return fd == b.fd;
//    }
//};

//class EpollRepertory : public FdManage {
//public:
//    EpollRepertory();
//    void eventAdd(FdMsg &fdMsg);
//    void eventDel(FdMsg &fdMsg);
//    void eventDel(int fd);
//    void eventMod(int events, int fd);
//    int getEpollFd();
//private:
//    int g_epollFd;
//};

#endif // EPOLLREPERTORY_H
