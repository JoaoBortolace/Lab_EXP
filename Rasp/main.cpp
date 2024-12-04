/*
 *  Rasp: programa envias as imagens para a Base (computador), e recebe
 *  os comandos vindos da base, para direcionar o carrinho
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Server.hpp"

/* -------- Variáveis Globais -------- */
std::mutex mutex;
std::condition_variable cv_motor;

Raspberry::Comando comando = Raspberry::Comando::NAO_SELECIONADO;

/* -------- Thread de controle dos motores -------- */
void controleMotor(std::atomic<bool>& run)
{
    // Inicializa os GPIOs da ponte H
    Raspberry::Motores::initPwm();

    while(run) {
        std::unique_lock<std::mutex> lock(mutex);

        // Espera receber um comando
        cv_motor.wait(lock);

        // Modo manual
        if (comando < Raspberry::Comando::AUTO_PARADO) {
            Raspberry::Motores::setDirAjustado(comando);
        }
        else { // Modo automático            
            double timeExe = 0.0;

            switch (comando) {
                case Raspberry::Comando::AUTO_180_ESQUERDA:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::GIRA_ESQUERDA);
                    timeExe = 0.9;
                    break;

                case Raspberry::Comando::AUTO_180_DIREITA:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::GIRA_DIREITA);
                    timeExe = 0.9;
                    break;

                case Raspberry::Comando::AUTO_90_ESQUERDA:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::GIRA_ESQUERDA);
                    timeExe = 0.48;
                    break;

                case Raspberry::Comando::AUTO_90_DIREITA:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::GIRA_DIREITA);
                    timeExe = 0.48;
                    break;

                case Raspberry::Comando::AUTO_FRENTE:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::FRENTE);
                    timeExe = 2;
                    break;

                default:
                    Raspberry::Motores::setDirAjustado(Raspberry::Comando::PARADO);
                    timeExe = 1.5;
                    break;
            }

            double timer = Raspberry::timeSinceEpoch();    
            while (Raspberry::timeSinceEpoch() - timer < timeExe) {
                ;;
            }
            
            Raspberry::Motores::setDirAjustado(Raspberry::Comando::PARADO);
        }
    }

    // Desliga os motores
    Raspberry::Motores::stopPWM();
}

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

    // Controle dos Motores
    std::atomic<bool> runMotor{true};
    std::thread motorThread(controleMotor, std::ref(runMotor));

    try {
        // Inicializa o servidor
        Server server(argv[1], 30);
        server.waitConnection();
       
        // Para armazenar as imagens que serão transmitidas
        Mat_<Raspberry::Cor> frameBuf;

        while(true) {
            // Transmite os quadros
            camera.read(frameBuf);
            server.sendImageCompactada(frameBuf);

            // Recebe o comando de ação
            {
                std::unique_lock<std::mutex> lock(mutex);
                server.receiveBytes(sizeof(comando), (Raspberry::Byte *) &comando); 
            }         

            // Acorda a thread para executar o comando
            cv_motor.notify_one();  
        }
    }
    catch (const std::exception& e) {
        Raspberry::print(e.what());
    }

    // Para a thread dos motores
    runMotor = false;
    cv_motor.notify_one();
    motorThread.join();
    
    return 0;
}