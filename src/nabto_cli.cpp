#include <iostream>
#include <fstream>

#ifndef WIN32
#include <signal.h>
#endif

#include "cxxopts.hpp"
#include "nabto_client_api.h"
#include "tunnel_manager.hpp"

namespace nabtocli {

static std::unique_ptr<TunnelManager> tunnelManager_;

void sigHandler(int signo) {
    if (signo == SIGINT && tunnelManager_) {
        tunnelManager_->stop();
    }
}

void help(cxxopts::Options& options) {
    std::cout << options.help({"", "Group"}) << std::endl;
}

void die(const std::string& msg, int status=1) {
    std::cout << msg << std::endl;
    nabtoShutdown();
    exit(status);
}

bool init(cxxopts::Options& options) {
    nabto_status_t st;
    if (options.count("home-dir")) {
        st = nabtoStartup(options["home-dir"].as<std::string>().c_str());
    } else {
        st = nabtoStartup(NULL);
    }
#ifndef WIN32
    signal(SIGINT, sigHandler);
#endif
    return st == NABTO_OK && nabtoInstallDefaultStaticResources(NULL) == NABTO_OK;
}

////////////////////////////////////////////////////////////////////////////////
// cert

bool certCreate(const std::string& commonName, const std::string& password) {
    nabto_status_t st = nabtoCreateSelfSignedProfile(commonName.c_str(), password.c_str());
    if (st != NABTO_OK) {
        std::cout << "Failed to create self signed certificate " << st << std::endl;
        return false;
    }
    char fingerprint[16];
    st = nabtoGetFingerprint(commonName.c_str(), fingerprint);    
    if (st != NABTO_OK) {
        std::cout << "Failed to get fingerprint of self signed certificate " << st << std::endl;
        return false;
    }
    char fingerprintString[3*sizeof(fingerprint)];
    for (size_t i=0; i<sizeof(fingerprint)-1; i++) {
        sprintf(fingerprintString+3*i, "%02x:", (unsigned char)(fingerprint[i]));
    }
    sprintf(fingerprintString+3*15, "%02x", (unsigned char)(fingerprint[15]));
    std::cout << "Created self signed cert with fingerprint [" << fingerprintString << "]" << std::endl;
    return true;
}

bool certList() {
    char** certificates;
    int certificatesLength;
    nabto_status_t status;
    
    status = nabtoGetCertificates(&certificates, &certificatesLength);
    if (status != NABTO_OK) {
        fprintf(stderr, "nabtoGetCertificates failed: %d (%s)\n", (int) status, nabtoStatusStr(status));
        return false;
    }
    
    for (int i = 0; i < certificatesLength; i++) {
        std::cout << certificates[i] << std::endl;
    }

    for (int i = 0; i < certificatesLength; i++) {
        nabtoFree(certificates[i]);
    }
    nabtoFree(certificates);
    return true;
}

bool certOpenSession(nabto_handle_t& session, cxxopts::Options& options) {
    const std::string& cert = options["cert-name"].as<std::string>();
    const std::string& passwd = options["password"].as<std::string>();
    nabto_status_t status = nabtoOpenSession(&session, cert.c_str(), passwd.c_str());
    if (status == NABTO_OK) {
        if (options.count("basestation-auth-json")) {
            const std::string& json = options["basestation-auth-json"].as<std::string>();
            if (nabtoSetBasestationAuthJson(session, json.c_str()) == NABTO_OK) {
                return true;
            } else {
                std::cout << "Cert opened ok, but could not set basestation auth json doc" << std::endl;
                return false;
            }
        }
        return true;
    } else if (status == NABTO_OPEN_CERT_OR_PK_FAILED) {
        std::cout << "No such certificate " << cert << std::endl;
    } else if (status == NABTO_UNLOCK_PK_FAILED) {
        std::cout << "Invalid password specified for " << cert << std::endl;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// rpc

bool rpcSetInterface(nabto_handle_t session, const std::string& file) {
    std::string content;
    bool ok = false;
    
    std::ifstream ifs(file.c_str(), std::ifstream::in);
    if (ifs.good()) {
        content = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ok = true;
    } else {
        ok = false;
    }
    ifs.close();
    if (!ok) {
        return false;
    }
    char* error;
    nabto_status_t status = nabtoRpcSetDefaultInterface(session, content.c_str(), &error);
    if (status == NABTO_FAILED_WITH_JSON_MESSAGE) {
        std::cout << error << std::endl;
        nabtoFree(error);
    }
    return status == NABTO_OK;
}

bool rpcInvoke(cxxopts::Options& options) {
    nabto_handle_t session;
    if (!certOpenSession(session, options)) {
        return false;
    }
    if (!rpcSetInterface(session, options["interface-definition"].as<std::string>())) {
        return false;
    }
    char* json;
    nabto_status status = nabtoRpcInvoke(session, options["rpc-invoke-url"].as<std::string>().c_str(), &json);
    if (status == NABTO_OK || status == NABTO_FAILED_WITH_JSON_MESSAGE) {
        std::cout << json << std::endl;
        nabtoFree(json);
    } else {
        std::cout << "RPC invocation failed with status " << status << std::endl;
    }
    return status == NABTO_OK;
}

////////////////////////////////////////////////////////////////////////////////
// tunnel

bool tunnelRunFromParams(cxxopts::Options& options) {
    nabto_handle_t session;
    if (!certOpenSession(session, options)) {
        return false;
    }
    tunnelManager_.reset(new TunnelManager(session));

    uint16_t localPort = options["tunnel-local-port"].as<uint16_t>();
    const std::string& deviceId = options["tunnel-host"].as<std::string>();
    uint16_t remotePort = options["tunnel-remote-port"].as<uint16_t>();
    const std::string& remoteHost = options["tunnel-remote-host"].as<std::string>();

    if (tunnelManager_->open(localPort, deviceId, remoteHost, remotePort)) {
        tunnelManager_->watchStatus();
    }
    return tunnelManager_->close();
}

bool tunnelRunFromString(cxxopts::Options& options) {
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// show stuff

bool showLocalDevices() {
    char** devices;
    int devicesLength;
    nabto_status_t status;
    
    status = nabtoGetLocalDevices(&devices, &devicesLength);
    if (status != NABTO_OK) {
        return false;
    }
    
    for(int i = 0; i < devicesLength; i++) {
        std::cout << devices[i] << std::endl;
    }

    for (int i = 0; i < devicesLength; i++) {
        nabtoFree(devices[i]);
    }
    nabtoFree(devices);
    return true;
}

bool showVersion() {
    char* version;
    nabto_status_t status = nabtoVersionString(&version);
    std::cout << version << std::endl;
    nabtoFree(version);
    return status == NABTO_OK;
}

} // namespace

using namespace nabtocli;

////////////////////////////////////////////////////////////////////////////////
// main

int main(int argc, char** argv) {
    try
    {
        cxxopts::Options options(argv[0], "Nabto Command Line demo");
        options.positional_help("[optional args]");

        bool apple = true;

        options.add_options()
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name", cxxopts::value<std::string>())
            ("a,password", "Password for private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("basestation-auth-json", "JSON doc to pass to basestation for authentication", cxxopts::value<std::string>())
            ("q,rpc-invoke-url", "URL for RPC query", cxxopts::value<std::string>())
            ("d,interface-definition", "Path to unabto_queries.xml file with RPC interface definition", cxxopts::value<std::string>())
            ("h,tunnel-host", "Nabto device id for tunnel", cxxopts::value<std::string>())
            ("l,tunnel-local-port", "TCP port that local nabto tunnel endpoint listens on", cxxopts::value<uint16_t>()->default_value("0"))
            ("r,tunnel-remote-port", "TCP port that remote nabto tunnel endpoint connects to", cxxopts::value<uint16_t>())
            ("tunnel-remote-host", "Host on remote network that remote nabto tunnel endpoint connects to", cxxopts::value<std::string>()->default_value("127.0.0.1"))
            ("t,tunnel-string", "Compact tunnel specification that can be specified multiple times: <local tcp port>:<remote tcp host>:<remote tcp port>", cxxopts::value<std::vector<std::string>>())
            ("H,home-dir", "Override default Nabto home directory", cxxopts::value<std::string>())
            ("discover", "Show Nabto devices ids discovered on local network")
            ("certs", "Show available certificates")
            ("v,version", "Show version")
            ("help", "Show help");

        options.parse(argc, argv);

        if (!init(options)) {
            die("Initialization failed");
        }

        ////////////////////////////////////////////////////////////////////////////////
        // show stuff

        if (options.count("discover")) {
            showLocalDevices();
            exit(0);
        }

        if (options.count("version")) {
            showVersion();
            exit(0);
        }

        if (options.count("help")) {
            help(options);
            die("", 0);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // certs 
        
        if(options.count("create-cert")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (certCreate(options["cert-name"].as<std::string>(), options["password"].as<std::string>())) {
                exit(0);
            } else {
                die("Create cert failed");
            }
        }
        
        if(options.count("certs")) {
            if (certList()) {
                exit(0);
            } else {
                die("list certs failed");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // rpc

        if (options.count("rpc-invoke-url")) {
            if (!options.count("interface-definition")) {
                die("Missing RPC interface definition");
            }
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (rpcInvoke(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("RPC Invoke failed");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // tunnel
        
        if (options.count("tunnel-host") && options.count("tunnel-remote-port")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (tunnelRunFromParams(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("Could not start tunnel");
            }
        }

        if (options.count("tunnel-string")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (tunnelRunFromString(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("Could not start tunnel");
            }
        }

        help(options);
        
    } catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }
    

    return 0;
}

