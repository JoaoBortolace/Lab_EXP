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
int pwmMotor[4] = {0};

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
            Raspberry::Motores::setVelPWM(pwmMotor);
        }
        else { // Modo automático            
            auto executarAcao = [&](Raspberry::Comando dir, int duracao) {
                auto start = std::chrono::steady_clock::now();
                
                Raspberry::Motores::setDirPwm(dir, 100);
                
                while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(duracao)) {
                    if (!run) {
                        break;  // Interrompe em caso de finalização
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            };

            switch (comando) {
                case Raspberry::Comando::AUTO_180_ESQUERDA:
                    executarAcao(Raspberry::Comando::GIRA_ESQUERDA, 900);
                    break;
                case Raspberry::Comando::AUTO_180_DIREITA:
                    executarAcao(Raspberry::Comando::GIRA_DIREITA, 900);
                    break;
                case Raspberry::Comando::AUTO_90_ESQUERDA:
                    executarAcao(Raspberry::Comando::GIRA_ESQUERDA, 500);
                    break;
                case Raspberry::Comando::AUTO_90_DIREITA:
                    executarAcao(Raspberry::Comando::GIRA_DIREITA, 500);
                    break;
                case Raspberry::Comando::AUTO_FRENTE:
                    executarAcao(Raspberry::Comando::FRENTE, 1500);
                    break;
                default:
                    executarAcao(Raspberry::Comando::PARADO, 1500);
                    break;
            }

            Raspberry::Motores::setDirPwm(Raspberry::Comando::PARADO, 100);
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
                server.receiveBytes(sizeof(pwmMotor), (Raspberry::Byte *) pwmMotor); 
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