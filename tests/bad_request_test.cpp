#include "../src/protocol.h"

#include <iostream>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;

    int status = getaddrinfo(
        "127.0.0.1",
        "9000",
        &hints,
        &result
    );

    if (status != 0) {
        std::cerr << "getaddrinfo failed\n";
        return 1;
    }

    int fd = socket(
        result->ai_family,
        result->ai_socktype,
        result->ai_protocol
    );

    if (fd < 0) {
        std::cerr << "socket failed\n";
        freeaddrinfo(result);
        return 1;
    }

    if (connect(
            fd,
            result->ai_addr,
            result->ai_addrlen
        ) < 0) {

        std::cerr << "connect failed\n";
        close(fd);
        freeaddrinfo(result);
        return 1;
    }

    freeaddrinfo(result);

    Request request;
    request.path = "/index.html";

    request.headers.push_back({
        HEADER_HOST,
        "localhost:9000"
    });

    request.headers.push_back({
        HEADER_ACCEPT,
        "*/*"
    });

    std::vector<std::uint8_t> payload;

    if (!encode_request(request, payload)) {
        std::cerr << "Failed to encode request\n";
        close(fd);
        return 1;
    }

    Frame frame;

    frame.header.magic = MAGIC;
    frame.header.version = VERSION;
    frame.header.frame_type = FRAME_REQUEST;
    frame.header.flags = 0;

    frame.header.header_count =
        static_cast<std::uint8_t>(
            request.headers.size()
        );

    // Invalid for a request.
    frame.header.status_code = 999;

    frame.header.reserved1 = 0;
    frame.header.reserved2 = 0;

    frame.payload = payload;

    if (!send_frame(fd, frame)) {
        std::cerr << "Failed to send bad request\n";
        close(fd);
        return 1;
    }

    std::cout << "Bad request sent.\n";

    Frame response;

    if (!receive_frame(fd, response)) {
        std::cerr << "Failed to receive response\n";
        close(fd);
        return 1;
    }

    std::cout << "Received response: status="
              << response.header.status_code
              << "\n";

    if (response.header.status_code == 400) {
        std::cout << "400 TEST PASSED\n";
    } else {
        std::cout << "400 TEST FAILED\n";
    }

    close(fd);

    return 0;
}