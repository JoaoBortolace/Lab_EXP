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
    Raspberry::Motores::init();
    
    while(run) {
        std::unique_lock<std::mutex> lock(mutex);

        // Espera receber um comando
        cv_motor.wait(lock);

        // Modo manual
        if (comando < Raspberry::Comando::AUTO_PARADO) {
            Raspberry::Motores::setDir(comando);
        }
        else { // Modo automático            
            switch (comando) {
                case Raspberry::Comando::AUTO_180_ESQUERDA:
                    Raspberry::Motores::setDir(Raspberry::Comando::GIRA_ESQUERDA);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
                    break;
                case Raspberry::Comando::AUTO_180_DIREITA:
                    Raspberry::Motores::setDir(Raspberry::Comando::GIRA_DIREITA);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
                    break;
                case Raspberry::Comando::AUTO_90_ESQUERDA:
                    Raspberry::Motores::setDir(Raspberry::Comando::GIRA_ESQUERDA);
                    std::this_thread::sleep_for(std::chrono::milliseconds(600));
                    break;
                case Raspberry::Comando::AUTO_90_DIREITA:
                    Raspberry::Motores::setDir(Raspberry::Comando::GIRA_DIREITA);
                    std::this_thread::sleep_for(std::chrono::milliseconds(600));
                    break;
                default:
                    Raspberry::Motores::setDir(Raspberry::Comando::PARADO);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    break;
            }
            
            Raspberry::Motores::setDir(Raspberry::Comando::PARADO);
        }
    }

    // Desliga os motores
    Raspberry::Motores::stop();
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
            cv_motor.notify_all();  
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