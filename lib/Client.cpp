#include "Client.hpp"

/*
 * Construi um client TCP-IP IPv4, caso não seja possivel joga uma excessão
 */
Client::Client(const char* serverName, const char* port)
{
    // Obtem um socket
    transferSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (transferSocket == SOCKET_ERROR) {
        throw std::runtime_error("Client: Erro ao obter o socket! Código de erro: " + std::to_string(errno));       
    }

    // Obtem o endereço do server
    struct hostent *server = gethostbyname(serverName);
    if (server == NULL) {
        throw std::runtime_error("Client: Erro ao obter o server! Código de erro: " + std::to_string(errno));
    }

    // Configura o struct com os dados do servidor
    memset(&transferAddr, 0, sizeof(transferAddr));
    transferAddr.sin_family = AF_INET;
    memcpy(&transferAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    transferAddr.sin_port = htons(atoi(port));
}

Client::~Client()
{
    if (transferSocket != SOCKET_ERROR) {
        close(transferSocket);
    }
}

/*
 * Espera até conectar-se ao servidor correspondete ao endereço e porta passados no construtor, caso não seja possivel, joga um excessão
 */
void Client::waitConnection()
{
    // Conecta ao servidor
    if (connect(transferSocket, (struct sockaddr *)&transferAddr, sizeof(transferAddr)) < 0) {
        throw std::runtime_error("Client: Erro ao conectar-se ao servidor! Código de erro: " + std::to_string(errno));
    }
}

/*
 * Transmite um buffer de bytes, caso não seja possivel, joga um excessão
 */
void Client::sendBytes(uint32_t numBytes, const Raspberry::Byte* txBuffer)
{
    size_t totalSend = 0;
    while (totalSend < static_cast<size_t>(numBytes)) {
        size_t chunk_size = std::min(CHUNK_SIZE, numBytes - totalSend);
        ssize_t numSend = write(transferSocket, &txBuffer[totalSend], chunk_size);

        // Verifica se não ouve algum erro na transmissão
        if (numSend == SOCKET_ERROR) {
            throw std::runtime_error("Client: Erro ao transmitir os dados! Código de erro: " + std::to_string(errno));                  
        }

        totalSend += numSend;
    }
}

/*
 * Recebe um buffer de bytes, caso não seja possivel, joga um excessão
 */
void Client::receiveBytes(uint32_t numBytes, Raspberry::Byte* rxBuffer)
{
    size_t totalRecv = 0;
    while (totalRecv < static_cast<size_t>(numBytes)) {
        size_t chunk_size = std::min(CHUNK_SIZE, numBytes - totalRecv);
        ssize_t numRecv = read(transferSocket, &rxBuffer[totalRecv], chunk_size);

        if (numRecv == 0 && numBytes > 0) {
            Raspberry::print("Client: Timeout para receber.");
            break;
        }
        else if (numRecv < 0) {
            throw std::runtime_error("Client: Erro ao receber os dados! Código de erro: " + std::to_string(errno));
        }  

        totalRecv += numRecv;
    }
}