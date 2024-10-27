// Raspberry.hpp
#ifndef RASPBERRY_HPP
#define RASPBERRY_HPP

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>

/* -------- Defines -------- */
#define CAMERA_VIDEO        0
#define CAMERA_FRAME_HEIGHT     240
#define CAMERA_FRAME_WIDTH      320
#define TECLADO_HEIGHT          CAMERA_FRAME_HEIGHT
#define TECLADO_WIDTH           TECLADO_HEIGHT      
#define BUTTON_WIDTH            80U
#define BUTTON_HEIGHT           80U

using namespace cv;

#define xdebug { string st = "File="+string(__FILE__)+" line="+to_string(__LINE__)+"\n"; cout << st; }
#define xprint(x) { ostringstream os; os << #x " = " << x << '\n'; cout << os.str(); }

namespace Raspberry
{
    typedef uint8_t Byte;
    typedef uint8_t Gry;
    typedef Vec3b Cor;

    typedef enum
    {
        FRENTE,
        ATRAS,
        DIAGONAL_FRENTE_DIREITA,
        DIAGONAL_FRENTE_ESQUERDA,
        DIAGONAL_ATRAS_DIREITA,
        DIAGONAL_ATRAS_ESQUERDA,
        GIRA_ESQUERDA,
        GIRA_DIREITA,
        NAO_FAZ_NADA,
        NAO_SELECIONADO,
    } Teclado;

    /*
     * Retorna a quantidade de segundos desde a última vez que esta foi chamada
     */
    inline double timeSinceEpoch(void) 
    {
        using namespace std::chrono;
        return duration_cast<duration<double>>( system_clock::now().time_since_epoch() ).count();
    }

    #ifdef RASP_PINS
    #include <wiringPi.h>

    #define M1_A    0
    #define M1_B    1
    #define M2_A    2
    #define M2_B    3

    /*
     * Inicializa os GPIOs do controle da ponte H
     */
    inline void motorInit()
    {
        wiringPiSetup();
        pinMode (M1_A, OUTPUT);
        pinMode (M1_B, OUTPUT);
        pinMode (M2_A, OUTPUT);
        pinMode (M2_B, OUTPUT);

        digitalWrite(M1_A, LOW);
        digitalWrite(M1_B, LOW);
        digitalWrite(M2_A, LOW);
        digitalWrite(M2_B, LOW);
    }

    /*
     * Seta a direção de rotação dos motores, conforme o comando passado
     */
    inline void motorSetDir(Teclado comando) 
    {
        switch (comando) {
            case FRENTE:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, HIGH);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, HIGH);
                break;
            case ATRAS:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_FRENTE_DIREITA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, HIGH);
                break;
            case DIAGONAL_FRENTE_ESQUERDA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, HIGH);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_ATRAS_DIREITA:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case GIRA_ESQUERDA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, HIGH);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case GIRA_DIREITA:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, HIGH);
                break;
            case NAO_FAZ_NADA:            
            case NAO_SELECIONADO:
            default:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
        }
    }
    #endif

    using namespace std;

    /*
     * Printa o erro e encerra o programa, caso seja chamado pela Raspberry, também parará os motores
     */
    inline void erro(string s1="") 
    {
        #ifdef RASP_PINS
        motorSetDir(Teclado::NAO_FAZ_NADA);
        #endif
        
        cerr << s1 << endl;
        exit(1);
    }

    inline void print(string s1="")
    {
        cout << s1 << endl;
    }

    inline bool testaVb(const std::vector<Raspberry::Byte>& vb, Raspberry::Byte b) {
        for (unsigned i=0; i<vb.size(); i++)
            if (vb[i]!=b) { 
                return false;
            }
        return true;
    }

    namespace Paleta 
    {
        const Raspberry::Cor red(0x58, 0x48, 0xFF);
        const Raspberry::Cor grey(0x7F, 0x7F, 0x74);
        const Raspberry::Cor blue01(0x79, 0x7F, 0x1B);
        const Raspberry::Cor blue02(0xC0, 0xCC, 0x00);
        const Raspberry::Cor blue03(0xEB, 0xF2, 0x72);
    }

    /*
     * Retorna o template do teclado de comandos
     */
    inline void getTeclado(Mat_<Cor>& teclado) {
        // Define a cor inicial do teclado e seu tamanho
        teclado = Mat_<Vec3b>(TECLADO_HEIGHT, TECLADO_WIDTH, Vec3b(Paleta::blue01[0], Paleta::blue01[1], Paleta::blue01[2]));

        // Desenha linhas horizontais e verticais
        line(teclado, Point(0, 80), Point(240, 80), Paleta::blue02, 2.5);
        line(teclado, Point(0, 160), Point(240, 160), Paleta::blue02, 2.5);
        line(teclado, Point(80, 0), Point(80, 240), Paleta::blue02, 2.5);
        line(teclado, Point(160, 0), Point(160, 240), Paleta::blue02, 2.5);

        // Desenha um retângulo preenchido
        rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::blue03, -1);

        // Desenha setas
        arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::blue03, 3);
        arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::blue03, 3);
        arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::blue03, 3);
        arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::blue03, 3);
        arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::blue03, 2.5);
        arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::blue03, 2.5);
        arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::blue03, 2.5);
        arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::blue03, 2.5);
    }

    /*
     *  Retorna para a cor padrão da seta do comando correspondente 
     */
    inline void limpaTeclado(Mat_<Cor>& teclado, Teclado comando) {
        switch (comando)
        {
            case FRENTE:
                arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::blue03, 3);
                break;
            case ATRAS:
                arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::blue03, 3);
                break;
            case DIAGONAL_FRENTE_DIREITA:
                arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::blue03, 2.5);
                break;
            case DIAGONAL_FRENTE_ESQUERDA:
                arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::blue03, 2.5);
                break;
            case DIAGONAL_ATRAS_DIREITA:
                arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::blue03, 2.5);
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::blue03, 2.5);
                break;
            case GIRA_ESQUERDA:
                arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::blue03, 3);
                break;
            case GIRA_DIREITA:
                arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::blue03, 3);
                break;
            case NAO_FAZ_NADA:
                rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::blue03, -1);
                break;
            case NAO_SELECIONADO:
            default:
                break;
        }
    }
    /*
     * Obtém-se qual é o comando equivalente a posição do clique, se for válido, também altera a cor da seta correspondete para vermelho 
     */
    inline void getComando(uint32_t col, uint32_t row, Mat_<Cor>& teclado, Teclado comando) 
    {
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
}

#endif

