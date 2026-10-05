#include "../src/protocol.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
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

    std::vector<std::uint8_t> encoded;

    bool ok = encode_request(request, encoded);

    assert(ok);

    // Path length = 11 for "/index.html"
    assert(encoded[0] == 0);
    assert(encoded[1] == 11);

    // Check that the path starts immediately after the 2-byte length.
    std::string path(
        reinterpret_cast<const char*>(encoded.data() + 2),
        11
    );

    assert(path == "/index.html");

    std::cout << "encode_request test passed!\n";
}