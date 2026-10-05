#include "../src/protocol.h"

#include <cassert>
#include <iostream>

int main() {
    FrameHeader header{};

    // A valid request header.
    header.magic = MAGIC;
    header.version = VERSION;
    header.frame_type = FRAME_REQUEST;
    header.flags = 0;
    header.header_count = 0;
    header.payload_length = 100;
    header.status_code = 0;
    header.reserved1 = 0;
    header.reserved2 = 0;

    assert(validate_header(header));

    // Invalid magic.
    header.magic = 0x1234;
    assert(!validate_header(header));

    header.magic = MAGIC;

    // Invalid version.
    header.version = 2;
    assert(!validate_header(header));

    header.version = VERSION;

    // Invalid frame type.
    header.frame_type = 0x99;
    assert(!validate_header(header));

    header.frame_type = FRAME_REQUEST;

    // Invalid flags.
    header.flags = 1;
    assert(!validate_header(header));

    header.flags = 0;

    // Payload too large.
    header.payload_length = MAX_PAYLOAD_SIZE + 1;
    assert(!validate_header(header));

    header.payload_length = 100;

    // Reserved field must be zero.
    header.reserved1 = 1;
    assert(!validate_header(header));

    header.reserved1 = 0;

    header.reserved2 = 1;
    assert(!validate_header(header));

    std::cout << "Header validation test passed!\n";

    return 0;
}