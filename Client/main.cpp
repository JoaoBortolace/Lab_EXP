#include "Raspberry.hpp"
#include "Client.hpp"

#define BUFFER_DEPTH    32

int main(int argc, char *argv[])
{
    Raspberry::Byte rxBuffer[BUFFER_DEPTH] = {0};
    Raspberry::Byte txBuffer[BUFFER_DEPTH];
    
    if (argc < 3) {
        Raspberry::erro("Poucos argumentos.");
    }

    try {
        Client client(argv[1], argv[2]);
        client.waitConnection();

        // Recebe uma sequências de bytes
        client.receiveBytes(sizeof(rxBuffer), rxBuffer);
        std::cout << rxBuffer << std::endl;  

        // Transmite uma sequências de bytes
        std::strcpy((char*) txBuffer, "Client");
        client.sendBytes(sizeof(txBuffer), txBuffer);

        // Recebe um inteiro
        uint32_t rxInt;
        client.receiveUInt(rxInt);
        std::cout << rxInt << std::endl;  

        // Transmite um inteiro
        client.sendUInt(44); 

        // Recebe um vector
        std::vector<Raspberry::Byte> vecB;
        client.receiveVectorByte(vecB);

        if (Raspberry::testaVb(vecB, 111)) {
            Raspberry::print("Primeiro vector correto.");
        }
        else {
            Raspberry::erro("Primeiro vector errado.");
        }

        // Transmite um vector
        vecB.assign(100000, 222);
        client.sendVectorByte(vecB);

        // Recebe um vector
        client.receiveVectorByte(vecB);

        if (Raspberry::testaVb(vecB, 10)) {
            Raspberry::print("Segundo vector correto.");
        }
        else {
            Raspberry::erro("Segundo vector errado.");
        }

        // Enviado um imagem
        Mat_<Raspberry::Cor> img = cv::imread("../img.png", 1);
        client.sendImage(img);

        // Recebe uma imagem compactada
        client.receiveImageCompactada(img);
        imshow("Client", img);
        waitKey(0);
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}