#include "conninfo.h"


namespace msystem {

ConnInfo::ConnInfo(boost::asio::io_context& io_ctx)
    : client_socket(io_ctx),
      server_socket(io_ctx),
      connect_method(false),
      show_stats(false),
      state_(kConnInitial) {
}

}
