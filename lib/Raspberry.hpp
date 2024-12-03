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

#ifdef BASE
#include <torch/script.h>
#include <memory>
#endif

#ifdef RASP
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <wiringPi.h>
#include <softPwm.h>
#endif

/* -------- Defines -------- */
#define CAMERA_VIDEO        0
#define CAMERA_FRAME_HEIGHT     240
#define CAMERA_FRAME_WIDTH      320
#define TECLADO_HEIGHT          CAMERA_FRAME_HEIGHT
#define TECLADO_WIDTH           TECLADO_HEIGHT      
#define BUTTON_WIDTH            80
#define BUTTON_HEIGHT           80
#define PWM_MAX                 100
#define MNIST_SIZE              28

#define xdebug { string st = "File="+string(__FILE__)+" line="+to_string(__LINE__)+"\n"; cout << st; }
#define xprint(x) { ostringstream os; os << #x " = " << x << '\n'; cout << os.str(); }

using namespace cv;

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
        PARADO,
        ALTERNA_MODO,
        NAO_SELECIONADO,
        
        AUTO_PARADO,
        AUTO_FRENTE,
        AUTO_180_ESQUERDA,
        AUTO_180_DIREITA,
        AUTO_90_ESQUERDA,
        AUTO_90_DIREITA,
    } Comando;

    typedef enum
    {
        AUTOMATICO = 0,
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
        return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
    }

    #ifdef RASP    
    #define M1_A    0
    #define M1_B    1
    #define M2_A    2
    #define M2_B    3

    namespace Motores
    {
        /*
         * Inicializa os GPIOs do controle da ponte H
         */
        inline void init()
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
         * Para os motores
         */
        inline void stop()
        {
            digitalWrite(M1_A, LOW);
            digitalWrite(M1_B, LOW);
            digitalWrite(M2_A, LOW);
            digitalWrite(M2_B, LOW);
        }

        /*
         * Seta a direção de rotação dos motores, conforme o comando passado
         */
        inline void setDir(Comando comando) 
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
                case PARADO:
                default:
                    digitalWrite(M1_A, LOW);
                    digitalWrite(M1_B, LOW);
                    digitalWrite(M2_A, LOW);
                    digitalWrite(M2_B, LOW);
                    break;
            }
        }

        /*
         * Inicializa os PWMs do controle da ponte H
         */
        inline void initPwm()
        {
            wiringPiSetup();
            
            if (softPwmCreate(M1_A, 0, PWM_MAX))
                throw std::runtime_error("Erro ao criar o PWM do M1_A");

            if (softPwmCreate(M1_B, 0, PWM_MAX))
                throw std::runtime_error("Erro ao criar o PWM do M1_B");

            if (softPwmCreate(M2_A, 0, PWM_MAX))
                throw std::runtime_error("Erro ao criar o PWM do M2_A");

            if (softPwmCreate(M2_B, 0, PWM_MAX))
                throw std::runtime_error("Erro ao criar o PWM do M2_B");
        }

        /*
         * Seta a direção de rotação dos motores, conforme o comando passado
         */
        inline void setDirPwm(Comando comando, int velocidade) 
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
                case PARADO:         
                default:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 0);
                    break;
            }
        }

        /*
         * Seta a direção de rotação dos motores, conforme o comando passado
         */
        inline void setDirAjustado(Comando comando) 
        {
            switch (comando) {
                case FRENTE:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 88);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 100);
                    break;
                case ATRAS:
                    softPwmWrite(M1_A, 88);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 100);
                    softPwmWrite(M2_B, 0);
                    break;
                case DIAGONAL_FRENTE_DIREITA:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 100);
                    break;
                case DIAGONAL_FRENTE_ESQUERDA:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 100);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 0);
                    break;
                case DIAGONAL_ATRAS_DIREITA:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 100);
                    softPwmWrite(M2_B, 0);
                    break;
                case DIAGONAL_ATRAS_ESQUERDA:
                    softPwmWrite(M1_A, 100);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 0);
                    break;
                case GIRA_ESQUERDA:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 100);
                    softPwmWrite(M2_A, 100);
                    softPwmWrite(M2_B, 0);
                    break;
                case GIRA_DIREITA:
                    softPwmWrite(M1_A, 100);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 100);
                    break;
                case PARADO:         
                default:
                    softPwmWrite(M1_A, 0);
                    softPwmWrite(M1_B, 0);
                    softPwmWrite(M2_A, 0);
                    softPwmWrite(M2_B, 0);
                    break;
            }
        }

        /*
         * Seta a velocidae de rotação dos motores, conforme o comando passado
         */
        inline void setVelPWM(int velocidades[]) 
        {        
            softPwmWrite(M1_A, velocidades[0]);
            softPwmWrite(M1_B, velocidades[1]);
            softPwmWrite(M2_A, velocidades[2]);
            softPwmWrite(M2_B, velocidades[3]);
        }

        /*
         * Para os motores
         */
        inline void stopPWM() 
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
    } // namespace Motores
    #endif

    /*
     * Função para facilitar a printar
     */
    inline void print(std::string s1="")
    {
        std::cout << s1 << std::endl;
    }

    /*
     * Printa o erro e encerra o programa, caso seja chamado pela Raspberry, também parará os motores
     */
    inline void erro(std::string s1="") 
    {
        #ifdef RASP_PINS
        Motores::motorStop();
        motorStop();
        #endif
        
        std::cerr << s1 << std::endl;
        exit(1);
    }

    #ifdef BASE
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
     * Retorna para a cor padrão da seta do comando correspondente 
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
                getTeclado(teclado);
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
     * Retorna os valores dos PWMs para o comando passado
     */
    inline void getVelocidades(Comando comando, int velocidadesPWM[]) 
    {
        switch (comando) {
            case FRENTE:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = PWM_MAX;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = PWM_MAX;
                break;
            case ATRAS:
                velocidadesPWM[0] = PWM_MAX;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = PWM_MAX;
                velocidadesPWM[3] = 0;
                break;
            case DIAGONAL_FRENTE_DIREITA:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = PWM_MAX;
                break;
            case DIAGONAL_FRENTE_ESQUERDA:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = PWM_MAX;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = 0;
                break;
            case DIAGONAL_ATRAS_DIREITA:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = PWM_MAX;
                velocidadesPWM[3] = 0;
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                velocidadesPWM[0] = PWM_MAX;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = 0;
                break;
            case GIRA_ESQUERDA:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = PWM_MAX;
                velocidadesPWM[2] = PWM_MAX;
                velocidadesPWM[3] = 0;
                break;
            case GIRA_DIREITA:
                velocidadesPWM[0] = PWM_MAX;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = PWM_MAX;
                break;
            case ALTERNA_MODO:            
            case NAO_SELECIONADO:
            default:
                velocidadesPWM[0] = 0;
                velocidadesPWM[1] = 0;
                velocidadesPWM[2] = 0;
                velocidadesPWM[3] = 0;
                break;
        }
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
    #endif // Base
} // namespace Raspberry

