# Binary HTTP Protocol Specification

## 1. Overview

Binary HTTP is a simple HTTP-like protocol implemented over TCP.

The protocol uses binary frames instead of textual HTTP messages.

The server is started as:

    ./observe ./www 9000

The client is used as:

    ./bcurl -v localhost:9000/index.html

All multi-byte integer fields use **big-endian (network byte order)**.

---

## 2. Frame Format

Every frame consists of:

    16-byte fixed header
    +
    variable-length payload

### Fixed Header

| Offset | Size | Field | Description |
|---|---:|---|---|
| 0 | 2 | Magic | `0x4F42` |
| 2 | 1 | Version | Protocol version, currently `1` |
| 3 | 1 | Frame Type | Request, response, or unknown |
| 4 | 1 | Flags | Must be `0` in version 1 |
| 5 | 1 | Header Count | Number of encoded headers |
| 6 | 4 | Payload Length | Payload size in bytes |
| 10 | 2 | Status Code | HTTP-style status code |
| 12 | 2 | Reserved 1 | Must be `0` |
| 14 | 2 | Reserved 2 | Must be `0` |

### Frame Types

| Value | Meaning |
|---|---|
| `0x01` | REQUEST |
| `0x02` | RESPONSE |
| Other | Unknown frame type |

Unknown frame types are skipped using their payload length. The connection remains usable.

---

## 3. Header Encoding

Each protocol header is encoded as:

    1 byte   Header ID
    2 bytes  Value Length
    N bytes  UTF-8 Value

The following ten header IDs are defined:

| ID | Header |
|---:|---|
| 1 | content-type |
| 2 | content-length |
| 3 | connection |
| 4 | host |
| 5 | server |
| 6 | date |
| 7 | accept |
| 8 | user-agent |
| 9 | cache-control |
| 10 | content-encoding |

Only IDs from 1 through 10 are valid.

---

## 4. Request Format

A request uses frame type `0x01`.

The request status code must be `0`.

The request payload contains:

    2 bytes  Path Length
    N bytes  Path
    Headers

The path is encoded as UTF-8.

Example:

    /index.html

A request does not contain a request body in version 1.

---

## 5. Response Format

A response uses frame type `0x02`.

The status code is stored in the frame header.

The response payload contains:

    2 bytes  Header Block Length
    N bytes  Encoded Headers
    Remaining bytes  Response Body

The response body contains the requested file contents.

Example successful response:

    Status: 200
    Content-Type: text/html
    Content-Length: 59
    Connection: keep-alive
    Server: observe/1.0

---

## 6. Status Codes

The server supports:

| Status | Meaning |
|---:|---|
| 200 | Successful request |
| 400 | Bad request |
| 404 | File not found |
| 500 | Internal server error |

The client exits with a non-zero status when it receives a 4xx or 5xx response.

---

## 7. Persistent Connections

The server keeps a client TCP connection open after processing a request.

Multiple request/response exchanges may therefore occur over the same TCP connection.

For example:

    Client -> REQUEST
    Server -> RESPONSE 200

    Client -> REQUEST
    Server -> RESPONSE 200

The client does not need to create another TCP connection for the second request.

---

## 8. Unknown Frames

If the server receives a frame with an unknown frame type, it:

1. Reads the complete frame header.
2. Reads/discards exactly `Payload Length` bytes.
3. Does not process the frame as a request.
4. Continues waiting for the next frame on the same connection.

This allows future frame types to be ignored by older implementations.

---

## 9. Error Handling

Malformed requests that can still be safely processed as frames result in:

    400 Bad Request

If the requested file does not exist:

    404 Not Found

Internal file/server failures result in:

    500 Internal Server Error

Truncated or invalid frames that cannot be safely synchronized cause the connection to be closed.

---

## 10. Path Security

The server maps request paths relative to the configured web root.

For example:

    /index.html

maps to:

    ./www/index.html

Paths attempting to escape the web root, such as:

    /../etc/passwd

are rejected with:

    400 Bad Request

---

## 11. TCP I/O

TCP does not guarantee that one `send()` or `recv()` transfers the complete requested amount of data.

Therefore the implementation uses:

- `read_exact()` for complete reads
- `write_all()` for complete writes

Interrupted system calls (`EINTR`) are retried.

---

## 12. Maximum Payload

The maximum supported frame payload is:

    16 MiB

A frame advertising a payload larger than this limit is rejected.

---

## 13. Example Hex Header

A request frame begins with:

    4F 42 01 01 00 02 00 00
    00 00 00 00 00 00 00 00

This represents:

    Magic        = 4F 42
    Version      = 01
    Frame Type   = 01 (REQUEST)
    Flags        = 00
    Header Count = 02
    Payload Len  = 0
    Status       = 0
    Reserved     = 0

The actual payload length is filled by the frame serialization code before transmission.

---

## 14. Version 1 Rules

For protocol version 1:

- Magic must be `0x4F42`.
- Version must be `1`.
- Flags must be `0`.
- Reserved fields must be `0`.
- Payload length must not exceed 16 MiB.
- Request status code must be `0`.
- Header IDs must be between 1 and 10.
- Unknown frame types must be skipped safely.

## Annotated Hexdump

The following example shows one complete request and response exchanged
between `bcurl` and `observe`.

### Request

Frame header:

4F 42             Magic = 0x4F42 ("OB")
01                Version = 1
01                Frame type = REQUEST
00                Flags = 0
02                Header count = 2
00 00 00 24       Payload length = 36 bytes
00 00             Status code = 0
00 00             Reserved 1 = 0
00 00             Reserved 2 = 0

Request payload:

00 0B             Path length = 11
2F 69 6E 64 65 78 2E 68 74 6D 6C
                  Path = "/index.html"

04                Header ID = 4 (Host)
00 0E             Value length = 14
6C 6F 63 61 6C 68 6F 73 74 3A 39 30 30 30
                  Value = "localhost:9000"

07                Header ID = 7 (Accept)
00 03             Value length = 3
2A 2F 2A          Value = "*/*"


### Response

Frame header:

4F 42             Magic = 0x4F42 ("OB")
01                Version = 1
02                Frame type = RESPONSE
00                Flags = 0
04                Header count = 4
00 00 00 69       Payload length = 105 bytes
00 C8             Status code = 200
00 00             Reserved 1 = 0
00 00             Reserved 2 = 0

Response payload:

00 2C             Header block length = 44 bytes

01                Header ID = 1 (Content-Type)
00 09             Value length = 9
74 65 78 74 2F 68 74 6D 6C
                  Value = "text/html"

02                Header ID = 2 (Content-Length)
00 02             Value length = 2
35 39             Value = "59"

03                Header ID = 3 (Connection)
00 0A             Value length = 10
6B 65 65 70 2D 61 6C 69 76 65
                  Value = "keep-alive"

05                Header ID = 5 (Server)
00 0B             Value length = 11
6F 62 73 65 72 76 65 2F 31 2E 30
                  Value = "observe/1.0"

Remaining payload:
3C 68 74 6D 6C 3E 3C 62 6F 64 79 3E ...
                  File body = index.html