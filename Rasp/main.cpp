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
        Raspberry::Comando comando = Raspberry::Comando::NAO_SELECIONADO;
        Raspberry::Controle controle = Raspberry::Controle::MANUAL;

        Raspberry::motorInitPwm();

        // Inicia a camera e configura a camera
        VideoCapture camera(CAMERA_VIDEO);
        if (!camera.isOpened()) {
            Raspberry::erro("Falha ao abrir a camera.");
        }

        camera.set(CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
        camera.set(CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);
        
        // Para armazenar as imagens que serão transmitidas
        Mat_<Raspberry::Cor> frameBuf;

        // Transmite os quadros
        while(true) {
            camera.read(frameBuf);
            server.sendImageCompactada(frameBuf);

            // Recebe o comando
            server.receiveBytes(sizeof(Raspberry::Comando), (Raspberry::Byte *) &comando);

             // Alterna entre o controle manual ou automático
            if (comando == Raspberry::Comando::ALTERNA_MODO) {
                controle = static_cast<Raspberry::Controle>(~controle & 1);
            }

            // Modos de operação
            if (controle == Raspberry::Controle::MANUAL) {
                // Passa o comando para o motor
                Raspberry::Motores::motorSetDirPwm(comando, PWM_MAX);
            }
            else { // Modo Autômato
                int velocidades[4];
                server.receiveBytes(sizeof(velocidades), (Raspberry::Byte *) velocidades);
                Raspberry::Motores::motorSetVel(velocidades);
            }
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }

    // Desliga os motores
    Raspberry::Motores::motorStop();
    
    return 0;
}