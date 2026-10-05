#include "../src/protocol.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int sockets[2];

    // Create two connected sockets.
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
        perror("socketpair");
        return 1;
    }

    const char message[] = "Hello Binary HTTP";

    // Socket 0 writes.
    bool write_ok = write_all(
        sockets[0],
        message,
        sizeof(message)
    );

    assert(write_ok);

    // Socket 1 reads.
    char buffer[sizeof(message)] = {};

    bool read_ok = read_exact(
        sockets[1],
        buffer,
        sizeof(buffer)
    );

    assert(read_ok);

    // Verify the bytes are identical.
    assert(std::memcmp(message, buffer, sizeof(message)) == 0);

    std::cout << "read_exact/write_all test passed!\n";
    std::cout << "Received: " << buffer << '\n';

    close(sockets[0]);
    close(sockets[1]);

    return 0;
}
