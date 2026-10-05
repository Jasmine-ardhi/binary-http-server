#include "../src/protocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    const std::uint8_t encoded[] = {
        0x01,
        0x00, 0x09,
        0x74, 0x65, 0x78, 0x74, 0x2F,
        0x68, 0x74, 0x6D, 0x6C
    };

    Header header{};
    std::size_t bytes_consumed = 0;

    bool ok = decode_header(
        encoded,
        sizeof(encoded),
        header,
        bytes_consumed
    );

    assert(ok);

    assert(header.id == HEADER_CONTENT_TYPE);
    assert(header.value == "text/html");
    assert(bytes_consumed == sizeof(encoded));

    std::cout << "Header decoding test passed!\n";

    return 0;
}