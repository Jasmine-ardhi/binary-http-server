#include "../src/protocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    // Test 16-bit integer
    std::uint8_t buffer16[2];

    write_u16(buffer16, 0x1234);

    assert(buffer16[0] == 0x12);
    assert(buffer16[1] == 0x34);

    std::uint16_t value16 = read_u16(buffer16);

    assert(value16 == 0x1234);


    // Test 32-bit integer
    std::uint8_t buffer32[4];

    write_u32(buffer32, 0x12345678);

    assert(buffer32[0] == 0x12);
    assert(buffer32[1] == 0x34);
    assert(buffer32[2] == 0x56);
    assert(buffer32[3] == 0x78);

    std::uint32_t value32 = read_u32(buffer32);

    assert(value32 == 0x12345678);


    std::cout << "Endian encoding/decoding test passed!\n";

    return 0;
}