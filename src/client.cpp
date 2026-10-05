#include "protocol.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>

void dump_frame_hex(
    const Frame& frame,
    const std::string& direction
) {
    std::uint8_t header_buffer[FRAME_HEADER_SIZE];

    serialize_header(
        frame.header,
        header_buffer
    );

    std::cout << "\n--- "
              << direction
              << " frame ---\n";

    std::cout << "Header:\n";

    for (std::size_t i = 0;
         i < FRAME_HEADER_SIZE;
         ++i) {

        std::printf(
            "%02X ",
            header_buffer[i]
        );

        if ((i + 1) % 16 == 0) {
            std::cout << "\n";
        }
    }

    std::cout << "Payload:\n";

    for (std::size_t i = 0;
         i < frame.payload.size();
         ++i) {

        std::printf(
            "%02X ",
            frame.payload[i]
        );

        if ((i + 1) % 16 == 0) {
            std::cout << "\n";
        }
    }

    if (frame.payload.empty() ||
        frame.payload.size() % 16 != 0) {

        std::cout << "\n";
    }

    std::cout << "--- end frame ---\n\n";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./bcurl [-v] host:port/path\n";
        return 1;
    }

    bool verbose = false;
    const char* url = argv[2];

    if (std::string(argv[1]) != "-v") {
        std::cerr << "Only -v mode is supported currently.\n";
        return 1;
    }

    std::string target(url);

    // Find the colon separating host and port.
    std::size_t colon = target.find(':');

    if (colon == std::string::npos) {
        std::cerr << "Invalid target. Expected host:port/path\n";
        return 1;
    }

    // Find the slash separating port and path.
    std::size_t slash = target.find('/', colon + 1);

    if (slash == std::string::npos) {
        std::cerr << "Invalid target. Expected host:port/path\n";
        return 1;
    }

    std::string host =
        target.substr(0, colon);

    std::string port =
        target.substr(
            colon + 1,
            slash - colon - 1
        );

    std::string path =
        target.substr(slash);

    std::cout << "Host: " << host << "\n";
    std::cout << "Port: " << port << "\n";
    std::cout << "Path: " << path << "\n";

    // Resolve the server address.
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;

    int status = getaddrinfo(
        host.c_str(),
        port.c_str(),
        &hints,
        &result
    );

    if (status != 0) {
        std::cerr << "getaddrinfo() failed: "
                  << gai_strerror(status) << "\n";
        return 1;
    }

    int client_fd = socket(
        result->ai_family,
        result->ai_socktype,
        result->ai_protocol
    );

    if (client_fd < 0) {
        std::cerr << "socket() failed: "
                  << std::strerror(errno) << "\n";

        freeaddrinfo(result);
        return 1;
    }

    if (connect(
            client_fd,
            result->ai_addr,
            result->ai_addrlen
        ) < 0) {

        std::cerr << "connect() failed: "
                  << std::strerror(errno) << "\n";

        close(client_fd);
        freeaddrinfo(result);
        return 1;
    }

    freeaddrinfo(result);

    std::cout << "Connected to server.\n";

    // Build request.
    Request request;

    request.path = path;

    request.headers.push_back({
        HEADER_HOST,
        host + ":" + port
    });

    request.headers.push_back({
        HEADER_ACCEPT,
        "*/*"
    });

    std::vector<std::uint8_t> payload;

    if (!encode_request(
            request,
            payload
        )) {

        std::cerr << "Failed to encode request.\n";

        close(client_fd);
        return 1;
    }

    // Build frame.
    
    Frame frame;

    frame.header.magic = MAGIC;
    frame.header.version = VERSION;
    frame.header.frame_type = FRAME_REQUEST;
    frame.header.flags = 0;

    frame.header.header_count =
        static_cast<std::uint8_t>(
            request.headers.size()
        );

    frame.header.status_code = 0;
    frame.header.reserved1 = 0;
    frame.header.reserved2 = 0;

    frame.payload = payload;

    verbose = true;

    frame.header.payload_length =
    static_cast<std::uint32_t>(frame.payload.size());

    if (verbose) {
    std::cout << "Sending request frame...\n";
    dump_frame_hex(frame, "Outgoing");
    }

    if (!send_frame(
            client_fd,
            frame
        )) {

        std::cerr << "Failed to send request frame.\n";

        close(client_fd);
        return 1;
    }

    std::cout << "Request sent.\n";

    // Wait for response.
    Frame response_frame;

    if (!receive_frame(
            client_fd,
            response_frame
        )) {

        std::cerr << "Failed to receive response frame.\n";

        close(client_fd);
        return 1;
    }

    if (verbose) {
        std::cout << "Received response frame...\n";
        dump_frame_hex(response_frame, "Incoming");
    }

    std::cout << "Received response frame: "
              << "type="
              << static_cast<int>(
                     response_frame.header.frame_type
                 )
              << ", status="
              << response_frame.header.status_code
              << ", payload="
              << response_frame.header.payload_length
              << " bytes\n";
    
              if (response_frame.header.frame_type != FRAME_RESPONSE) {
                std::cerr << "Unexpected frame type from server.\n";
                close(client_fd);
                return 1;
            }
            
            Response response;
            
            if (!decode_response(
                response_frame.payload.data(),
                response_frame.payload.size(),
                response
                )) {
            
                std::cerr << "Failed to decode response.\n";
                close(client_fd);
                return 1;
            }

            response.status_code = response_frame.header.status_code;

           

if (response.headers.size() != response_frame.header.header_count) {
    std::cerr << "Response header count mismatch.\n";
    close(client_fd);
    return 1;
}

std::cout << "Status: "
          << response.status_code
          << "\n";
            
          
            
                      if (response.status_code >= 400) {
                        std::cerr << "Request failed with status "
                                  << response.status_code
                                  << ".\n";
                    
                        close(client_fd);
                        return 1;
                    }
            
            std::cout << "Response headers:\n";
            
            for (const Header& header : response.headers) {
                std::cout << "  Header ID "
                          << static_cast<int>(header.id)
                          << ": "
                          << header.value
                          << "\n";
            }
            
            std::cout << "Body ("
                      << response.body.size()
                      << " bytes):\n";
            
            std::cout.write(
                reinterpret_cast<const char*>(response.body.data()),
                static_cast<std::streamsize>(response.body.size())
            );
            
            std::cout << "\n";

            std::cout << "\nSending second request on the same TCP connection...\n";

            frame.header.payload_length =
    static_cast<std::uint32_t>(frame.payload.size());

if (verbose) {
    dump_frame_hex(frame, "Outgoing");
}

if (!send_frame(client_fd, frame)) {
    std::cerr << "Failed to send second request.\n";
    close(client_fd);
    return 1;
}

// Receive second response
Frame second_response_frame;

if (!receive_frame(client_fd, second_response_frame)) {
    std::cerr << "Failed to receive second response.\n";
    close(client_fd);
    return 1;
}

if (verbose) {
    std::cout << "Received second response frame...\n";
    dump_frame_hex(second_response_frame, "Incoming");
}

std::cout << "Second response received: status="
          << second_response_frame.header.status_code
          << "\n";

close(client_fd);

return 0;
}