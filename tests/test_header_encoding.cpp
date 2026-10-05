#include "../src/protocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    Header header;

    header.id = HEADER_CONTENT_TYPE;
    header.value = "text/html";

    std::vector<std::uint8_t> encoded;

    bool ok = encode_header(header, encoded);

    assert(ok);

    // Expected:
    // 01          -> header ID
    // 00 09       -> value length = 9
    // 74 65 78 74 2F 68 74 6D 6C -> "text/html"

    assert(encoded.size() == 12);

    assert(encoded[0] == 0x01);
    assert(encoded[1] == 0x00);
    assert(encoded[2] == 0x09);

    const std::uint8_t expected_value[] = {
        0x74, 0x65, 0x78, 0x74, 0x2F,
        0x68, 0x74, 0x6D, 0x6C
    };

    for (std::size_t i = 0; i < 9; ++i) {
        assert(encoded[3 + i] == expected_value[i]);
    }

    std::cout << "Header encoding test passed!\n";

    return 0;
}