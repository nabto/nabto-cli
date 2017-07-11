#include <thread>
#include <map>
#include <iostream>

#include "tunnel_manager.hpp"

namespace nabtocli {

TunnelManager::TunnelManager(nabto_handle_t session)
    : session_(session) {
}

bool TunnelManager::open(uint16_t localPort,
                         const std::string& deviceId,
                         const std::string& remoteHost,
                         uint16_t remotePort) {
    nabto_tunnel_t tunnel;
    nabto_status_t st = nabtoTunnelOpenTcp(&tunnel, session_, localPort, deviceId.c_str(), remoteHost.c_str(), remotePort);
    if (st == NABTO_OK) {
        tunnels_.push_back(tunnel);
        tunnelStates_[tunnel] = NTCS_CLOSED;
        return true;
    } else {
        std::cout << "Could not open tunnel to " << deviceId << ", tunnel open failed with status " << st << std::endl;
        return false;
    }
}
    
bool TunnelManager::watchStatus() {
    while (!stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (auto&& tunnel : tunnels_) {
            nabto_tunnel_state_t newState = NTCS_CLOSED;
            nabto_status_t st = nabtoTunnelInfo(tunnel, NTI_STATUS, sizeof(newState), &newState);
            if (st != NABTO_OK) {
                std::cout << "?" << std::flush;
            } else if (newState == NTCS_CONNECTING) {
                std::cout << "." << std::flush;
            } else if (newState == NTCS_CLOSED && tunnelStates_[tunnel] != NTCS_CLOSED) {
                std::cout << " connection closed." << std::endl;
                int ec;
                st = nabtoTunnelInfo(tunnel, NTI_LAST_ERROR, sizeof(ec), &ec);
                if (st == NABTO_OK) {
                    std::cout << "   last error = " << ec << std::endl;
                }
            }
                
            if (tunnelStates_[tunnel] != newState) {
                if (tunnelStates_[tunnel] == NTCS_CONNECTING) {
                    std::cout << std::endl;
                }
                std::cout << "State has changed for tunnel " << tunnel << " status " << statusStr(newState) << " (" << newState << ")" << std::endl;
                tunnelStates_[tunnel] = newState;
                int version;
                    
                version = -1; 
                nabtoTunnelInfo(tunnel, NTI_VERSION, sizeof(version), &version);
                unsigned short port = -1;
                nabtoTunnelInfo(tunnel, NTI_PORT, sizeof(port), &port);
                std::cout << "Tunnel connected, tunnel version: " << version << ", local TCP port: " << port << std::endl;
            }
        }
    }
    return true;
}

void TunnelManager::stop() {
    stop_ = true;
}

bool TunnelManager::close() {
    for (auto&& tunnel : tunnels_) {
        nabto_status_t st = nabtoTunnelClose(tunnel);
        if (st != NABTO_OK) {
            std::cout << "Tunnel close failed with status " << st << std::endl;
        }
    }
    return true;
}

const char* TunnelManager::statusStr(nabto_tunnel_state_t status) {
    switch(status)
    {
    case NTCS_CLOSED: return "CLOSED";
    case NTCS_CONNECTING: return "CONNECTING";
    case NTCS_READY_FOR_RECONNECT: return "READY_FOR_RECONNECT";
    case NTCS_UNKNOWN: return "Unknown connection";
    case NTCS_LOCAL: return "LOCAL";
    case NTCS_REMOTE_P2P: return "REMOTE_P2P";
    case NTCS_REMOTE_RELAY: return "REMOTE_RELAY (UDP)";
    case NTCS_REMOTE_RELAY_MICRO: return "REMOTE_RELAY (TCP)";
    default: return "?";
    }
};


} // namespace

