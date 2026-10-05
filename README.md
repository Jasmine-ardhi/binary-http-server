# Binary HTTP

A simple HTTP-like binary protocol implemented over TCP in C++.

## Project Overview

This project implements a custom binary HTTP-style client and server. Instead of using traditional text-based HTTP messages, the application communicates using a custom binary frame format over a TCP connection.

The project contains two main programs:

1. `observe` — the TCP server that serves files from a specified web root.
2. `bcurl` — the TCP client that requests files from the server.

The protocol supports:

* Binary requests and responses
* Persistent TCP connections
* Response status codes
* Protocol headers
* Unknown frame types
* Error handling
* Path traversal protection
* Verbose hexadecimal frame dumping

## Project Structure

```text
binary-http/
├── src/
│   ├── server.cpp
│   ├── client.cpp
│   ├── protocol.h
│   └── protocol.cpp
├── www/
│   └── index.html
├── tests/
│   ├── unknown_frame_test.cpp
│   └── bad_request_test.cpp
├── SPEC.md
└── README.md
```

## Build Requirements

The project requires:

* Linux
* A C++17-compatible compiler
* POSIX socket APIs

The project was developed and tested using `g++` and C++17.

## Build

Build the server:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
    src/server.cpp src/protocol.cpp \
    -o observe
```

Build the client:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
    src/client.cpp src/protocol.cpp \
    -o bcurl
```

## Build Test Programs

### Unknown Frame Test

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
    tests/unknown_frame_test.cpp src/protocol.cpp \
    -o unknown_frame_test
```

### Bad Request Test

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
    tests/bad_request_test.cpp src/protocol.cpp \
    -o bad_request_test
```

## Running the Server

Start the server by providing the web root directory and port:

```bash
./observe ./www 9000
```

The server listens for TCP connections on port `9000` and serves files from the `./www` directory.

## Running the Client

The client accepts a host, port, and requested path:

```bash
./bcurl -v localhost:9000/index.html
```

The `-v` option displays the binary request and response frames as hexadecimal output.

## Example Request

Requesting `/index.html`:

```bash
./bcurl -v localhost:9000/index.html
```

The client:

1. Connects to the server.
2. Creates a binary `REQUEST` frame.
3. Sends the request over TCP.
4. Receives the `RESPONSE` frame.
5. Decodes the response headers.
6. Displays the requested file contents.

## Protocol

The protocol uses a fixed 16-byte binary frame header followed by a variable-length payload.

All multi-byte integer fields use **big-endian byte order**.

The frame header contains:

* Magic
* Version
* Frame Type
* Flags
* Header Count
* Payload Length
* Status Code
* Reserved fields

The magic value is `0x4F42` and the protocol version is `1`.

### Frame Types

|  Value | Type       | Description           |
| -----: | ---------- | --------------------- |
| `0x01` | `REQUEST`  | Client request        |
| `0x02` | `RESPONSE` | Server response       |
|  Other | Unknown    | Ignored by the server |

Unknown frame types are safely skipped by reading and discarding their complete payload.

## Protocol Headers

Headers are encoded using:

```text
Header ID
Value Length
Header Value
```

The protocol defines ten header IDs:

|   ID | Header             |
| ---: | ------------------ |
|  `1` | `content-type`     |
|  `2` | `content-length`   |
|  `3` | `connection`       |
|  `4` | `host`             |
|  `5` | `server`           |
|  `6` | `date`             |
|  `7` | `accept`           |
|  `8` | `user-agent`       |
|  `9` | `cache-control`    |
| `10` | `content-encoding` |

## Requests

A request contains a path followed by its encoded headers.

For example, a request for:

```text
/index.html
```

contains the path `/index.html` and headers such as `Host` and `Accept`.

Requests use frame type `0x01` and must have a status code of `0`.

## Responses

A response uses frame type `0x02`.

The status code is stored in the frame header.

The response payload contains the encoded response headers followed by the requested file contents.

The server supports the following status codes:

| Status | Meaning               |
| -----: | --------------------- |
|  `200` | Successful request    |
|  `400` | Bad request           |
|  `404` | File not found        |
|  `500` | Internal server error |

## Persistent Connections

The server supports persistent TCP connections.

Multiple request/response exchanges can take place over the same TCP connection.

For example:

```text
Client → Request 1
Server → Response 1

Client → Request 2
Server → Response 2
```

The client does not create a second TCP connection for the second request.

## Error Handling

Malformed requests that can still be safely processed result in a `400 Bad Request` response.

If the requested file does not exist, the server returns `404 Not Found`.

Internal file or server failures return `500 Internal Server Error`.

The client exits with a non-zero status when a `4xx` or `5xx` response is received.

## Unknown Frame Handling

The server does not fail when it receives an unknown frame type.

It:

1. Reads the frame header.
2. Uses the payload length from the header.
3. Reads and discards the complete unknown payload.
4. Continues processing the next frame on the same TCP connection.

This allows future frame types to be ignored by older implementations.

This behavior was tested using frame type `0x7F` followed by a valid request. The server successfully ignored the unknown frame and returned a `200` response for the following request.

## Path Traversal Protection

The server prevents requests from escaping the configured web root.

For example:

```text
/index.html
```

is allowed.

A path such as:

```text
/../etc/passwd
```

is rejected with a `400 Bad Request` response.

## TCP I/O

TCP may perform partial reads and writes. Therefore, the implementation provides:

* `read_exact()` — ensures that the requested number of bytes is read.
* `write_all()` — ensures that the complete buffer is written.

Interrupted system calls are retried.

## Maximum Payload

The maximum supported frame payload is:

```text
16 MiB
```

A frame advertising a payload larger than this limit is rejected.

## Example Successful Response

A successful request for `/index.html` returns:

```text
Status: 200
Content-Type: text/html
Content-Length: 59
Connection: keep-alive
Server: observe/1.0
```

The client then displays the contents of the requested file.

## Testing

The following functionality has been tested:

* [x] Successful file request returning `200`
* [x] Binary request and response frame serialization
* [x] Big-endian integer encoding and decoding
* [x] Multiple protocol headers
* [x] Persistent TCP connection with two requests
* [x] Unknown frame type handling
* [x] `404` response for a missing file
* [x] `400` response for an invalid request
* [x] Path traversal protection
* [x] Client non-zero exit status for error responses
* [x] Verbose hexadecimal frame dumping

## Design Notes

The protocol uses binary frames so that message boundaries and fields can be represented explicitly rather than relying on textual HTTP parsing.

The fixed-size frame header allows the receiver to determine the frame type, payload size, and status before decoding the payload.

The payload length also allows unknown frame types to be skipped without terminating the TCP connection.

The implementation uses a maximum payload size of 16 MiB to prevent unbounded memory allocation from a malicious payload length.

## Author

**Jasmine Ardhi**


