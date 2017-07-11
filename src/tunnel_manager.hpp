#pragma once
#include <vector>
#include "nabto_client_api.h"

namespace nabtocli {

class TunnelManager {
private:
    std::vector<nabto_tunnel_t> tunnels_;
    std::map<nabto_tunnel_t, nabto_tunnel_state_t> tunnelStates_;
    nabto_handle_t session_;
    std::atomic<bool> stop_;
    const char* statusStr(nabto_tunnel_state_t status);
public:
    TunnelManager(nabto_handle_t session);
    bool open(uint16_t localPort,
              const std::string& deviceId,
              const std::string& remoteHost,
              uint16_t remotePort);
    bool close();
    bool watchStatus();
    void stop();
};

} // namespace

