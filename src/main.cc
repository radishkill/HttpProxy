#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>

#include "conninfo.h"
#include "conf.h"
#include "connreceiver.h"

namespace po = boost::program_options;

using std::cout;
using std::endl;
using boost::asio::io_context;

int process_cmd(int ac, char **av) {
  msystem::ConfigPool* config_pool = msystem::ConfigPool::GetConfigPool();
  po::options_description general_options("Options are:");
  general_options.add_options()
      ("help,h", "produce help message.")
      ("conf,c", po::value<std::string>(&config_pool->config_file)->default_value("proxy.cfg"), "use an alternate configuration file.")
      ("port,p", po::value<uint16_t>(), "listen port.")
      ("version,v", "display version information.")
  ;

  po::options_description usage("Usage: xxx [options]");

  po::options_description support("For support, please visit\n<https://radishkill.github.io/>");

  po::options_description visible("");
  visible.add(usage).add(general_options).add(support);

  po::options_description config("Configuration");
  config.add_options()
      ("port", po::value<uint16_t>(), "listen port")
      ("listen", po::value<std::string>(), "listen address")
      ;
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, general_options), vm);
  po::notify(vm);

  if (vm.count("help") || vm.empty()) {
    cout << visible << endl;
    return -1;
  }
  if (vm.count("version")) {
    cout << "develop version" << endl;
    return -1;
  }

  std::ifstream ifs(config_pool->config_file.c_str());
  if (!ifs) {
    std::string err = "can not open config file: ";
    err += config_pool->config_file;
    throw  err;
  } else {
    po::store(po::parse_config_file(ifs, config), vm);
    po::notify(vm);
    ifs.close();
  }

  if (vm.count("listen")) {
    config_pool->listen_addrs.push_back(vm["listen"].as<std::string>());
  } else {
    throw "bind_address in config file is necessary";
  }
  if (vm.count("port")) {
    config_pool->port = vm["port"].as<uint16_t>();
  } else {
    throw "port in config file is necessary";
  }
  return 1;
}

int main(int argc, char* argv[]) {
  try {
    msystem::ConfigPool* config_pool = new msystem::ConfigPool();
    msystem::ConfigPool::SetConfigPool(config_pool);

    if (!process_cmd(argc, argv)) {
      exit(0);
    }

    io_context io_ctx;
    io_context io_context_acceptor;
    io_context io_context_handler;

    msystem::ConnReceiver conn_receiver(io_context_acceptor, io_context_handler);


    std::thread accept_thread{
        [&]{
            std::cout << "[tid:" << std::this_thread::get_id() << "] "
                         "Acceptor ctx runs here" << std::endl;
            io_context_acceptor.run();
        } };

    asio::signal_set break_signals{ io_context_handler, SIGINT };
    break_signals.async_wait(
        [&]( boost::system::error_code ec, int ){
            if( !ec )
            {
                std::cout << "Stopping..." << std::endl;
                io_context_acceptor.stop();
                io_context_handler.stop();
            }
        } );
    std::thread handler_thread{
        [&]{
            std::cout << "[tid:" << std::this_thread::get_id() << "] "
                         "Handler ctx runs here" << std::endl;
            io_context_handler.run(); } };

    accept_thread.join();
    handler_thread.join();

  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (std::string& e) {
    std::cerr << "error: " << e << "\n";
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!\n";
  }

  return 0;
}
