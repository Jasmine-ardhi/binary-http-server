#include "protocol.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>

bool get_safe_file_path(
    const std::string& root_directory,
    const std::string& request_path,
    std::filesystem::path& result
) {
    if (request_path.empty()) {
        return false;
    }

    // Request paths must start with /
    if (request_path[0] != '/') {
        return false;
    }

    // Convert the URL path into a relative filesystem path.
    std::string relative_path = request_path.substr(1);

    std::filesystem::path root =
        std::filesystem::weakly_canonical(root_directory);

    std::filesystem::path requested =
        std::filesystem::weakly_canonical(
            root / relative_path
        );

    // Make sure requested is inside root.
    auto [root_it, requested_it] =
        std::mismatch(
            root.begin(),
            root.end(),
            requested.begin(),
            requested.end()
        );

    if (root_it != root.end()) {
        return false;
    }

    result = requested;

    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./observe <www-root> <port>\n";
        return 1;
    }

    const char* root_directory = argv[1];
    int port = std::stoi(argv[2]);

    std::cout << "Serving files from: "
              << root_directory << "\n";

    std::cout << "Starting server on port: "
              << port << "\n";

    // Create TCP socket
    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0) {
        std::cerr << "socket() failed: "
                  << std::strerror(errno) << "\n";
        return 1;
    }

    // Allow quick restart after server shutdown
    int reuse = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)
        ) < 0) {

        std::cerr << "setsockopt() failed: "
                  << std::strerror(errno) << "\n";

        close(server_fd);
        return 1;
    }

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(
        static_cast<std::uint16_t>(port)
    );

    // Bind socket to the requested port
    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)
        ) < 0) {

        std::cerr << "bind() failed: "
                  << std::strerror(errno) << "\n";

        close(server_fd);
        return 1;
    }

    // Start listening
    if (listen(server_fd, 10) < 0) {
        std::cerr << "listen() failed: "
                  << std::strerror(errno) << "\n";

        close(server_fd);
        return 1;
    }

    std::cout << "Server is listening on port "
          << port << "\n";

sockaddr_in client_address{};
socklen_t client_address_length =
    sizeof(client_address);


