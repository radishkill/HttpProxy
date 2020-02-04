//#include "epollrepertory.h"

//EpollRepertory::EpollRepertory()
//{
////    g_epollFd = epoll_create1(MAX_EVENTS);
//  g_epollFd = epoll_create1(0);
//}

//void EpollRepertory::eventAdd(FdMsg &fdMsg) {
//    //向管理器里面添加数据
//    FdMsg *p = this->add(fdMsg);
//    struct epoll_event epv = {0, {nullptr}};

//    epv.data.ptr = p;
//    epv.events = fdMsg.events;

//    if (epoll_ctl(g_epollFd, EPOLL_CTL_ADD, fdMsg.fd, &epv) < 0) {
//        std::cout << "add error" << std::endl;
//    }
//}

//void EpollRepertory::eventDel(FdMsg &fdMsg) {
//    this->drop(fdMsg);
//    struct epoll_event epv = {0, {0}};

//    epoll_ctl(g_epollFd, EPOLL_CTL_DEL, fdMsg.fd, &epv);
//}

//void EpollRepertory::eventDel(int fd) {
//    FdMsg fdMsg;
//    fdMsg.fd = fd;
//    this->drop(fdMsg);
//    struct epoll_event epv = {0, {0}};

//    epoll_ctl(g_epollFd, EPOLL_CTL_DEL, fdMsg.fd, &epv);
//}

//void EpollRepertory::eventMod(int events, int fd) {
//    events = events;
//    fd = fd;
//}

//int EpollRepertory::getEpollFd()
//{
//    return g_epollFd;
//}
