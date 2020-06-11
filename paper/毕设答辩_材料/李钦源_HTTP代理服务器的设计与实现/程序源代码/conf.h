#ifndef CONF_H
#define CONF_H

#include <vector>
#include <string>

#include <boost/unordered_map.hpp>

#define FILTER_ENABLE
#define REVERSE_SUPPORT

namespace msystem {



/*
 * Hold all the configuration time information.
 */
class ConfigPool {
 public:
  std::vector<int> basicauth_list;
  std::string logf_name;
  std::string config_file;
  std::string upstream;
  uint8_t syslog;    /* boolean */
  uint16_t port;
  char *stathost;
  uint8_t godaemon;  /* boolean */
  uint8_t quit;      /* boolean */
  uint32_t maxthread;
  char *user;
  char *group;
  std::vector<std::string> listen_addrs;
#ifdef FILTER_ENABLE
  std::string filter;
  uint8_t filter_url;        /* boolean */
  uint8_t filter_extended;   /* boolean */
  uint8_t filter_casesensitive;      /* boolean */
#endif                          /* FILTER_ENABLE */
#ifdef XTINYPROXY_ENABLE
  uint32_t add_xtinyproxy; /* boolean */
#endif
#ifdef REVERSE_SUPPORT
  //struct reversepath *reversepath_list;
  boost::unordered_map<std::string, std::string> reversepath_list;
  uint8_t reverseonly;       /* boolean */
  uint8_t reversemagic;      /* boolean */
  std::string reversepath_file;
  std::string reverse_base_url;
#endif
#ifdef UPSTREAM_SUPPORT
  struct upstream *upstream_list;
#endif                          /* UPSTREAM_SUPPORT */
  char *pidpath;
  uint32_t idletimeout;
  std::string bind_address;
  uint8_t bindsame;

  /*
       * The configured name to use in the HTTP "Via" header field.
       */
  char *via_proxy_name;

  uint32_t disable_viaheader; /* boolean */

  /*
       * Error page support.  Map error numbers to file paths.
       */
  //hashmap_t errorpages;

  /*
       * Error page to be displayed if appropriate page cannot be located
       * in the errorpages structure.
       */
  char *errorpage_undef;

  /*
       * The HTML statistics page.
       */
  char *statpage;

  std::vector<int> access_list;

  /*
       * Store the list of port allowed by CONNECT.
       */
  std::vector<int> connect_ports;

  /*
       * Map of headers which should be let through when the
       * anonymous feature is turned on.
       */
  std::vector<int> anonymous_map;

  /*
       * Extra headers to be added to outgoing HTTP requests.
       */
  std::vector<int> add_headers;

  ConfigPool();
  static ConfigPool* GetConfigPool();
  static int SetConfigPool(ConfigPool* config_pool);
  ~ConfigPool();

 private:
  static  ConfigPool* config_pool_;
};
}

#endif // CONF_H