#ifdef BASE

namespace ImageProcessing
{
    using namespace Raspberry;

    /*
     * Torna a somatoria absoluta da imagem dar dois
     */
    inline Mat_<Flt> modulo2(Mat_<Flt> imagem) 
    {
        Mat_<Flt> clone = imagem.clone();
        
        double soma = 0.0;
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

    namespace TemplateMatching
    {
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
         * Retorna o modelo a ser buscado pré-processado e em diferêntes escalas
         */
        inline void getModeloPreProcessados(Mat_<Flt>& modelo, Mat_<Flt> modelosPreProcessados[], uint8_t numEscalas, float escalas[])
        {
            #pragma omp parallel for
            for (auto i = 0; i < numEscalas; i++) {
                Mat_<Raspberry::Flt> temp;

                resize(modelo, temp, Size(), escalas[i], escalas[i], INTER_NEAREST);
                        
                // Para poder usar o metodo de Correlação cruzada é nescessário pre-processar o modelo
                modelosPreProcessados[i] = ImageProcessing::modulo2(ImageProcessing::dcReject(temp, 1.0));
            }
        }

        /*
         * Retorna a posição da maior correlação encontrada
         */
        inline Raspberry::FindPos getMaxCorrelacao(Mat_<Raspberry::Flt>& frameBufFlt, Mat_<Raspberry::Flt> modelos[], Raspberry::FindPos corrBuf[], int numEscalas, float escalas[])
        {
            // Realiza o template matching pelas diferentes escalas e captura a maior correlação
            #pragma omp parallel for
            for (auto n = 0; n < numEscalas; n++) {
                Mat_<Raspberry::Flt> correlacao = ImageProcessing::TemplateMatching::matchTemplateSame(frameBufFlt, modelos[n], TM_CCOEFF_NORMED);

                Raspberry::CorrelacaoPonto correlacaoPonto;
                minMaxLoc(correlacao, NULL, &correlacaoPonto.correlacao, NULL, &correlacaoPonto.posicao);
                
                corrBuf[n] = Raspberry::FindPos{escalas[n], correlacaoPonto};
            }

            Raspberry::FindPos maxCorr = corrBuf[0];
            for (auto i = 1; i < numEscalas; i++) {
                if (corrBuf[i].ponto.correlacao > maxCorr.ponto.correlacao) {
                    maxCorr = corrBuf[i];
                }
            }  

            return maxCorr; 
        }
    } // namespace TemplateMatching
} // namespace ImageProcessing

namespace MNIST
{
    /*
     * Recorta a imagem para obter o numero MNIST no ponto passado, retorna ele no formato MNIST
     */
    inline Mat_<Raspberry::Flt> getMNIST(Mat_<Raspberry::Flt>& imagem, Point center, float size)
    {
        // Cálculo dos pontos de recorte
        Point a {std::max(int(center.x - size*0.5), 0), std::max(int(center.y - size*0.5), 0)};      
        Point b {std::min(int(center.x + size*0.5), CAMERA_FRAME_WIDTH), std::min(int(center.y + size*0.5), CAMERA_FRAME_HEIGHT)};

        // Recorte da imagem usando as coordenadas calculadas
        Rect region(a.x, a.y, b.x - a.x, b.y - a.y); // Definir a região do recorte
        Mat_<Raspberry::Flt> mnist_num = imagem(region);  
        
        // Redimendiona a imagem para o tamanho das imagens MNIST
        resize(mnist_num, mnist_num, Size(MNIST_SIZE, MNIST_SIZE), INTER_LINEAR);    
        
        Mat mnist_int;
        mnist_num.convertTo(mnist_int, CV_8UC1, 255.0);    

        // Satura os pixeis de forma inteligente, isso torna o reconhecimento mais resistente a variações no brilho.
        adaptiveThreshold(mnist_int, mnist_int, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 9, 2);        
        
        mnist_int.convertTo(mnist_num, CV_32F, 1.0 / 255.0);
        return mnist_num;
    }

