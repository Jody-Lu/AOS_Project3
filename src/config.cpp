#include "config.h"

#include "utils.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

void Config::create(void)
{
    std::ifstream ifs(filename.c_str());

    std::string line;
    while (getline(ifs, line)) {
        TcpConfig cfg;
        cfg.number = Utils::str_to_int(line);
        getline(ifs, cfg.host);
        getline(ifs, line);
        cfg.port = Utils::str_to_int(line);
        tcp_configs.push_back(cfg);
    }
}

TcpConfig& Config::getTcpConfig(int number)
{
    return tcp_configs.at(number - 1);
}
