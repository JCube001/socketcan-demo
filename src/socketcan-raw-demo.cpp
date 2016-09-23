/*
The MIT License (MIT)

Copyright (c) 2015, 2016 Jacob McGladdery

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

-------------------------------------------------------------------------------

Raw Interface Demo

This service demonstrates how to read CAN traffic using the SocketCAN Raw
interface. Specifically, this service shows how to read CAN FD frame, filter by
message ID, perform a blocking read, and process some hypothetical CAN
messages. The exact format of the CAN messages which this service will
recognize is given in the README file.

TODO: Specify the message formats in the README file.
*/

#include <linux/can.h>
#include <linux/can/raw.h>

#include <endian.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define PROGNAME  "socketcan-raw-demo"
#define VERSION  "2.0.0"

namespace {

struct EngineFrame {
    std::uint16_t rpm;
    // TODO: Some more hypothetical data
};

struct VehicleFrame {
    // TODO: Some hypothetical vehicle status measurements
};

struct BodyControllerFrame {
    // TODO: Some hypothetical vehicle settings flags
};

std::sig_atomic_t signalValue;

void onSignal(int value) {
    signalValue = static_cast<decltype(signalValue)>(value);
}

void usage() {
    std::cout << "Usage: " PROGNAME " [-h] [-V] [-f] interface" << std::endl
              << "Options:" << std::endl
              << "  -h  Display this information" << std::endl
              << "  -V  Display version information" << std::endl
              << "  -f  Run in the foreground" << std::endl
              << std::endl;
}

void version() {
    std::cout << PROGNAME " version " VERSION << std::endl
              << "Compiled on " __DATE__ ", " __TIME__ << std::endl
              << std::endl;
}

void processFrame(const struct canfd_frame& frame) {
    switch (frame.can_id) {
    case 0x0A0:
    {
        EngineFrame engine;
        engine.rpm = be16toh(*(std::uint16_t *)(frame.data + 0));
        std::cout << "RPM: " << engine.rpm << std::endl;
    }
        break;
    case 0x110:
    {
        // TODO: Work!
        // VehicleFrame vehicle;
        std::cout << "Got 0x110" << std::endl; // XXX
    }
        break;
    case 0x320:
    {
        // TODO: Work!
        // BodyControllerFrame bodyController;
        std::cout << "Got 0x320" << std::endl; // XXX
    }
        break;
    default:
        // Should never get here if the receive filters were set up correctly
        std::cerr << "Unexpected CAN ID: 0x"
                  << std::hex << std::uppercase
                  << std::setw(3) << std::setfill('0')
                  << frame.can_id << std::endl;
        std::cerr.copyfmt(std::ios(nullptr));
        break;
    }
}

} // namespace

int main(int argc, char** argv) {
    using namespace std::chrono_literals;

    // Options
    const char* interface;
    bool foreground = false;

    // Service variables
    struct sigaction sa;
    int rc;

    // CAN connection variables
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sockfd;

    // Parse command line arguments
    {
        int opt;

        // Parse option flags
        while ((opt = ::getopt(argc, argv, "Vfh")) != -1) {
            switch (opt) {
            case 'V':
                version();
                return EXIT_SUCCESS;
            case 'f':
                foreground = true;
                break;
            case 'h':
                usage();
                return EXIT_SUCCESS;
            default:
                usage();
                return EXIT_FAILURE;
            }
        }

        // Check for the one positional argument
        if (optind != (argc - 1)) {
            std::cerr << "Missing network interface option!" << std::endl;
            usage();
            return EXIT_FAILURE;
        }

        // Set the network interface to use
        interface = argv[optind];
    }

    // Check if the service should be run as a daemon
    if (!foreground) {
        if (::daemon(0, 1) == -1) {
            std::perror("daemon");
            return EXIT_FAILURE;
        }
    }

    // Register signal handlers
    sa.sa_handler = onSignal;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGQUIT, &sa, nullptr);
    ::sigaction(SIGHUP, &sa, nullptr);

    // Initialize the signal value to zero
    signalValue = 0;

    // Open the CAN network interface
    sockfd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (-1 == sockfd) {
        std::perror("socket");
        goto errSocket;
    }

    // Set a receive filter so we only receive select CAN IDs
    {
        struct can_filter filter[3];
        filter[0].can_id   = 0x0A0;
        filter[0].can_mask = CAN_SFF_MASK;
        filter[1].can_id   = 0x110;
        filter[1].can_mask = CAN_SFF_MASK;
        filter[2].can_id   = 0x320;
        filter[2].can_mask = CAN_SFF_MASK;

        rc = ::setsockopt(
            sockfd,
            SOL_CAN_RAW,
            CAN_RAW_FILTER,
            &filter,
            sizeof(filter)
        );
        if (-1 == rc) {
            std::perror("setsockopt filter");
            goto errSetup;
        }
    }

    // Enable reception of CAN FD frames
    {
        int enable = 1;

        rc = ::setsockopt(
            sockfd,
            SOL_CAN_RAW,
            CAN_RAW_FD_FRAMES,
            &enable,
            sizeof(enable)
        );
        if (-1 == rc) {
            std::perror("setsockopt CAN FD");
            goto errSetup;
        }
    }

    // Get the index of the network interface
    std::strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (::ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        std::perror("ioctl");
        goto errSetup;
    }

    // Bind the socket to the network interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    rc = ::bind(
        sockfd,
        reinterpret_cast<struct sockaddr*>(&addr),
        sizeof(addr)
    );
    if (-1 == rc) {
        std::perror("bind");
        goto errSetup;
    }

    // Log that the service is up and running
    std::cout << "Started" << std::endl;

    // Main loop
    while (0 == signalValue) {
        struct canfd_frame frame;

        // Read in a CAN frame
        auto numBytes = ::read(sockfd, &frame, CANFD_MTU);
        switch (numBytes) {
        case CAN_MTU:
            processFrame(frame);
            break;
        case CANFD_MTU:
            // TODO: Should make an example for CAN FD
            break;
        case -1:
            // Check the signal value on interrupt
            if (EINTR == errno)
                continue;

            // Delay before continuing
            std::perror("read");
            std::this_thread::sleep_for(100ms);
        default:
            continue;
        }
    }

    // Cleanup
    if (::close(sockfd) == -1) {
        std::perror("close");
        return errno;
    }

    std::cout << std::endl << "Bye!" << std::endl;
    return EXIT_SUCCESS;

    // Error handling (reverse order cleanup)
errSetup:
    ::close(sockfd);
errSocket:
    return errno;
}
