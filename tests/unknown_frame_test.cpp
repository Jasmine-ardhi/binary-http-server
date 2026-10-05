#include "../src/protocol.h"

#include <iostream>
#include <string>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    const char* host = "127.0.0.1";
    const char* port = "9000";

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;

    int status = getaddrinfo(
        host,
        port,
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

    std::cout << "Connected to server.\n";

    // Send an unknown frame type.
    Frame unknown;

    unknown.header.magic = MAGIC;
    unknown.header.version = VERSION;
    unknown.header.frame_type = 0x7F;
    unknown.header.flags = 0;
    unknown.header.header_count = 0;
    unknown.header.status_code = 0;
    unknown.header.reserved1 = 0;
    unknown.header.reserved2 = 0;

    unknown.payload = {
        0x01, 0x02, 0x03, 0x04
    };

    if (!send_frame(fd, unknown)) {
        std::cerr << "Failed to send unknown frame.\n";
        close(fd);
        return 1;
    }

    std::cout << "Unknown frame sent.\n";

    // Now send a valid request on the SAME connection.
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
        std::cerr << "Failed to encode request.\n";
        close(fd);
        return 1;
    }

    Frame valid;

    valid.header.magic = MAGIC;
    valid.header.version = VERSION;
    valid.header.frame_type = FRAME_REQUEST;
    valid.header.flags = 0;

    valid.header.header_count =
        static_cast<std::uint8_t>(
            request.headers.size()
        );

    valid.header.status_code = 0;
    valid.header.reserved1 = 0;
    valid.header.reserved2 = 0;

    valid.payload = payload;

    if (!send_frame(fd, valid)) {
        std::cerr << "Failed to send valid request.\n";
        close(fd);
        return 1;
    }

    std::cout << "Valid request sent.\n";

    Frame response;

    if (!receive_frame(fd, response)) {
        std::cerr << "Failed to receive response.\n";
        close(fd);
        return 1;
    }

    std::cout << "Received response: status="
              << response.header.status_code
              << "\n";

    if (response.header.status_code == 200) {
        std::cout << "UNKNOWN FRAME TEST PASSED\n";
    } else {
        std::cout << "UNKNOWN FRAME TEST FAILED\n";
    }

    close(fd);

    return 0;
}