// Raspberry.hpp
#ifndef RASPBERRY_HPP
#define RASPBERRY_HPP

#include <opencv2/opencv.hpp>
#include <omp.h>
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
    using Byte = uint8_t;
    using Gry  = uint8_t;
    using Cor  = Vec3b;
    using Flt  = float;

    const double epsilon = FLT_EPSILON;

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
        ALTERNA_MODO,
        NAO_SELECIONADO,
    } Comando;

    typedef enum
    {
        AUTOMATO = 0,
        MANUAL,
    } Controle;

    typedef struct
    {
        double correlacao;
        Point posicao;
    } CorrelacaoPonto;

    typedef struct
    {
        float escala;
        CorrelacaoPonto ponto;
    } FindPos;

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
    #include <softPwm.h>
    
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
    inline void motorSetDir(Comando comando) 
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
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
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
            case ALTERNA_MODO:            
            case NAO_SELECIONADO:
            default:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
        }
    }

    /*
     * Seta a direção de rotação dos motores, conforme o comando passado
     */
    inline void motorSetDirPwm(Comando comando, int velocidade) 
    {
        switch (comando) {
            case FRENTE:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, velocidade);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, velocidade);
                break;
            case ATRAS:
                softPwmWrite(M1_A, velocidade);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, velocidade);
                softPwmWrite(M2_B, 0);
                break;
            case DIAGONAL_FRENTE_DIREITA:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, velocidade);
                break;
            case DIAGONAL_FRENTE_ESQUERDA:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, velocidade);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, 0);
                break;
            case DIAGONAL_ATRAS_DIREITA:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, velocidade);
                softPwmWrite(M2_B, 0);
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                softPwmWrite(M1_A, velocidade);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, 0);
                break;
            case GIRA_ESQUERDA:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, velocidade);
                softPwmWrite(M2_A, velocidade);
                softPwmWrite(M2_B, 0);
                break;
            case GIRA_DIREITA:
                softPwmWrite(M1_A, velocidade);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, velocidade);
                break;
            case ALTERNA_MODO:            
            case NAO_SELECIONADO:
            default:
                softPwmWrite(M1_A, 0);
                softPwmWrite(M1_B, 0);
                softPwmWrite(M2_A, 0);
                softPwmWrite(M2_B, 0);
                break;
        }
    }

    /*
     * Inicializa os PWMs do controle da ponte H
     */
    inline void motorInitPwm()
    {
        wiringPiSetup();
        
        if (softPwmCreate(M1_A, 0, 100))
            throw std::runtime_error("Erro ao criar o PWM do M1_A");

        if (softPwmCreate(M1_B, 0, 100))
            throw std::runtime_error("Erro ao criar o PWM do M1_B");

        if (softPwmCreate(M2_A, 0, 100))
            throw std::runtime_error("Erro ao criar o PWM do M2_A");

        if (softPwmCreate(M2_B, 0, 100))
            throw std::runtime_error("Erro ao criar o PWM do M2_B");
    }

    /*
     * Seta a velocidae de rotação dos motores, conforme o comando passado
     */
    inline void motorSetVel(int velocidades[]) 
    {        
        softPwmWrite(M1_A, velocidades[0]);
        softPwmWrite(M1_B, velocidades[1]);
        softPwmWrite(M2_A, velocidades[2]);
        softPwmWrite(M2_B, velocidades[3]);
    }

    /*
     * Para os motores
     */
    inline void motorStop() 
    {        
        softPwmWrite(M1_A, 0);
        softPwmWrite(M1_B, 0);
        softPwmWrite(M2_A, 0);
        softPwmWrite(M2_B, 0);
        softPwmStop(M1_A);
        softPwmStop(M1_B);
        softPwmStop(M2_A);
        softPwmStop(M2_B);
    }
    #endif

    using namespace std;

    /*
     * Printa o erro e encerra o programa, caso seja chamado pela Raspberry, também parará os motores
     */
    inline void erro(string s1="") 
    {
        #ifdef RASP_PINS
        motorSetDir(Comando::NAO_SELECIONADO);
        motorStop();
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
    inline void limpaTeclado(Mat_<Cor>& teclado, Comando comando) {
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
            case ALTERNA_MODO:
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
    inline void getComando(uint32_t col, uint32_t row, Mat_<Cor>& teclado, Comando& comando) 
    {
        // Primeiro seleciona qual coluna foi escolhida
        switch (col) {
            case 0:
                // Depois seleciona qual linha foi escolhida
                switch (row) {
                    case 0:
                        comando = Comando::DIAGONAL_FRENTE_ESQUERDA;
                        arrowedLine(teclado, Point(60, 60), Point(20, 20), Paleta::red, 2.5);
                        break;
                    case 1:
                        comando = Comando::GIRA_ESQUERDA;
                        arrowedLine(teclado, Point(60, 120), Point(20, 120), Paleta::red, 3);
                        break;
                    case 2:
                        comando = Comando::DIAGONAL_ATRAS_ESQUERDA;
                        arrowedLine(teclado, Point(60, 180), Point(20, 220), Paleta::red, 2.5);    
                        break;
                }
                break;
            case 1:
                switch (row) {
                    case 0:
                        comando = Comando::FRENTE;
                        arrowedLine(teclado, Point(120, 60), Point(120, 20), Paleta::red, 3);
                        break;
                    case 1:
                        comando = Comando::ALTERNA_MODO;
                        rectangle(teclado, Point(115, 115), Point(125, 125), Paleta::grey, -1);
                        break;
                    case 2:
                        comando = Comando::ATRAS;
                        arrowedLine(teclado, Point(120, 180), Point(120, 220), Paleta::red, 3);
                        break;
                }
                break;
            case 2:
                switch (row) {
                    case 0:
                        comando = Comando::DIAGONAL_FRENTE_DIREITA;
                        arrowedLine(teclado, Point(180, 60), Point(220, 20), Paleta::red, 2.5);
                        break;
                    case 1:
                        comando = Comando::GIRA_DIREITA;
                        arrowedLine(teclado, Point(180, 120), Point(220, 120), Paleta::red, 3);
                        break;
                    case 2:
                        comando = Comando::DIAGONAL_ATRAS_DIREITA;
                        arrowedLine(teclado, Point(180, 180), Point(220, 220), Paleta::red, 2.5);
                        break;
                }
                break;
            default:
                comando = Comando::NAO_SELECIONADO;
                break;
        }
    }

    /*
     * Realiza a busca do modelo na imagem, retorna uma imagem de correlação de mesma dimensão.
     */
    inline Mat_<Flt> matchTemplateSame(Mat_<Flt> imagem, Mat_<Flt> modelo, int metodo, Flt backgroundColor = 0.0f)
    {
        Mat_<Flt> resultado{imagem.size(), backgroundColor};
        Rect rect{(modelo.cols-1)/2, (modelo.rows-1)/2, imagem.cols - modelo.cols + 1, imagem.rows - modelo.rows + 1};
        Mat_<Flt> roi{resultado, rect};
        matchTemplate(imagem, modelo, roi, metodo);
        return resultado;
    }

    /*
     * Calcula a distância euclidiana de dois pontos
     */
    inline double distanciaEuclidiana(Point a, Point b)
    {
        double x = a.x - b.x;
        double y = a.y - b.y;
        return sqrtf64(x*x + y*y);
    }

    /*
     * Retorna o ponto médio
     */
    inline Point pontoMedio(Point a, Point b)
    {
        int x = 0.5*(a.x + b.x);
        int y = 0.5*(a.y + b.y);
        return Point{x, y};
    }

    /*
     * Torna a somatoria absoluta da imagem dar dois
     */
    inline Mat_<Flt> modulo2(Mat_<Flt> imagem) 
    {
        Mat_<Flt> clone = imagem.clone();
        
        double soma=0.0;
        for (auto it = clone.begin(); it != clone.end(); it++) {
            soma += abs(*it);
        }

        if (soma < epsilon) {
            throw std::runtime_error("Erro: Divisao por zero");
        }

        soma = 2.0/(soma);

        for (auto it = clone.begin(); it != clone.end(); it++) {
            (*it) *= soma;
        }
              
        return clone;
    }

    /*
     * Elimina nivel DC (subtrai media)
     */
    inline Mat_<Flt> dcReject(Mat_<Flt> imagem) 
    { 
        return imagem - mean(imagem)[0];;
    }

    /*
     * Elimina nivel DC (subtrai media) com dontcare
     */
    inline Mat_<Flt> dcReject(Mat_<Flt> imagem, Flt dontcare) 
    {
        Mat_<uchar> naodontcare = (imagem != dontcare); 
        Scalar media = mean(imagem, naodontcare);
        subtract(imagem, media[0], imagem, naodontcare);
        Mat_<uchar> simdontcare = (imagem == dontcare); 
        subtract(imagem, dontcare, imagem, simdontcare);
        return imagem;
    }
    
    /*
     * Converte uma imagem de Cor (Vec3b) para Float em escala de cinza
     */
    inline void Cor2Flt(Mat_<Cor> entrada, Mat_<Flt>& saida) 
    {
        Mat_<Vec3f> temp; 
        entrada.convertTo(temp, CV_32F, 1.0/255.0, 0.0);
        cvtColor(temp, saida, COLOR_BGR2GRAY);
    }

    /*
     * Adiciona a imagem um retangulo não preenchido centralizado no ponto passado
     */
    template <typename T>
    inline void ploteRetangulo(Mat_<T>& image, Point center, float size, Raspberry::Cor color = Paleta::red, float espessura = 1.5)
    {
        Point a {max(center.x - (int) (size*0.5), 0), max(center.y - (int) (size*0.5), 0)};
        Point b {min(center.x + (int) (size*0.5), CAMERA_FRAME_WIDTH), min(center.y + (int) (size*0.5), CAMERA_FRAME_HEIGHT)};    
        rectangle(image, a, b, color, espessura);
    }

   inline void getModeloPreProcessados(Mat_<Flt>& modelo, Mat_<Flt> modelosPreProcessados[], uint8_t numEscalas, float escala, float escalaMin=0.0f)
   {
        #pragma omp parallel for
        for (auto i = 0; i < numEscalas; i++) {
            auto fator = escala*i + escalaMin;
            Mat_<Raspberry::Flt> temp;

            resize(modelo, temp, Size(), fator, fator, INTER_NEAREST);
                    
            // Para poder usar o metodo de Correlação cruzada é nescessário pre-processar o modelo
            modelosPreProcessados[i] = Raspberry::modulo2(Raspberry::dcReject(temp, 1.0));
        }
    }
}

#endif
