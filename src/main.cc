#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include "conf.h"
#include "filter.h"
#include "common.h"
#include "proxyserver.h"

namespace po = boost::program_options;

using std::cout;
using std::endl;
using boost::asio::io_context;

int process_cmd(int ac, char **av) {
  msystem::ConfigPool* config_pool = msystem::ConfigPool::GetConfigPool();
  //控制台选项参数初始化
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

  //配置文件参数初始化
  po::options_description config("Configuration");
  config.add_options()
      ("port", po::value<uint16_t>(), "listen port")
      ("listen", po::value<std::string>(), "listen address")
      ("upstream", po::value<std::string>(&config_pool->upstream), "")
      ("timeout", po::value<uint32_t>()->default_value(60), "")
      ("bindsame", po::bool_switch()->default_value(false), "")
      ("filter", po::value<std::string>(&config_pool->filter), "")
      ("filterURLs", po::bool_switch()->default_value(false), "")
      ("filterextended", po::bool_switch()->default_value(false), "")
      ("filtercasesensitive", po::bool_switch()->default_value(false), "")
      ("maxthread", po::value<uint32_t>(&config_pool->maxthread)->default_value(10), "maxthread")
      ("reversebaseurl", po::value<std::string>(&config_pool->reverse_base_url), "reversebaseurl")
      ("reversepathfile", po::value<std::string>(&config_pool->reversepath_file), "reversepathfile")
      ;
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, general_options), vm);
  po::notify(vm);

  //处理help参数 显示并退出
  if (vm.count("help") || vm.empty()) {
    std::cout << visible << std::endl;
    std::exit(0);
  }
  //处理version参数 显示并退出
  if (vm.count("version")) {
    cout << "develop version" << endl;
    std::exit(0);
  }
  //处理端口与地址参数，并保存
  if (vm.count("listen")) {
    config_pool->listen_addrs.push_back(vm["listen"].as<std::string>());
  }
  if (vm.count("port")) {
    config_pool->port = vm["port"].as<uint16_t>();
  }
  //读取配置文件参数
  std::ifstream ifs(config_pool->config_file.c_str());
  if (!ifs) {
    throw  std::string("can not open config file: ") + config_pool->config_file;
  } else {
    po::store(po::parse_config_file(ifs, config), vm);
    po::notify(vm);
    ifs.close();
  }

  if (config_pool->listen_addrs.size() == 0 && vm.count("listen")) {
    config_pool->listen_addrs.push_back(vm["listen"].as<std::string>());
  } else {
    throw "bind_address in config file is necessary";
  }

  if (config_pool->port == 0 && vm.count("port")) {
    config_pool->port = vm["port"].as<uint16_t>();
  } else {
    throw "port in config file is necessary";
  }
  if (vm.count("timeout")) {
    config_pool->idletimeout = vm["timeout"].as<uint32_t>();
  }
  if (vm.count("bindsame")) {
    config_pool->bindsame = vm["bindsame"].as<bool>();
  }
  if (vm.count("filterURLs")) {
    config_pool->filter_url = vm["filterURLs"].as<bool>();
  }

  if (vm.count("filterextended")) {
    config_pool->filter_extended = vm["filterextended"].as<bool>();
  }

  if (vm.count("filtercasesensitive")) {
    config_pool->filter_casesensitive = vm["filtercasesensitive"].as<bool>();
  }
  return 1;
}
void ParseReverseFile() {
  msystem::ConfigPool* config_pool = msystem::ConfigPool::GetConfigPool();
  if (config_pool->reversepath_file.empty()) {
      return;
  }
  //打开反向代理地址表文件
  std::ifstream ifs(config_pool->reversepath_file);
  if (!ifs.is_open())
    throw std::string("can not open reverse file: ") + config_pool->reversepath_file;
  std::array<char, 8129> buff;
  std::string v1;
  std::string v2;
  uint8_t skip = false;
  uint8_t v2_flag = false;
  while (!ifs.eof()) {
    //读取一定字符
    std::streamsize size = ifs.readsome(buff.data(), buff.max_size());
    if (size == 0) {
      break;
    }
    //对读取的字符进行处理
    for (auto iter = buff.begin(); iter != buff.begin()+size; ++iter) {
      if (v1.empty() && *iter == '#') {
        skip = true;
      } else if (v1.empty() && *iter == ' ') {

      } else if (!v1.empty() && *iter == ' ') {
        v2_flag = true;
      } else if (*iter == '\n') {
        if (!skip)
          config_pool->reversepath_list.insert(std::make_pair(v1, v2));
        v1.clear();
        v2.clear();
        skip = false;
        v2_flag = false;
      } else {
        if (!v2_flag)
          v1.push_back(*iter);
        else
          v2.push_back(*iter);
      }
    }
  }
  //关闭文件
  ifs.close();
}

int main(int argc, char* argv[]) {
  try {
    //配置管理类 初始化
    msystem::ConfigPool* config_pool = new msystem::ConfigPool();
    msystem::ConfigPool::SetConfigPool(config_pool);
    //读取控制台选项和配置文件
    if (!process_cmd(argc, argv)) {
      std::exit(0);
    }
    //读取过滤配置表
    msystem::Filter* filter = new msystem::Filter(config_pool->filter);
    msystem::Filter::SetFilter(filter);
    //读取反向代理配置表
    ParseReverseFile();
    //建立线程池模型
    msystem::ioctx_deque io_contexts;
    //初始化任务队列
    std::deque<msystem::ba::io_service::work> io_context_work;
    boost::thread_group thr_grp;
    for (int i = 0; i < 10; ++i) {
          msystem::io_context_ptr ios(new msystem::ba::io_context);
          io_contexts.push_back(ios);
          io_context_work.push_back(msystem::ba::io_context::work(*ios));
          //申请线程并绑定任务处理的io_context
          thr_grp.create_thread(boost::bind(&msystem::ba::io_context::run, ios));
    }
    //启动HTTP代理服务器类
    msystem::ProxyServer proxy_server(io_contexts);
    //开始监听客户端请求
    proxy_server.StartAccept();
    thr_grp.join_all();
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
