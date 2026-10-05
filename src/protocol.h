#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

bool read_exact(int fd, void* buffer, std::size_t length);

bool write_all(int fd, const void* buffer, std::size_t length);

void write_u16(std::uint8_t* buffer, std::uint16_t value);

std::uint16_t read_u16(const std::uint8_t* buffer);

void write_u32(std::uint8_t* buffer, std::uint32_t value);

std::uint32_t read_u32(const std::uint8_t* buffer);

struct FrameHeader {
    std::uint16_t magic;
    std::uint8_t version;
    std::uint8_t frame_type;
    std::uint8_t flags;
    std::uint8_t header_count;
    std::uint32_t payload_length;
    std::uint16_t status_code;
    std::uint16_t reserved1;
    std::uint16_t reserved2;
};

struct Header {
    std::uint8_t id;
    std::string value;
};

struct Request {
    std::string path;
    std::vector<Header> headers;
};

struct Response {
    std::uint16_t status_code;
    std::vector<Header> headers;
    std::vector<std::uint8_t> body;
};

struct Frame {
    FrameHeader header;
    std::vector<std::uint8_t> payload;
};

bool encode_request(
    const Request& request,
    std::vector<std::uint8_t>& output
);

bool decode_request(
    const std::uint8_t* buffer,
    std::size_t length,
    Request& request
);

bool encode_response(
    const Response& response,
    std::vector<std::uint8_t>& output
);

bool decode_response(
    const std::uint8_t* buffer,
    std::size_t length,
    Response& response
);

bool send_frame(
    int fd,
    const Frame& frame
);

bool receive_frame(
    int fd,
    Frame& frame
);

bool encode_header(
    const Header& header,
    std::vector<std::uint8_t>& output
);

bool decode_header(
    const std::uint8_t* buffer,
    std::size_t length,
    Header& header,
    std::size_t& bytes_consumed
);

bool encode_headers(
    const std::vector<Header>& headers,
    std::vector<std::uint8_t>& output
);

bool decode_headers(
    const std::uint8_t* buffer,
    std::size_t length,
    std::vector<Header>& headers
);

constexpr std::uint16_t MAGIC = 0x4F42;
constexpr std::uint8_t VERSION = 1;

constexpr std::uint8_t FRAME_REQUEST = 0x01;
constexpr std::uint8_t FRAME_RESPONSE = 0x02;


constexpr std::uint8_t HEADER_CONTENT_TYPE = 1;
constexpr std::uint8_t HEADER_CONTENT_LENGTH = 2;
constexpr std::uint8_t HEADER_CONNECTION = 3;
constexpr std::uint8_t HEADER_HOST = 4;
constexpr std::uint8_t HEADER_SERVER = 5;
constexpr std::uint8_t HEADER_DATE = 6;
constexpr std::uint8_t HEADER_ACCEPT = 7;
constexpr std::uint8_t HEADER_USER_AGENT = 8;
constexpr std::uint8_t HEADER_CACHE_CONTROL = 9;
constexpr std::uint8_t HEADER_CONTENT_ENCODING = 10;


constexpr std::size_t FRAME_HEADER_SIZE = 16;

constexpr std::uint32_t MAX_PAYLOAD_SIZE = 16 * 1024 * 1024;

void serialize_header(
    const FrameHeader& header,
    std::uint8_t* buffer
);

bool deserialize_header(
    const std::uint8_t* buffer,
    FrameHeader& header
);

bool validate_header(const FrameHeader& header);

#endif