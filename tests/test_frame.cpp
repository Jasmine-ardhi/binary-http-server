#include "../src/protocol.h"

#include <cassert>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main() {
    int sockets[2];

    int result = socketpair(
        AF_UNIX,
        SOCK_STREAM,
        0,
        sockets
    );

    assert(result == 0);

    // Create a request payload
    Request request;

    request.path = "/index.html";
    request.headers.push_back(
        {HEADER_HOST, "localhost:9000"}
    );

    std::vector<std::uint8_t> payload;

    bool encoded = encode_request(
        request,
        payload
    );

    assert(encoded);

    // Create a frame
    Frame outgoing;

    outgoing.header.magic = MAGIC;
    outgoing.header.version = VERSION;
    outgoing.header.frame_type = FRAME_REQUEST;
    outgoing.header.flags = 0;
    outgoing.header.header_count =
        static_cast<std::uint8_t>(request.headers.size());
    outgoing.header.status_code = 0;
    outgoing.header.reserved1 = 0;
    outgoing.header.reserved2 = 0;

    outgoing.payload = payload;

    // Send from socket 0
    bool sent = send_frame(
        sockets[0],
        outgoing
    );

    assert(sent);

    // Receive from socket 1
    Frame incoming;

    bool received = receive_frame(
        sockets[1],
        incoming
    );

    assert(received);

    // Verify frame header
    assert(incoming.header.magic == MAGIC);
    assert(incoming.header.version == VERSION);
    assert(incoming.header.frame_type == FRAME_REQUEST);
    assert(incoming.header.flags == 0);
    assert(incoming.header.header_count == 1);
    assert(incoming.header.status_code == 0);

    // Verify payload
    assert(incoming.payload == outgoing.payload);

    // Decode request
    Request decoded;

    bool decoded_ok = decode_request(
        incoming.payload.data(),
        incoming.payload.size(),
        decoded
    );

    assert(decoded_ok);

    assert(decoded.path == "/index.html");
    assert(decoded.headers.size() == 1);
    assert(decoded.headers[0].id == HEADER_HOST);
    assert(decoded.headers[0].value == "localhost:9000");

    std::cout << "send_frame/receive_frame test passed!\n";

    close(sockets[0]);
    close(sockets[1]);

    return 0;
}