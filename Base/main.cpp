/*
 *  Base: programa que recebe as imagens provindas da Raspberry Pi 3b
 *  realiza processamento nestas imagens e retorna comandos para a Pi
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Client.hpp"

/* -------- Defines -------- */
#define CAMERA_FRAME_HEIGHT     240
#define CAMERA_FRAME_WIDTH      320
#define TECLADO_HEIGHT          CAMERA_FRAME_HEIGHT
#define TECLADO_WIDTH           TECLADO_HEIGHT      
#define BUTTON_WIDTH            80U
#define BUTTON_HEIGHT           80U

/* -------- Variáveis Globais -------- */
static Raspberry::Teclado comando = Raspberry::Teclado::NAO_SELECIONADO;

/* -------- Callbacks -------- */
void mouse_callback(int event, int x, int y, int flags, void *usedata)
{
    (void)flags;
    (void)usedata;
    
    if (event == EVENT_LBUTTONDOWN) {
        int col = x / BUTTON_WIDTH;
        int row = y / BUTTON_HEIGHT;
        
        using namespace Raspberry;
        // Primeiro seleciona qual coluna foi escolhida
        switch (col) {
            case 0:
                // Depois seleciona qual linha foi escolhida
                switch (row) {
                    case 0:
                        comando = Teclado::DIAGONAL_FRENTE_ESQUERDA;
                        break;
                    case 1:
                        comando = Teclado::GIRA_ESQUERDA;
                        break;
                    case 2:
                        comando = Teclado::DIAGONAL_ATRAS_ESQUERDA;
                        break;
                }
                break;
            case 1:
                switch (row) {
                    case 0:
                        comando = Teclado::FRENTE;
                        break;
                    case 1:
                        comando = Teclado::NAO_FAZ_NADA;
                        break;
                    case 2:
                        comando = Teclado::ATRAS;
                        break;
                }
                break;
            case 2:
                switch (row) {
                    case 0:
                        comando = Teclado::DIAGONAL_FRENTE_DIREITA;
                        break;
                    case 1:
                        comando = Teclado::GIRA_DIREITA;
                        break;
                    case 2:
                        comando = Teclado::DIAGONAL_ATRAS_DIREITA;
                        break;
                }
                break;
            default:
                comando = Teclado::NAO_SELECIONADO;
                break;
        }
    }
    else if (event == EVENT_LBUTTONUP) {
        comando = Raspberry::Teclado::NAO_SELECIONADO;
    }
}

/* -------- Main -------- */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        Raspberry::erro("Poucos argumentos.");
    }

    // Configura a janela aonde será transmitido o vídeo
    namedWindow("RaspCam", WINDOW_AUTOSIZE);
    setMouseCallback("RaspCam", mouse_callback);
    Mat_<Raspberry::Cor> frameBuf;
    
    // Teclado Virtual
    Mat_<Raspberry::Cor> teclado(TECLADO_WIDTH, TECLADO_HEIGHT, Paleta::blue01);
    
    line(teclado, Point(0, 80), Point(240, 80), Paleta::grey, 2);
    line(teclado, Point(0, 160), Point(240, 160), Paleta::grey, 2);    
    line(teclado, Point(80, 0), Point(80, 240), Paleta::grey, 2);    
    line(teclado, Point(160, 0), Point(160, 240), Paleta::grey, 2);    

    arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::blue03, 3);
    
    try {
        // Conecta à Raspberry
        Client client(argv[1], argv[2]);
        client.waitConnection();

        // Recebe os quadros e envia o comando
        while (true) {
            client.receiveImageCompactada(frameBuf);
            client.sendBytes(sizeof(Raspberry::Teclado),(Raspberry::Byte *) &comando);  

            // Coloca o teclado 
            hconcat(teclado, frameBuf, frameBuf);  

            // Exibi o quadro
            imshow("RaspCam", frameBuf);
            if ((waitKey(1) & 0xFF)  == 27) { // Esc
                break;
            }
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}