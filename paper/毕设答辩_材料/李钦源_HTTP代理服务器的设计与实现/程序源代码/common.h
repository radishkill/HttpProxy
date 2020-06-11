#ifndef MSYSTEM_COMMON_H
#define MSYSTEM_COMMON_H

#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/regex.hpp>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

namespace msystem {

namespace ba=boost::asio;
namespace bs=boost::system;

typedef boost::shared_ptr<ba::ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<ba::io_context> io_context_ptr;

}

#endif