while (true) {

std::cout << "Waiting for a client...\n";

int client_fd = accept(
    server_fd,
    reinterpret_cast<sockaddr*>(&client_address),
    &client_address_length
);

if (client_fd < 0) {
    std::cerr << "accept() failed: "
              << std::strerror(errno) << "\n";

    close(server_fd);
    return 1;
}

std::cout << "Client connected!\n";

while (true) {
    Frame frame;

    bool received = receive_frame(
        client_fd,
        frame
    );

    if (!received) {
        std::cout << "Client disconnected "
                  << "or sent an invalid frame.\n";
        break;
    }

    std::cout << "Received frame: "
              << "type="
              << static_cast<int>(frame.header.frame_type)
              << ", payload="
              << frame.header.payload_length
              << " bytes\n";

    if (frame.header.frame_type != FRAME_REQUEST) {
        std::cout << "Ignoring non-request frame.\n";
        continue;
    }

    if (frame.header.status_code != 0) {
        std::cout << "Invalid request status code.\n";
    
        Response response;
    
        response.status_code = 400;
    
        response.headers.push_back(
            {HEADER_CONTENT_TYPE, "text/plain"}
        );
    
        response.headers.push_back(
            {HEADER_CONTENT_LENGTH, "11"}
        );
    
        response.headers.push_back(
            {HEADER_CONNECTION, "keep-alive"}
        );
    
        response.body = {
            'B', 'a', 'd', ' ', 'R', 'e', 'q', 'u', 'e', 's', 't'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 400 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
        response_frame.header.header_count =
            static_cast<std::uint8_t>(response.headers.size());
        response_frame.header.status_code = 400;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
    
        response_frame.payload = response_payload;
    
        if (!send_frame(client_fd, response_frame)) {
            std::cerr << "Failed to send 400 response.\n";
            break;
        }
    
        std::cout << "400 response sent.\n";
    
        continue;
    }

    Request request;

    bool decoded = decode_request(
        frame.payload.data(),
        frame.payload.size(),
        request
    );

    if (decoded &&
        request.headers.size() != frame.header.header_count) {
    
        std::cout << "Request header count mismatch.\n";
    
        Response response;
        response.status_code = 400;
        response.headers.push_back({
            HEADER_CONTENT_TYPE,
            "text/plain"
        });
        response.headers.push_back({
            HEADER_CONTENT_LENGTH,
            "11"
        });
        response.headers.push_back({
            HEADER_CONNECTION,
            "keep-alive"
        });
    
        response.body = {
            'B','a','d',' ','R','e','q','u','e','s','t'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 400 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
        response_frame.header.header_count =
            static_cast<std::uint8_t>(
                response.headers.size()
            );
        response_frame.header.status_code = 400;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
        response_frame.payload = response_payload;
    
        if (!send_frame(
                client_fd,
                response_frame
            )) {
    
            std::cerr << "Failed to send 400 response.\n";
            break;
        }
    
        std::cout << "400 response sent.\n";
        continue;
    }

    if (!decoded) {
        std::cout << "Malformed request payload.\n";
    
        Response response;
    
        response.status_code = 400;
    
        response.headers.push_back(
            {HEADER_CONTENT_TYPE, "text/plain"}
        );
    
        response.headers.push_back(
            {HEADER_CONTENT_LENGTH, "11"}
        );
    
        response.headers.push_back(
            {HEADER_CONNECTION, "keep-alive"}
        );
    
        response.body = {
            'B', 'a', 'd', ' ', 'R', 'e', 'q', 'u', 'e', 's', 't'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 400 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
        response_frame.header.header_count =
            static_cast<std::uint8_t>(response.headers.size());
        response_frame.header.status_code = 400;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
    
        response_frame.payload = response_payload;
    
        if (!send_frame(client_fd, response_frame)) {
            std::cerr << "Failed to send 400 response.\n";
            break;
        }
    
        std::cout << "400 response sent.\n";
    
        continue;
    }

    std::cout << "Requested path: "
          << request.path
          << "\n";

std::filesystem::path file_path;

if (!get_safe_file_path(
    root_directory,
    request.path,
    file_path
)) {

std::cout << "Unsafe request path rejected.\n";

Response response;

response.status_code = 400;

response.headers.push_back(
    {HEADER_CONTENT_TYPE, "text/plain"}
);

response.headers.push_back(
    {HEADER_CONTENT_LENGTH, "11"}
);

response.headers.push_back(
    {HEADER_CONNECTION, "keep-alive"}
);

response.body = {
    'B', 'a', 'd', ' ', 'R', 'e', 'q', 'u', 'e', 's', 't'
};

std::vector<std::uint8_t> response_payload;

if (!encode_response(
        response,
        response_payload
    )) {

    std::cerr << "Failed to encode 400 response.\n";
    break;
}

Frame response_frame;

response_frame.header.magic = MAGIC;
response_frame.header.version = VERSION;
response_frame.header.frame_type = FRAME_RESPONSE;
response_frame.header.flags = 0;
response_frame.header.header_count =
    static_cast<std::uint8_t>(response.headers.size());
response_frame.header.status_code = 400;
response_frame.header.reserved1 = 0;
response_frame.header.reserved2 = 0;

response_frame.payload = response_payload;

if (!send_frame(client_fd, response_frame)) {
    std::cerr << "Failed to send 400 response.\n";
    break;
}

std::cout << "400 response sent.\n";

continue;
}

std::cout << "Resolved file path: "
          << file_path
          << "\n";

          if (!std::filesystem::exists(file_path) ||
    !std::filesystem::is_regular_file(file_path)) {

    std::cout << "File not found: "
              << file_path
              << "\n";

    Response response;

    response.status_code = 404;

    response.headers.push_back(
        {HEADER_CONTENT_TYPE, "text/plain"}
    );

    response.headers.push_back(
        {HEADER_CONTENT_LENGTH, "9"}
    );

    response.headers.push_back(
        {HEADER_CONNECTION, "keep-alive"}
    );

    response.body = {
        'N', 'o', 't', ' ', 'F', 'o', 'u', 'n', 'd'
    };

    std::vector<std::uint8_t> response_payload;

    if (!encode_response(
            response,
            response_payload
        )) {

        std::cerr << "Failed to encode 404 response.\n";
        break;
    }

    Frame response_frame;

    response_frame.header.magic = MAGIC;
    response_frame.header.version = VERSION;
    response_frame.header.frame_type = FRAME_RESPONSE;
    response_frame.header.flags = 0;
    response_frame.header.header_count =
        static_cast<std::uint8_t>(response.headers.size());
    response_frame.header.status_code = 404;
    response_frame.header.reserved1 = 0;
    response_frame.header.reserved2 = 0;

    response_frame.payload = response_payload;

    if (!send_frame(client_fd, response_frame)) {
        std::cerr << "Failed to send 404 response.\n";
        break;
    }

    std::cout << "404 response sent.\n";

    continue;
}
      
      std::cout << "File exists!\n";

      std::ifstream file(
        file_path,
        std::ios::binary | std::ios::ate
    );
    
    if (!file) {
        std::cerr << "Failed to open file.\n";
    
        Response response;
        response.status_code = 500;
    
        response.headers.push_back({
            HEADER_CONTENT_TYPE,
            "text/plain"
        });
    
        response.headers.push_back({
            HEADER_CONTENT_LENGTH,
            "21"
        });
    
        response.headers.push_back({
            HEADER_CONNECTION,
            "keep-alive"
        });
    
        response.body = {
            'I','n','t','e','r','n','a','l',' ',
            'S','e','r','v','e','r',' ',
            'E','r','r','o','r'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 500 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
    
        response_frame.header.header_count =
            static_cast<std::uint8_t>(
                response.headers.size()
            );
    
        response_frame.header.status_code = 500;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
    
        response_frame.payload = response_payload;
    
        if (!send_frame(
                client_fd,
                response_frame
            )) {
    
            std::cerr << "Failed to send 500 response.\n";
            break;
        }
    
        std::cout << "500 response sent.\n";
        continue;
    }
    
    std::streamsize file_size = file.tellg();
    
    if (file_size < 0) {
        std::cerr << "Failed to determine file size.\n";
    
        Response response;
        response.status_code = 500;
    
        response.headers.push_back({
            HEADER_CONTENT_TYPE,
            "text/plain"
        });
    
        response.headers.push_back({
            HEADER_CONTENT_LENGTH,
            "21"
        });
    
        response.headers.push_back({
            HEADER_CONNECTION,
            "keep-alive"
        });
    
        response.body = {
            'I','n','t','e','r','n','a','l',' ',
            'S','e','r','v','e','r',' ',
            'E','r','r','o','r'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 500 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
    
        response_frame.header.header_count =
            static_cast<std::uint8_t>(
                response.headers.size()
            );
    
        response_frame.header.status_code = 500;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
    
        response_frame.payload = response_payload;
    
        if (!send_frame(
                client_fd,
                response_frame
            )) {
    
            std::cerr << "Failed to send 500 response.\n";
            break;
        }
    
        std::cout << "500 response sent.\n";
        continue;
    }
    
    file.seekg(0, std::ios::beg);
    
    std::vector<std::uint8_t> body(
        static_cast<std::size_t>(file_size)
    );
    
    if (file_size > 0 &&
        !file.read(
            reinterpret_cast<char*>(body.data()),
            file_size
        )) {
    
        std::cerr << "Failed to read file.\n";
    
        Response response;
        response.status_code = 500;
    
        response.headers.push_back({
            HEADER_CONTENT_TYPE,
            "text/plain"
        });
    
        response.headers.push_back({
            HEADER_CONTENT_LENGTH,
            "21"
        });
    
        response.headers.push_back({
            HEADER_CONNECTION,
            "keep-alive"
        });
    
        response.body = {
            'I','n','t','e','r','n','a','l',' ',
            'S','e','r','v','e','r',' ',
            'E','r','r','o','r'
        };
    
        std::vector<std::uint8_t> response_payload;
    
        if (!encode_response(
                response,
                response_payload
            )) {
    
            std::cerr << "Failed to encode 500 response.\n";
            break;
        }
    
        Frame response_frame;
    
        response_frame.header.magic = MAGIC;
        response_frame.header.version = VERSION;
        response_frame.header.frame_type = FRAME_RESPONSE;
        response_frame.header.flags = 0;
    
        response_frame.header.header_count =
            static_cast<std::uint8_t>(
                response.headers.size()
            );
    
        response_frame.header.status_code = 500;
        response_frame.header.reserved1 = 0;
        response_frame.header.reserved2 = 0;
    
        response_frame.payload = response_payload;
    
        if (!send_frame(
                client_fd,
                response_frame
            )) {
    
            std::cerr << "Failed to send 500 response.\n";
            break;
        }
    
        std::cout << "500 response sent.\n";
        continue;
    }
    
    std::cout << "Read "
              << body.size()
              << " bytes from file.\n";
              Response response;

              response.status_code = 200;
              
              response.headers.push_back(
                  {HEADER_CONTENT_TYPE, "text/html"}
              );
              
              response.headers.push_back(
                  {HEADER_CONTENT_LENGTH, std::to_string(body.size())}
              );
              
              response.headers.push_back(
                  {HEADER_CONNECTION, "keep-alive"}
              );
              
              response.headers.push_back(
                  {HEADER_SERVER, "observe/1.0"}
              );
              
              response.body = body;
              
              std::vector<std::uint8_t> response_payload;
              
              if (!encode_response(response, response_payload)) {
                  std::cout << "Failed to encode response.\n";
                  continue;
              }
              
              Frame response_frame;
              
              response_frame.header.magic = MAGIC;
              response_frame.header.version = VERSION;
              response_frame.header.frame_type = FRAME_RESPONSE;
              response_frame.header.flags = 0;
              response_frame.header.header_count =
                  static_cast<std::uint8_t>(response.headers.size());
              response_frame.header.status_code = response.status_code;
              response_frame.header.reserved1 = 0;
              response_frame.header.reserved2 = 0;
              
              response_frame.payload = response_payload;
              
              if (!send_frame(client_fd, response_frame)) {
                  std::cout << "Failed to send response.\n";
                  break;
              }
              
              std::cout << "200 response sent.\n";          

std::cout << "Request headers: "
          << request.headers.size()
          << "\n";

    for (const Header& header : request.headers) {
        std::cout << "  Header ID "
                  << static_cast<int>(header.id)
                  << ": "
                  << header.value
                  << "\n";
    }
}

close(client_fd);
}
close(server_fd);

return 0;
}