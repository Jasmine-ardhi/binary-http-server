#include "../src/protocol.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    std::vector<Header> original_headers;

    original_headers.push_back({
        HEADER_CONTENT_TYPE,
        "text/html"
    });

    original_headers.push_back({
        HEADER_CONNECTION,
        "keep-alive"
    });

    // Encode the headers.
    std::vector<std::uint8_t> encoded;

    bool encode_ok = encode_headers(
        original_headers,
        encoded
    );

    assert(encode_ok);

    // Decode them again.
    std::vector<Header> decoded_headers;

    bool decode_ok = decode_headers(
        encoded.data(),
        encoded.size(),
        decoded_headers
    );

    assert(decode_ok);

    // We should get exactly 2 headers back.
    assert(decoded_headers.size() == 2);

    // Check header 1.
    assert(decoded_headers[0].id == HEADER_CONTENT_TYPE);
    assert(decoded_headers[0].value == "text/html");

    // Check header 2.
    assert(decoded_headers[1].id == HEADER_CONNECTION);
    assert(decoded_headers[1].value == "keep-alive");

    std::cout << "Multiple header decoding test passed!\n";

    return 0;
}