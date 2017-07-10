#include <vector>
#include <iostream>
#include "nabto_client_api.h"
#include "cxxopts.hpp"

void help(cxxopts::Options& options) {
    std::cout << options.help({"", "Group"}) << std::endl;
}

void die(const std::string& msg, int status=1) {
    std::cout << msg << std::endl;
    exit(status);
}

bool createCert(const std::string& commonName, const std::string& password) {
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

int main(int argc, char** argv) {
    try
    {
        cxxopts::Options options(argv[0], " - example command line options");
        options.positional_help("[optional args]");

        bool apple = true;

        options.add_options()
            ("c,create-cert", "Create self signed certificate")
            ("n,cert-name", "Certificate name", cxxopts::value<std::string>())
            ("a,password", "Password for private key", cxxopts::value<std::string>()->default_value("not-so-secret"))
            ("q,rpc-invoke", "URL for RPC query")
            ("d,interface-definition", "Path to unabto_queries.xml file with RPC interface definition")
            ("h,tunnel-host", "Nabto device id for tunnel")
            ("l,tunnel-local-port", "TCP port that local nabto tunnel endpoint listens on", cxxopts::value<uint16_t>())
            ("r,tunnel-remote-port", "TCP port that remote nabto tunnel endpoint connects to", cxxopts::value<uint16_t>())
            ("tunnel-remote-host", "TCP host that remote nabto tunnel endpoint connects to", cxxopts::value<std::string>())
            ("t,tunnel-string", "Compact tunnel specification that can be specified multiple times: <local tcp port>:<remote tcp host>:<remote tcp port>", cxxopts::value<std::vector<std::string>>())
            ("install-resources", "Install necessary resources")
            ("discover", "Show Nabto devices ids discovered on local network")
            ("certs", "Show available certificates")
            ("s,stun", "Show STUN analysis results")
            ("v,version", "Show version")
            ("H,help", "Show help");

        options.parse(argc, argv);

        if (options.count("help")) {
            help(options);
            die(0);
        }

        if(options.count("create-cert")) {
            if (!options.count("cert-name")) {
                die("Missing cert-name parameter");
            }
            if (!createCert(options["cert-name"].as<std::string>(), options["password"].as<std::string>())) {
                die("Create cert failed");
            }
        }
        
        if (apple)
        {
            std::cout << "Saw option ‘a’ " << options.count("a") << " times " <<
                std::endl;
        }

        if (options.count("b"))
        {
            std::cout << "Saw option ‘b’" << std::endl;
        }

        if (options.count("f"))
        {
            auto& ff = options["f"].as<std::vector<std::string>>();
            std::cout << "Files" << std::endl;
            for (const auto& f : ff)
            {
                std::cout << f << std::endl;
            }
        }

        if (options.count("input"))
        {
            std::cout << "Input = " << options["input"].as<std::string>()
                      << std::endl;
        }

        if (options.count("output"))
        {
            std::cout << "Output = " << options["output"].as<std::string>()
                      << std::endl;
        }

        if (options.count("positional"))
        {
            std::cout << "Positional = {";
            auto& v = options["positional"].as<std::vector<std::string>>();
            for (const auto& s : v) {
                std::cout << s << ", ";
            }
            std::cout << "}" << std::endl;
        }

        if (options.count("int"))
        {
            std::cout << "int = " << options["int"].as<int>() << std::endl;
        }

        std::cout << "Arguments remain = " << argc << std::endl;

    } catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    return 0;
}


