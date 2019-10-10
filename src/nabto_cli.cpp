/*
 * Copyright (C) 2017 Nabto - All Rights Reserved.
 */

#include "tunnel_manager.hpp"
#include "nabto_client_api.h"
#include "cxxopts.hpp"
#include <json/json.h>
#include "json_helper.hpp"

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

#ifndef WIN32
#include <signal.h>
#endif


namespace nabtocli {

static std::unique_ptr<TunnelManager> tunnelManager_;

void sigHandler(int signo) {
#ifndef WIN32
    if (signo == SIGINT && tunnelManager_) {
        tunnelManager_->stop();
    }
    nabtoShutdown();
    exit(0);
#endif
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

unsigned char parseHex(char c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    die("Invalid hex character in input");
    return false;
}

bool parseHexString(std::vector<char>& parsed, const std::string& text, int offset) {
    for (std::size_t i = 0; i < (text.size() + offset-2) / offset; i++) {
        char l = 16 * parseHex(text[offset * i]) + parseHex(text[offset * i + 1]);
        parsed.push_back(l);
    }
    return true;
}

bool pskParseHex(std::vector<char>& parsed, const std::string& text, int length) {
    int offset;
    if (text.size() == length * 2) {
        offset = 2;
    } else if (text.size() == length * 3 - 1) {
        offset = 3;
    } else {
        std::cout << "hex input should be " << length << " hex characters" << std::endl;
        return false;
    }
    return parseHexString(parsed, text, offset);
}

bool pskSetKeyIfPresent(nabto_handle_t session, const std::string& host, cxxopts::Options& options) {
    if (!(options.count("local-connection-psk-id") && options.count("local-connection-psk"))) {
        return true;
    }
    std::string pskId = options["local-connection-psk-id"].as<std::string>();
    std::string psk = options["local-connection-psk"].as<std::string>();

    std::vector<char> keyIdBytes;
    if (!pskParseHex(keyIdBytes, pskId, 16)) {
        return false;
    }
    std::vector<char> keyBytes;
    if (!pskParseHex(keyBytes, psk, 16)) {
        return false;
    }
    return nabtoSetLocalConnectionPsk(session, host.c_str(), keyIdBytes.data(), keyBytes.data()) == NABTO_OK;
}

bool getFingerprintString(const std::string& commonName, std::string& fingerprintString) {
    char fingerprint[16];
    nabto_status_t st = nabtoGetFingerprint(commonName.c_str(), fingerprint);
    if (st != NABTO_OK) {
        return false;
    }
    char buf[3*sizeof(fingerprint)];
    for (size_t i=0; i<sizeof(fingerprint)-1; i++) {
        sprintf(buf+3*i, "%02x:", (unsigned char)(fingerprint[i]));
    }
    sprintf(buf+3*15, "%02x", (unsigned char)(fingerprint[15]));
    fingerprintString = std::string(buf);
    return true;

}

bool certCreate(const std::string& commonName, const std::string& password) {
    if ( password.compare("not-so-secret") == 0 ){
        std::cout << "Warning: creating certificate with default password for user: " << commonName << std::endl;
    }
    nabto_status_t st = nabtoCreateSelfSignedProfile(commonName.c_str(), password.c_str());
    if (st != NABTO_OK) {
        std::cout << "Failed to create self signed certificate " << st << std::endl;
        return false;
    }
    std::string fingerprint;
    if (getFingerprintString(commonName, fingerprint)) {
        std::cout << "Created self signed cert with fingerprint [" << fingerprint << "]" << std::endl;
        return true;
    } else {
        std::cout << "Failed to get fingerprint of self signed certificate " << st << std::endl;
        return false;
    }
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
        std::string fingerprint;
        if (getFingerprintString(certificates[i], fingerprint)) {
            std::cout << fingerprint << " " << certificates[i] << std::endl;
        }
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
        if (options.count("bs-auth-json")) {
            const std::string& json = options["bs-auth-json"].as<std::string>();
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
        std::cout << "Failed to open queries file: " << file << std::endl;
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

bool checkInterface(nabto_handle_t session, std::string device, cxxopts::Options& options) {
    if (!options.count("interface-id") || !options.count("interface-version")) {
        std::cout << "ERROR: strict-interface-check was given, but interface-id or interface-version was missing" << std::endl;
        return false;
    } else if (options["interface-version"].as<std::string>().find(".") == std::string::npos) {
        std::cout << "ERROR: Badly formatted version string: " << options["interface-version"].as<std::string>() << std::endl;
        return false;
    }

    std::string localVersion = options["interface-version"].as<std::string>();
    int localMajor, localMinor;
    try {
        localMajor = std::stoi(localVersion);
        localMinor = std::stoi(std::string(&localVersion[localVersion.find(".")+1]));
    } catch (std::invalid_argument) {
        std::cout << "ERROR: invalid version provided: " << options["interface-version"].as<std::string>() << std::endl;
        return false;
    }
    if (localMajor < 1 || localMinor < 0) {
        std::cout << "ERROR: invalid version provided: " << options["interface-version"].as<std::string>() << std::endl;
        return false;
    }

    std::string interfaceString("nabto://");
    interfaceString.append(device);
    interfaceString.append("/get_interface_info.json");
    char* json;

    nabto_status status = nabtoRpcInvoke(session, interfaceString.c_str(), &json);
    if (status == NABTO_OK) {
        Json::Value jsonDoc;
        nabto::JsonHelper::parse(std::string(json),jsonDoc);
        nabtoFree(json);
        int major = jsonDoc["response"]["interface_version_major"].asInt();
        int minor = jsonDoc["response"]["interface_version_minor"].asInt();
        std::string interfaceId = options["interface-id"].as<std::string>();

        if (interfaceId.compare(jsonDoc["response"]["interface_id"].asString()) != 0) {
            std::cout << "ERROR: Interface ID mismatch: " << interfaceId << " != " << jsonDoc["response"]["interface_id"] << std::endl;
            return false;
        } else if (major != localMajor || minor < localMinor) {
            std::cout << "ERROR: interface version mismatch between: " << major << "." << minor << " and " << localMajor << "." << localMinor << std::endl;
            return false;
        } else {
            return true;
        }
    } else if(status == NABTO_FAILED_WITH_JSON_MESSAGE) {
        std::cout << json << std::endl;
        nabtoFree(json);
    } else {
        std::cout << "RPC invocation failed with status " << status << std::endl;
        nabtoFree(json);
        return false;
    }
    return true;
}

bool extractHostFromUrl(const std::string& url, std::string& host) {
    std::string prefix = "nabto://";
    size_t hostStart = prefix.length();
    size_t slash = url.find("/", hostStart);
    if (slash != std::string::npos) {
        host = std::string(&url[hostStart],slash-hostStart);
        return true;
    } else {
        return false;
    }
}

bool rpcInvoke(cxxopts::Options& options) {
    nabto_handle_t session;
    if (!certOpenSession(session, options)) {
        return false;
    }
    if (!rpcSetInterface(session, options["interface-def"].as<std::string>())) {
        return false;
    }
    std::string host;
    if (!extractHostFromUrl(options["rpc-invoke-url"].as<std::string>(), host)) {
        std::cout << "ERROR: bad url" << std::endl;
        return false;
    }
    if (options.count("strict-interface-check") && !checkInterface(session, host, options)) {
        std::cout << "ERROR: strict interface check failed" << std::endl;
        return false;
    }

    if (!pskSetKeyIfPresent(session, host, options)) {
        die("Could not set PSK");
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

bool rpcPair(cxxopts::Options& options) {
    char** devices;
    int devicesLength;
    nabto_status_t status;
    nabto_handle_t session;
    char input;
    int deviceChoice = -1;
    std::string url = "nabto://";

    status = nabtoGetLocalDevices(&devices, &devicesLength);
    if (status != NABTO_OK) {
        std::cout << "Failed to discover local devices" << std::endl;
        return false;
    }

    while (deviceChoice < 0 || deviceChoice >= devicesLength){
        std::cout << "Choose a device for pairing: " << std::endl;
        std::cout << "[q]: Quit without pairing" << std::endl;
        for(int i = 0; i < devicesLength; i++) {
            std::cout << "["<< i << "]: " << devices[i] << std::endl;
        }
        std::cin >> input;
        if (input == 'q') {
            std::cout << "Quitting" << std::endl;
            for (int i = 0; i < devicesLength; i++) {
                nabtoFree(devices[i]);
            }
            nabtoFree(devices);
            return false;
        }
        deviceChoice = input - '0';
    }

    if (!certOpenSession(session, options)) {
        std::cout << "Failed to open session" << std::endl;
        for (int i = 0; i < devicesLength; i++) {
            nabtoFree(devices[i]);
        }
        nabtoFree(devices);
        return false;
    }
    if (!rpcSetInterface(session, options["interface-def"].as<std::string>())) {
        for (int i = 0; i < devicesLength; i++) {
            nabtoFree(devices[i]);
        }
        nabtoFree(devices);
        return false;
    }
    if(options.count("strict-interface-check")) {
        if (!checkInterface(session, std::string(devices[deviceChoice]), options)) {
            std::cout << "ERROR: strict interface check failed" << std::endl;
            return false;
        }
    }
    char* json;
    url.append(devices[deviceChoice]);
    url.append("/pair_with_device.json?name=");
    url.append(options["cert-name"].as<std::string>());
    status = nabtoRpcInvoke(session, url.c_str() , &json);
    if (status == NABTO_OK || status == NABTO_FAILED_WITH_JSON_MESSAGE) {
        std::cout << json << std::endl;
        nabtoFree(json);
    } else {
        std::cout << "RPC invocation failed with status " << status << std::endl;
    }

    for (int i = 0; i < devicesLength; i++) {
        nabtoFree(devices[i]);
    }
    nabtoFree(devices);
    return true;

}

////////////////////////////////////////////////////////////////////////////////
// tunnel

bool tunnelRunFromString(cxxopts::Options& options) {
    nabto_handle_t session;

    if (!certOpenSession(session, options)) {
        return false;
    }

    if (!pskSetKeyIfPresent(session, options["tunnel-device"].as<std::string>(), options)) {
        die("Could not set PSK");
    }

    tunnelManager_.reset(new TunnelManager(session));

    for (auto tunnelStr : options["tunnel"].as<std::vector<std::string> >()) {
        int localPort, remotePort;
        std::string remoteHost;
        size_t first = tunnelStr.find(':');
        if(first == std::string::npos) {
            std::cout << "Error: invalid tunnel string: " << tunnelStr << std::endl;
            return false;;
        } else if (first == 0 ) {
            localPort = 0;
        } else {
            localPort = std::stoi(std::string(tunnelStr,0,first));
        }

        size_t second = tunnelStr.find(':',first+1);
        if(second == std::string::npos) {
            std::cout << "Error: invalid tunnel string: " << tunnelStr << std::endl;
            return false;
        } else {
            remoteHost = std::string(tunnelStr,first+1,second-first-1);
            remotePort = std::stoi(std::string(tunnelStr,second+1));
        }
        if (!tunnelManager_->open(localPort, options["tunnel-device"].as<std::string>(), remoteHost, remotePort)) {
            std::cout << "Failed to open tunnel: " << tunnelStr << std::endl;
            return false;
        }
    }
    tunnelManager_->watchStatus();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// stream

std::mutex iomutex_;

bool streamReadFunc(nabto_handle_t session, cxxopts::Options& options) {
    nabto_stream_t stream;
    char* response;
    size_t actual = 0; /* actual size (in bytes) of response */
    nabto_status_t status;

    if (!certOpenSession(session, options)) {
        return false;
    }
    const char* host(options["tunnel-device"].as<std::string>().c_str());
    status = nabtoStreamOpen(&stream, session, host);
    if (status == NABTO_OK) {
        std::lock_guard<std::mutex> lock(iomutex_);
        std::cout << "nabtoStreamOpen() succeeded, stream = " << stream <<  std::endl;

    } else {
        std::lock_guard<std::mutex> lock(iomutex_);
        std::cout << "nabtoStreamOpen() failed with status " << status << ": " << nabtoStatusStr(status) << std::endl;
        nabtoCloseSession(session);
        return false;
    }

    while (true) {
        status = nabtoStreamRead(stream, &response, &actual);
        std::cout << "got " << actual << " bytes with status " << status << std::endl;
        if (status == NABTO_OK) {
            std::string responseStr(response, actual);
            std::cout << responseStr << std::endl;
        } else {
            break;
        }
    }

    std::lock_guard<std::mutex> lock(iomutex_);
    if (status == NABTO_STREAM_CLOSED) {
        std::cout << "Stream " << stream << " closed cleanly" << std::endl;
        return true;
    } else {
        std::cout << "Stream read failed with status " << nabtoStatusStr(status) << std::endl;
        return false;
    }

}

bool streamRead(cxxopts::Options& options) {
    nabto_handle_t session;
    return streamReadFunc(session, options);
#if 0
    for (int i=0; i<1; i++) {
        std::thread t([&]() {
                streamReadFunc(session, options);
            });
        t.detach();
    }
    return true;
#endif
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

        options.add_options()
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name. ex.: nabto-user", cxxopts::value<std::string>())
            ("a,password", "Password for encrypting private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("bs-auth-json", "JSON doc to pass to basestation for authentication. ex.: {\"key\": \"secretKey\"}", cxxopts::value<std::string>())
            ("local-connection-psk-id", "16 byte hex encoded PSK id to use for PSK on local psk connection (32 hex xhars)", cxxopts::value<std::string>())
            ("local-connection-psk", "16 byte hex encoded PSK to use for local psk connection (32 hex chars)", cxxopts::value<std::string>())
            ("q,rpc-invoke-url", "URL for RPC query. ex.: nabto://device.nabto.com/get_public_device_info.json?", cxxopts::value<std::string>())
            ("i,interface-def", "Path to unabto_queries.xml file with RPC interface definition. ex.: /path/to/unabto_queries.xml", cxxopts::value<std::string>())
            ("strict-interface-check", "Use strict interface check for all RPC calls")
            ("interface-id", "interface ID to match for strict interface check. ex.: 317aadf2-3137-474b-8ddb-fea437c424f4", cxxopts::value<std::string>())
            ("interface-version", "<major>.<minor> version number to match for strict interface check. ex.: 1.0", cxxopts::value<std::string>())
            ("d,tunnel-device", "Nabto device ID for tunnel (and more), e.g. device.nabto.com", cxxopts::value<std::string>())
            ("t,tunnel", "Tunnel specification, can be repeated to open multiple tunnel. Format: <local tcp port>:<remote tcp host>:<remote tcp port>", cxxopts::value<std::vector<std::string>>())
            ("stream-read", "Open stream to device specified with -d, read and dump all received data")
            ("H,home-dir", "Override default Nabto home directory. ex.: /path/to/dir", cxxopts::value<std::string>())
            ("pair", "pair user to a local device")
            ("discover", "Show Nabto devices ids discovered on local network")
            ("certs", "Show available certificates")
            ("v,version", "Show version")
            ("h,help", "Show help");

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

        if (options.count("local-connection-psk-id") && !options.count("local-connection-psk")) {
            die("missing local-connection-psk option");
        }

        if (options.count("local-connection-psk") && !options.count("local-connection-psk-id")) {
            die("missing local-connection-psk-id option");
        }

        ////////////////////////////////////////////////////////////////////////////////
        // rpc

        if (options.count("rpc-invoke-url")) {
            if (!options.count("interface-def")) {
                die("Missing RPC interface definition");
            }
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (rpcInvoke(options)) {
                if (!options.count("tunnel")) {
                    nabtoShutdown();
                    exit(0);
                } else {
                    // next, start any tunnels
                }
            } else {
                die("RPC Invoke failed");
            }
        }

        if (options.count("pair")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (!options.count("interface-def")) {
                die("Missing RPC interface definition");
            }
            if (rpcPair(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("Pairing failed");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // tunnel

        if (options.count("tunnel")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (!options.count("tunnel-device")) {
                die("Missing tunnel-device parameter");
            }
            if (tunnelRunFromString(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("Could not start tunnel");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // stream

        if (options.count("stream-read")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (!options.count("tunnel-device")) {
                die("Missing tunnel-device parameter");
            }
            if (streamRead(options)) {
                nabtoShutdown();
                exit(0);
            } else {
                die("Could not start stream read");
            }
        }


        help(options);

    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "Error parsing options: " << e.what() << std::endl;
        exit(1);
    }
    return 0;
}
