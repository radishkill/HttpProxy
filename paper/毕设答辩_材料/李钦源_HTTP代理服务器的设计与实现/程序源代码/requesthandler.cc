#include "requesthandler.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <list>

#include "filter.h"
#include "proxyserver.h"
#include "conf.h"

namespace msystem {
RequestHandler::RequestHandler(Connection& conn)
    : conn_(conn) {

}


RequestHandler::~RequestHandler() {

}
}