    /*
     * Realiza a inferência do MNIST passado
     */
    inline int inferencia(Mat_<Raspberry::Flt>& imagem, torch::jit::script::Module& module)
    {
        // Converte o tipo para poder inserir no modelo
        torch::jit::IValue numEncontradoTensor = torch::from_blob(imagem.data, {1, 1, MNIST_SIZE, MNIST_SIZE}, torch::kFloat);

        torch::Tensor outputTensor = module.forward({numEncontradoTensor}).toTensor();

        // Obtém o numero predito
        return outputTensor.argmax(1).item<int>();
    }
} // namespace MNIST

namespace ControleAutomatico
{
    typedef enum 
    {
        BUSCA = 0,
        FOCA,
        IDENTIFICA,
        FINALIZA,
    } Estados;

    /*
     * Máquina de estado do controle automático
     */
    inline void maquinaEstados(Estados& controleEstado, Raspberry::Comando& comando, bool enquadrado, int numPredito) 
    {
        static double timer = Raspberry::timeSinceEpoch();

        switch (controleEstado) {
            case Estados::BUSCA: {
                comando = Raspberry::Comando::FRENTE;

                if (enquadrado) {
                    controleEstado = Estados::FOCA;
                }
                break;
            }
            
            case Estados::FOCA: {
                static bool inicioDelay = true;

                if (inicioDelay) {
                    timer = Raspberry::timeSinceEpoch();
                    inicioDelay = false;
                }

                comando = Raspberry::Comando::PARADO;

                double tempoDecorrido = Raspberry::timeSinceEpoch() - timer;
                if (tempoDecorrido > 2.0 && enquadrado) {
                    controleEstado = Estados::IDENTIFICA;
                    inicioDelay = true;
                }
                break;
            }

            case Estados::IDENTIFICA: {
                std::cout << numPredito << std::endl;

                switch (numPredito) {
                    case 2:
                        comando = Raspberry::Comando::AUTO_180_ESQUERDA;
                        break;
                    case 3:
                        comando = Raspberry::Comando::AUTO_180_DIREITA;
                        break;
                    case 4:
                    case 5:
                        comando = Raspberry::Comando::AUTO_FRENTE;
                        break;
                    case 6:
                    case 7:
                        comando = Raspberry::Comando::AUTO_90_ESQUERDA;
                        break;
                    case 8:
                    case 9:
                        comando = Raspberry::Comando::AUTO_90_DIREITA;
                        break;
                    case 0:
                    case 1:
                    default:
                        comando = Raspberry::Comando::AUTO_PARADO;
                        break;
                }

                timer = Raspberry::timeSinceEpoch();
                controleEstado = Estados::FINALIZA;
                break;
            }

            case Estados::FINALIZA: {
                static bool inicioDelay = true;

                if (inicioDelay) {
                    timer = Raspberry::timeSinceEpoch();
                    inicioDelay = false;
                }

                comando = Raspberry::Comando::PARADO;
                double tempoDecorrido = Raspberry::timeSinceEpoch() - timer;

                if (tempoDecorrido > 2.0) {
                    controleEstado = Estados::BUSCA;
                    inicioDelay = true;
                }
                break;
            }

            default: {
                comando = Raspberry::Comando::PARADO;               
                controleEstado = Estados::BUSCA;
                break;
            }
        }
    }
} // namespace ControleAutomatico
#endif // Base
#endif  // RASPBERRY_HPP
