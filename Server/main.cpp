#include "Raspberry.hpp"
#include "Server.hpp"

#define BUFFER_DEPTH    32

int main(int argc, char *argv[])
{
    Raspberry::Byte rxBuffer[BUFFER_DEPTH] = {0};
    Raspberry::Byte txBuffer[BUFFER_DEPTH];
    
    if (argc < 2) {
        Raspberry::erro("Poucos argumentos.");
    }

    try {
        Server server(argv[1], 30);
        server.waitConnection();

        // Transmite uma sequências de bytes
        std::strcpy((char*) txBuffer, "Server");
        server.sendBytes(sizeof(txBuffer), txBuffer);

        // Recebe uma sequências de bytes
        server.receiveBytes(sizeof(rxBuffer), rxBuffer);
        std::cout << rxBuffer << std::endl;  

        // Transmite um inteiro
        server.sendUInt(333); 

        // Recebe um inteiro
        uint32_t rxInt;
        server.receiveUInt(rxInt);
        std::cout << rxInt << std::endl;  

        // Transmite um vector
        std::vector<Raspberry::Byte> vecB;
        vecB.assign(100000, 111);
        server.sendVectorByte(vecB);

        // Recebe um vector
        server.receiveVectorByte(vecB);

        if (Raspberry::testaVb(vecB, 222)) {
            Raspberry::print("Primeiro vector correto.");
        }
        else {
            Raspberry::erro("Primeiro vector errado.");
        }

        // Transmite um vector
        vecB.assign(10000, 10);
        server.sendVectorByte(vecB);

        // Recebe uma imagem
        Mat_<Raspberry::Cor> img;
        server.receiveImage(img);
        imshow("Server", img);
        waitKey(0);

        // Envia um imagem compactada
        server.sendImageCompactada(img);
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}