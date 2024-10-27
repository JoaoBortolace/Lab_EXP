#include <opencv2/opencv.hpp>
#include "Raspberry.hpp"
#include "Client.hpp"

#define CAMERA_VIDEO        0
#define CAMERA_FRAME_WIDTH  320
#define CAMERA_FRAME_HEIGHT 240

int main(int argc, char *argv[])
{
    if (argc < 3) {
        Raspberry::erro("Poucos argumentos.");
    }

    // Inicia a camera e configura a camera
    VideoCapture camera(CAMERA_VIDEO);
    if (!camera.isOpened()) {
        Raspberry::erro("Falha ao abrir a camera.");
    }

    camera.set(CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
    camera.set(CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);
    Mat_<Raspberry::Cor> frameBuf;

    Raspberry::Teclado comando = Raspberry::Teclado::NAO_SELECIONADO;
    Raspberry::motorInit();
    
    try {
        // Inicializa o servidor
        Client client(argv[1], argv[2]);
        client.waitConnection();

        // Transmite os quadros
        while(true) {
            camera.read(frameBuf);
            client.sendImageCompactada(frameBuf);

            // Recebe o comando
            client.receiveBytes(sizeof(Raspberry::Teclado),(Raspberry::Byte *) &comando);

            // Passa o comando para o motor
            Raspberry::motorSetDir(comando);
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }

    // Desliga os motores
    Raspberry::motorSetDir(Raspberry::Teclado::NAO_FAZ_NADA);
    
    return 0;
}