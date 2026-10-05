#include "protocol.h"

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

bool read_exact(int fd, void* buffer, std::size_t length) {
    std::size_t total_read = 0;

    while (total_read < length) {
        ssize_t n = recv(
            fd,
            static_cast<char*>(buffer) + total_read,
            length - total_read,
            0
        );

        if (n > 0) {
            total_read += static_cast<std::size_t>(n);
        } else if (n == 0) {
            // Peer closed the connection.
            return false;
        } else {
            if (errno == EINTR) {
                continue;
            }

            return false;
        }
    }

    return true;
}

bool write_all(int fd, const void* buffer, std::size_t length) {
    std::size_t total_written = 0;

    while (total_written < length) {
        ssize_t n = send(
            fd,
            static_cast<const char*>(buffer) + total_written,
            length - total_written,
            0
        );

        if (n > 0) {
            total_written += static_cast<std::size_t>(n);
        } else if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            return false;
        }
    }

    return true;
}

void write_u16(std::uint8_t* buffer, std::uint16_t value) {
    buffer[0] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    buffer[1] = static_cast<std::uint8_t>(value & 0xFF);
}

std::uint16_t read_u16(const std::uint8_t* buffer) {
    return
        (static_cast<std::uint16_t>(buffer[0]) << 8) |
        static_cast<std::uint16_t>(buffer[1]);
}

void write_u32(std::uint8_t* buffer, std::uint32_t value) {
    buffer[0] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
    buffer[1] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    buffer[2] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    buffer[3] = static_cast<std::uint8_t>(value & 0xFF);
}

std::uint32_t read_u32(const std::uint8_t* buffer) {
    return
        (static_cast<std::uint32_t>(buffer[0]) << 24) |
        (static_cast<std::uint32_t>(buffer[1]) << 16) |
        (static_cast<std::uint32_t>(buffer[2]) << 8) |
        static_cast<std::uint32_t>(buffer[3]);
}

void serialize_header(
    const FrameHeader& header,
    std::uint8_t* buffer
) {
    write_u16(buffer, header.magic);

    buffer[2] = header.version;
    buffer[3] = header.frame_type;
    buffer[4] = header.flags;
    buffer[5] = header.header_count;

    write_u32(buffer + 6, header.payload_length);

    write_u16(buffer + 10, header.status_code);
    write_u16(buffer + 12, header.reserved1);
    write_u16(buffer + 14, header.reserved2);
}

bool deserialize_header(
    const std::uint8_t* buffer,
    FrameHeader& header
) {
    header.magic = read_u16(buffer);

    header.version = buffer[2];
    header.frame_type = buffer[3];
    header.flags = buffer[4];
    header.header_count = buffer[5];

    header.payload_length = read_u32(buffer + 6);

    header.status_code = read_u16(buffer + 10);
    header.reserved1 = read_u16(buffer + 12);
    header.reserved2 = read_u16(buffer + 14);

    return true;
}

bool validate_header(const FrameHeader& header) {
    if (header.magic != MAGIC) {
        return false;
    }

    if (header.version != VERSION) {
        return false;
    }

    if (header.flags != 0) {
        return false;
    }

    if (header.payload_length > MAX_PAYLOAD_SIZE) {
        return false;
    }

    if (header.reserved1 != 0 ||
        header.reserved2 != 0) {
        return false;
    }

    return true;
}

bool encode_header(
    const Header& header,
    std::vector<std::uint8_t>& output
) {
    if (header.id < HEADER_CONTENT_TYPE ||
        header.id > HEADER_CONTENT_ENCODING) {
        return false;
    }

    if (header.value.size() > 65535) {
        return false;
    }

    std::size_t old_size = output.size();

    // 1 byte: header ID
    output.push_back(header.id);

    // 2 bytes: value length
    std::uint8_t length_buffer[2];

    write_u16(
        length_buffer,
        static_cast<std::uint16_t>(header.value.size())
    );

    output.push_back(length_buffer[0]);
    output.push_back(length_buffer[1]);

    // N bytes: header value
    output.insert(
        output.end(),
        header.value.begin(),
        header.value.end()
    );

    return output.size() == old_size + 3 + header.value.size();
}

bool decode_header(
    const std::uint8_t* buffer,
    std::size_t length,
    Header& header,
    std::size_t& bytes_consumed
) {
    bytes_consumed = 0;

    // Minimum size:
    // 1 byte ID + 2 bytes value length
    if (length < 3) {
        return false;
    }

    header.id = buffer[0];

    // Header IDs must be between 1 and 10.
    if (header.id < HEADER_CONTENT_TYPE ||
        header.id > HEADER_CONTENT_ENCODING) {
        return false;
    }

    std::uint16_t value_length = read_u16(buffer + 1);

    // Check that the complete value is present.
    if (length < 3 + static_cast<std::size_t>(value_length)) {
        return false;
    }

    header.value.assign(
        reinterpret_cast<const char*>(buffer + 3),
        value_length
    );

    bytes_consumed = 3 + value_length;

    return true;
}

bool encode_headers(
    const std::vector<Header>& headers,
    std::vector<std::uint8_t>& output
) {
    for (const Header& header : headers) {
        if (!encode_header(header, output)) {
            return false;
        }
    }

    return true;
}

