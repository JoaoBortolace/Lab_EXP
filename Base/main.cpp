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
static Mat_<Raspberry::Cor> teclado(TECLADO_WIDTH, TECLADO_HEIGHT, Paleta::blue01);

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
                        arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::red, 2.5);
                        break;
                    case 1:
                        comando = Teclado::GIRA_ESQUERDA;
                        arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::red, 3);
                        break;
                    case 2:
                        comando = Teclado::DIAGONAL_ATRAS_ESQUERDA;
                        arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::red, 2.5);    
                        break;
                }
                break;
            case 1:
                switch (row) {
                    case 0:
                        comando = Teclado::FRENTE;
                        arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::red, 3);
                        break;
                    case 1:
                        comando = Teclado::NAO_FAZ_NADA;
                        rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::grey, -1);
                        break;
                    case 2:
                        comando = Teclado::ATRAS;
                        arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::red, 3);
                        break;
                }
                break;
            case 2:
                switch (row) {
                    case 0:
                        comando = Teclado::DIAGONAL_FRENTE_DIREITA;
                        arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::red, 2.5);
                        break;
                    case 1:
                        comando = Teclado::GIRA_DIREITA;
                        arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::red, 3);
                        break;
                    case 2:
                        comando = Teclado::DIAGONAL_ATRAS_DIREITA;
                        arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::red, 2.5);
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
        arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::blue03, 3);
        arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::blue03, 3);
        arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::blue03, 3);
        arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::blue03, 3);
        arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::blue03, 2.5);
        arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::blue03, 2.5);    
        arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::blue03, 2.5);
        arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::blue03, 2.5);
        rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::blue03, -1);
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
        
    line(teclado, Point(0, 80), Point(240, 80), Paleta::blue02, 2.5);
    line(teclado, Point(0, 160), Point(240, 160), Paleta::blue02, 2.5);    
    line(teclado, Point(80, 0), Point(80, 240), Paleta::blue02, 2.5);    
    line(teclado, Point(160, 0), Point(160, 240), Paleta::blue02, 2.5);  
    rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::blue03, -1);
    arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::blue03, 3);
    arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::blue03, 3);
    arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::blue03, 3);
    arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::blue03, 3);
    arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::blue03, 2.5);
    arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::blue03, 2.5);    
    arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::blue03, 2.5);
    arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::blue03, 2.5);

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