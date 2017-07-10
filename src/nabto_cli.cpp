#include <vector>
#include <iostream>
#include <fstream>
#include "nabto_client_api.h"
#include "cxxopts.hpp"

void help(cxxopts::Options& options) {
    std::cout << options.help({"", "Group"}) << std::endl;
}

void die(const std::string& msg, int status=1) {
    std::cout << msg << std::endl;
    nabtoShutdown();
    exit(status);
}

////////////////////////////////////////////////////////////////////////////////
// cert

bool certCreate(const std::string& commonName, const std::string& password) {
    nabto_status_t res = nabtoCreateSelfSignedProfile(commonName.c_str(), password.c_str());
    if (res != NABTO_OK) {
        std::cout << "Failed to create self signed certificate " << res << std::endl;
        return false;
    }
    char fingerprint[16];
    res = nabtoGetFingerprint(commonName.c_str(), fingerprint);    
    if (res != NABTO_OK) {
        std::cout << "Failed to get fingerprint of self signed certificate " << res << std::endl;
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

bool certOpen(cxxopts::Options& options, nabto_handle_t& session) {
    std::string cert = options["cert-name"].as<std::string>();
    nabto_status_t status = nabtoOpenSession(&session, cert.c_str(), options["password"].as<std::string>().c_str());
    if (status == NABTO_OK) {
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
    if (!certOpen(options, session)) {
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
// main

int main(int argc, char** argv) {
    try
    {
        cxxopts::Options options(argv[0], "Nabto Command Line demo");
        options.positional_help("[optional args]");

        bool apple = true;

        options.add_options()
            ("H,home-dir", "Override default Nabto home directory", cxxopts::value<std::string>())
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name", cxxopts::value<std::string>())
            ("a,password", "Password for private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("basestation-auth-json", "JSON doc to path to basestation for authentication", cxxopts::value<std::string>())
            ("q,rpc-invoke-url", "URL for RPC query", cxxopts::value<std::string>())
            ("d,interface-definition", "Path to unabto_queries.xml file with RPC interface definition", cxxopts::value<std::string>())
            ("h,tunnel-host", "Nabto device id for tunnel", cxxopts::value<std::string>())
            ("l,tunnel-local-port", "TCP port that local nabto tunnel endpoint listens on", cxxopts::value<uint16_t>())
            ("r,tunnel-remote-port", "TCP port that remote nabto tunnel endpoint connects to", cxxopts::value<uint16_t>())
            ("tunnel-remote-host", "TCP host that remote nabto tunnel endpoint connects to", cxxopts::value<std::string>())
            ("t,tunnel-string", "Compact tunnel specification that can be specified multiple times: <local tcp port>:<remote tcp host>:<remote tcp port>", cxxopts::value<std::vector<std::string>>())
            ("install-resources", "Install necessary resources")
            ("discover", "Show Nabto devices ids discovered on local network")
            ("certs", "Show available certificates")
            ("v,version", "Show version")
            ("help", "Show help");

        options.parse(argc, argv);

        if (options.count("help")) {
            help(options);
            die("", 0);
        }

        nabto_status_t st;
        if (options.count("home-dir")) {
            st = nabtoStartup(options["home-dir"].as<std::string>().c_str());
        } else {
            st = nabtoStartup(NULL);
        }
        if (st != NABTO_OK) {
            die("Nabto startup failed");
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

        help(options);
        
    } catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }


    return 0;
}


