#include "../src/protocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    // Case 1: Header is too short.
    {
        const std::uint8_t data[] = {
            0x01,
            0x00
        };

        std::vector<Header> headers;

        bool ok = decode_headers(
            data,
            sizeof(data),
            headers
        );

        assert(!ok);
    }

    // Case 2: Header claims a 9-byte value,
    // but only 2 bytes are actually present.
    {
        const std::uint8_t data[] = {
            HEADER_CONTENT_TYPE,
            0x00,
            0x09,
            'h',
            'i'
        };

        std::vector<Header> headers;

        bool ok = decode_headers(
            data,
            sizeof(data),
            headers
        );

        assert(!ok);
    }

    // Case 3: Unknown header ID.
    {
        const std::uint8_t data[] = {
            99,
            0x00,
            0x03,
            'a',
            'b',
            'c'
        };

        std::vector<Header> headers;

        bool ok = decode_headers(
            data,
            sizeof(data),
            headers
        );

        assert(!ok);
    }

    std::cout << "Malformed header tests passed!\n";

    return 0;
}