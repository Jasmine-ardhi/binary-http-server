#include "../src/protocol.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    Response original;

    original.status_code = 200;

    original.headers.push_back({
        HEADER_CONTENT_TYPE,
        "text/html"
    });

    original.headers.push_back({
        HEADER_CONTENT_LENGTH,
        "13"
    });

    const std::string body = "Hello, world!";

    original.body.assign(
        body.begin(),
        body.end()
    );

    std::vector<std::uint8_t> encoded;

    bool encode_ok = encode_response(
        original,
        encoded
    );

    assert(encode_ok);

    Response decoded;

    bool decode_ok = decode_response(
        encoded.data(),
        encoded.size(),
        decoded
    );

    assert(decode_ok);

    assert(decoded.headers.size() == 2);

    assert(decoded.headers[0].id == HEADER_CONTENT_TYPE);
    assert(decoded.headers[0].value == "text/html");

    assert(decoded.headers[1].id == HEADER_CONTENT_LENGTH);
    assert(decoded.headers[1].value == "13");

    std::string decoded_body(
        decoded.body.begin(),
        decoded.body.end()
    );

    assert(decoded_body == body);

    std::cout << "response encode/decode test passed!\n";
}