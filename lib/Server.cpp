#include "Server.hpp"

Server::Server(const char* port, time_t timeout)
{
    // Obtem um socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == SOCKET_ERROR) {
        throw std::runtime_error("Server: Erro ao obter o socket! Código de erro: " + std::to_string(errno));                
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(port));

    // Timeout para receber dados
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // Configura o reuso do endereço e porta, para evitar problema com multiplas conexões
    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        throw std::runtime_error("Server: Erro ao definir o reuso do endereço! Código de erro: " + std::to_string(errno));                
    }

#ifdef SO_REUSEPORT
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
        throw std::runtime_error("Server: Erro ao definir o reuso da porta! Código de erro: " + std::to_string(errno));                  
    }
#endif

    // Vincular o socket para um Ip e porta
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Server: Erro ao vincular o socket! Código de erro: " + std::to_string(errno));                
    }

    // Conecta ao socket
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        throw std::runtime_error("Server: Erro ao ouvir a porta! Código de erro: " + std::to_string(errno));                
    }
}

Server::~Server()
{
    if (transferSocket != SOCKET_ERROR) {
        close(transferSocket);
    }

    if (serverSocket != SOCKET_ERROR) {
        close(serverSocket);
    }
}

void Server::waitConnection()
{
    socklen_t clientLen = sizeof(transferAddr);
    transferSocket = accept(serverSocket, (struct sockaddr *) &transferAddr, &clientLen);

    if (transferSocket == SOCKET_ERROR) {
        throw std::runtime_error("Server: Erro ao aceitar a conexão! Código de erro: " + std::to_string(errno));                  
    }
}

void Server::sendBytes(uint32_t numBytes, const Raspberry::Byte* txBuffer)
{
    size_t totalSend = 0;
    while (totalSend < static_cast<size_t>(numBytes)) {
        size_t chunk_size = std::min(CHUNK_SIZE, numBytes - totalSend);
        ssize_t numSend = write(transferSocket, &txBuffer[totalSend], chunk_size);

        // Verifica se não ouve algum erro na transmissão
        if (numSend == SOCKET_ERROR) {
            throw std::runtime_error("Server: Erro ao transmitir os dados! Código de erro: " + std::to_string(errno));                  
        }

        totalSend += numSend;
    }
}

void Server::receiveBytes(uint32_t numBytes, Raspberry::Byte* rxBuffer)
{
    size_t totalRecv = 0;
    while (totalRecv < static_cast<size_t>(numBytes)) {
        size_t chunk_size = std::min(CHUNK_SIZE, numBytes - totalRecv);
        ssize_t numRecv = read(transferSocket, &rxBuffer[totalRecv], chunk_size);

        // Verifica se houve timeout
        if (numRecv == SOCKET_ERROR) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                Raspberry::print("Server: Timeout para receber.");
                break;
            } else {
                throw std::runtime_error("Server: Erro ao receber os dados! Código de erro: " + std::to_string(errno));                
            }
        } else if (numRecv == 0) {
            Raspberry::print("Server: Conexão fechada pelo cliente.");
            break;
        }   

        totalRecv += numRecv;
    }
}
