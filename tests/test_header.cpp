#include "../src/protocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    FrameHeader original{};

    original.magic = MAGIC;
    original.version = VERSION;
    original.frame_type = FRAME_REQUEST;
    original.flags = 0;
    original.header_count = 2;
    original.payload_length = 1234;
    original.status_code = 0;
    original.reserved1 = 0;
    original.reserved2 = 0;

    std::uint8_t buffer[FRAME_HEADER_SIZE] = {};

    // Convert struct -> 16 bytes
    serialize_header(original, buffer);

    // Convert 16 bytes -> struct
    FrameHeader decoded{};

    bool ok = deserialize_header(buffer, decoded);

    assert(ok);

    // Verify every field
    assert(decoded.magic == original.magic);
    assert(decoded.version == original.version);
    assert(decoded.frame_type == original.frame_type);
    assert(decoded.flags == original.flags);
    assert(decoded.header_count == original.header_count);
    assert(decoded.payload_length == original.payload_length);
    assert(decoded.status_code == original.status_code);
    assert(decoded.reserved1 == original.reserved1);
    assert(decoded.reserved2 == original.reserved2);

    std::cout << "Frame header round-trip test passed!\n";

    return 0;
}