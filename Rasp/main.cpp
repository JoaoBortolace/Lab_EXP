/*
 *  Rasp: programa envias as imagens para a Base (computador), e recebe
 *  os comandos vindos da base, para direcionar o carrinho
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Server.hpp"

/* -------- Main -------- */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        Raspberry::erro("Poucos argumentos.");
    }
    
    // Inicia a camera e configura a camera
    VideoCapture camera(CAMERA_VIDEO);
    if (!camera.isOpened()) {
        Raspberry::erro("Falha ao abrir a camera.");
    }

    camera.set(CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
    camera.set(CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);

    try {
        // Inicializa o servidor
        Server server(argv[1], 30);
        server.waitConnection();
       
        // Controle dos Motores
        Raspberry::Motores::motorInitPwm();
        int velocidades[4] = {0};

        // Para armazenar as imagens que serão transmitidas
        Mat_<Raspberry::Cor> frameBuf;

        // Transmite os quadros
        while(true) {
            camera.read(frameBuf);
            server.sendImageCompactada(frameBuf);

            // Recebe e aplica os valores de velocidades dos PWMs
            server.receiveBytes(sizeof(velocidades), (Raspberry::Byte *) &velocidades);

            Raspberry::Motores::motorSetVel(velocidades);
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }

    // Desliga os motores
    Raspberry::Motores::motorStop();
    
    return 0;
}