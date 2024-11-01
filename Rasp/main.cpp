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
    
    try {
        // Inicializa o servidor
        Server server(argv[1], 30);
        server.waitConnection();

        // Controle dos Motores
        Raspberry::Teclado comando = Raspberry::Teclado::NAO_SELECIONADO;
        Raspberry::motorInit();

        // Inicia a camera e configura a camera
        VideoCapture camera(CAMERA_VIDEO);
        if (!camera.isOpened()) {
            Raspberry::erro("Falha ao abrir a camera.");
        }

        camera.set(CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
        camera.set(CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);
        Mat_<Raspberry::Cor> frameBuf;

        // Transmite os quadros
        while(true) {
            camera.read(frameBuf);
            server.sendImageCompactada(frameBuf);

            // Recebe o comando
            server.receiveBytes(sizeof(Raspberry::Teclado),(Raspberry::Byte *) &comando);

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