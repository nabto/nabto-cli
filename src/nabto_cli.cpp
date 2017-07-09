#include <iostream>
#include "nabto_client_api.h"

int main(int argc, char** argv) {
    nabto_status_t status = nabtoStartup(NULL);
    std::cout << "Status is " << status << std::endl;
    return 0;
}