bool decode_headers(
    const std::uint8_t* buffer,
    std::size_t length,
    std::vector<Header>& headers
) {
    headers.clear();

    std::size_t offset = 0;

    while (offset < length) {
        Header header{};
        std::size_t bytes_consumed = 0;

        bool ok = decode_header(
            buffer + offset,
            length - offset,
            header,
            bytes_consumed
        );

        if (!ok) {
            return false;
        }

        headers.push_back(header);

        offset += bytes_consumed;
    }

    return offset == length;
}

bool encode_request(
    const Request& request,
    std::vector<std::uint8_t>& output
) {
    output.clear();

    if (request.path.size() > 65535) {
        return false;
    }

    std::uint8_t path_length_buffer[2];

    write_u16(
        path_length_buffer,
        static_cast<std::uint16_t>(request.path.size())
    );

    output.push_back(path_length_buffer[0]);
    output.push_back(path_length_buffer[1]);

    output.insert(
        output.end(),
        request.path.begin(),
        request.path.end()
    );

    if (!encode_headers(request.headers, output)) {
        return false;
    }

    return true;
}

bool decode_request(
    const std::uint8_t* buffer,
    std::size_t length,
    Request& request
) {
    request.path.clear();
    request.headers.clear();

    // We need at least 2 bytes for the path length.
    if (length < 2) {
        return false;
    }

    std::uint16_t path_length = read_u16(buffer);

    std::size_t path_end =
        2 + static_cast<std::size_t>(path_length);

    // Make sure the complete path is present.
    if (length < path_end) {
        return false;
    }

    request.path.assign(
        reinterpret_cast<const char*>(buffer + 2),
        path_length
    );

    // Everything after the path is the header block.
    const std::uint8_t* header_buffer =
        buffer + path_end;

    std::size_t header_length =
        length - path_end;

    if (!decode_headers(
            header_buffer,
            header_length,
            request.headers
        )) {
        return false;
    }

    return true;
}

bool encode_response(
    const Response& response,
    std::vector<std::uint8_t>& output
) {
    output.clear();

    // Encode all response headers first.
    std::vector<std::uint8_t> encoded_headers;

    if (!encode_headers(response.headers, encoded_headers)) {
        return false;
    }

    // Header block length is stored in 2 bytes.
    if (encoded_headers.size() > 65535) {
        return false;
    }

    std::uint8_t header_length_buffer[2];

    write_u16(
        header_length_buffer,
        static_cast<std::uint16_t>(encoded_headers.size())
    );

    // Write header block length.
    output.push_back(header_length_buffer[0]);
    output.push_back(header_length_buffer[1]);

    // Write encoded headers.
    output.insert(
        output.end(),
        encoded_headers.begin(),
        encoded_headers.end()
    );

    // Write file/body bytes.
    output.insert(
        output.end(),
        response.body.begin(),
        response.body.end()
    );

    return true;
}

bool decode_response(
    const std::uint8_t* buffer,
    std::size_t length,
    Response& response
) {
    response.headers.clear();
    response.body.clear();

    // Need at least 2 bytes for header block length.
    if (length < 2) {
        return false;
    }

    std::uint16_t header_length = read_u16(buffer);

    std::size_t header_end =
        2 + static_cast<std::size_t>(header_length);

    // Make sure the complete header block exists.
    if (length < header_end) {
        return false;
    }

    // Decode headers.
    if (!decode_headers(
            buffer + 2,
            header_length,
            response.headers
        )) {
        return false;
    }

    // Everything after the header block is the body.
    response.body.assign(
        buffer + header_end,
        buffer + length
    );

    return true;
}

bool send_frame(
    int fd,
    const Frame& frame
) {
    if (frame.payload.size() > MAX_PAYLOAD_SIZE) {
        return false;
    }

    FrameHeader header = frame.header;

    header.payload_length =
        static_cast<std::uint32_t>(frame.payload.size());

    std::uint8_t header_buffer[FRAME_HEADER_SIZE];

    serialize_header(
        header,
        header_buffer
    );

    if (!write_all(
            fd,
            header_buffer,
            FRAME_HEADER_SIZE
        )) {
        return false;
    }

    if (!frame.payload.empty()) {
        if (!write_all(
                fd,
                frame.payload.data(),
                frame.payload.size()
            )) {
            return false;
        }
    }

    return true;
}

bool receive_frame(
    int fd,
    Frame& frame
) {
    std::uint8_t header_buffer[FRAME_HEADER_SIZE];

    if (!read_exact(
            fd,
            header_buffer,
            FRAME_HEADER_SIZE
        )) {
        return false;
    }

    FrameHeader header;

    deserialize_header(
        header_buffer,
        header
    );

    if (!validate_header(header)) {
        return false;
    }

    frame.header = header;
    frame.payload.clear();

    if (header.payload_length == 0) {
        return true;
    }

    frame.payload.resize(header.payload_length);

    if (!read_exact(
            fd,
            frame.payload.data(),
            header.payload_length
        )) {
        frame.payload.clear();
        return false;
    }

    return true;
}