//#include "epollserver.h"

//EpollServer::EpollServer(EpollRepertory *_epollRep)
//{
//    this->epollRep = _epollRep;
//}
//void EpollServer::linstenFd(int __maxEvents, int __timeout) {
//    struct epoll_event events[__maxEvents];
//    int nfds = epoll_wait(epollRep->getEpollFd(), events, __maxEvents, __timeout);//time ms 时间无穷
//    if (nfds > 0) {
//      for (int i = 0; i < nfds; i++) {
//        FdMsg *fm = static_cast<FdMsg *>(events[i].data.ptr);
//        fm->call_back(fm->fd, events[i].events, fm);
//      }
//    } else if (nfds == 0) {
//      cout << __FILE__ << "time  out" << endl;
//    } else {
//      switch (errno) {
//        case EBADF: {
//          std::cout << __FILE__ << "epfd is not a valid file descriptor." << std::endl;
//          break;
//        }
//        case EFAULT: {
//          std::cout << __FILE__ << "The memory area pointed to by events is not accessible with write permissions." << std::endl;
//          break;
//        }
//        case EINTR: {
//          std::cout << __FILE__ << "The call was interrupted by a signal." << std::endl;
//          break;
//        }
//        case EINVAL: {
//          std::cout << __FILE__ << "epfd is not an epoll file descriptor." << std::endl;
//          break;
//        }
//        default: {
//          std::cout << __FILE__ << "epoll unkonw error." << std::endl;
//        }
//      }
//    }
//}
