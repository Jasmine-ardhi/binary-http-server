#include "../src/protocol.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    std::vector<Header> headers;

    headers.push_back({
        HEADER_CONTENT_TYPE,
        "text/html"
    });

    headers.push_back({
        HEADER_CONNECTION,
        "keep-alive"
    });

    std::vector<std::uint8_t> encoded;

    bool ok = encode_headers(headers, encoded);

    assert(ok);

    /*
        Header 1:

        01          -> ID = content-type
        00 09       -> length = 9
        text/html   -> 9 bytes

        Total = 12 bytes


        Header 2:

        03          -> ID = connection
        00 0A       -> length = 10
        keep-alive  -> 10 bytes

        Total = 13 bytes

        Combined = 25 bytes
    */

    assert(encoded.size() == 25);

    // First header
    assert(encoded[0] == HEADER_CONTENT_TYPE);
    assert(encoded[1] == 0x00);
    assert(encoded[2] == 0x09);

    // Second header starts at byte 12
    assert(encoded[12] == HEADER_CONNECTION);
    assert(encoded[13] == 0x00);
    assert(encoded[14] == 0x0A);

    std::cout << "Multiple header encoding test passed!\n";

    return 0;
}