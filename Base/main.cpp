/*
 *  Base: programa que recebe as imagens provindas da Raspberry Pi 3b
 *  realiza processamento nestas imagens e retorna comandos para a Pi
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Client.hpp"

/* -------- Defines -------- */
#define NUM_ESCALAS     20
#define ESCALA_MAX      0.4f 
#define ESCALA_MIN      0.03f
#define THRESHOLD       0.65f
#define NUM_SIZE        280

/* -------- Variáveis Globais -------- */
static Raspberry::Comando comando = Raspberry::Comando::NAO_SELECIONADO;
static Mat_<Raspberry::Cor> teclado;

/* -------- Callbacks -------- */
void mouse_callback(int event, int x, int y, int flags, void *usedata)
{
    (void)flags;
    (void)usedata;
    
    if (event == EVENT_LBUTTONDOWN) {
        uint32_t col = x / BUTTON_WIDTH;
        uint32_t row = y / BUTTON_HEIGHT;
        Raspberry::getComando(col, row, teclado, comando);
    }
    else if (event == EVENT_LBUTTONUP) {
        Raspberry::limpaTeclado(teclado, comando);
        comando = Raspberry::Comando::NAO_SELECIONADO;
    }
}

/* -------- Main -------- */
int main(int argc, char *argv[])
{
    if (argc < 4) {
        Raspberry::erro("Poucos argumentos.");
    }

    // Configura a janela aonde será transmitido o vídeo, obtem o template do teclado
    Raspberry::getTeclado(teclado);
    namedWindow("RaspCam", WINDOW_AUTOSIZE);
    setMouseCallback("RaspCam", mouse_callback);

    // Obtem o modelo a ser buscado
    Mat_<Raspberry::Flt> modelo;
    Raspberry::Cor2Flt(imread(argv[3], 1), modelo);
    const int templateSize = modelo.cols;

    // Para conseguir detectar o modelo para diferentes distâncias é nescessário obtê-lo em diferêntes escalas
    Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
    const float escala = ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS);
    Raspberry::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, escala, ESCALA_MIN);

    // Para armazenar as imagens recebidas
    Mat_<Raspberry::Cor> frameBuf;
    Mat_<Raspberry::Flt> frameBufFlt;

    // Modos de operação
    Raspberry::Controle controle = Raspberry::Controle::MANUAL;
        
    try {
        // Conecta à Raspberry
        Client client(argv[1], argv[2]);
        client.waitConnection();

        while (true) {
            auto t1 = Raspberry::timeSinceEpoch();
            
            // Recebe os quadros e envia o comando
            client.receiveImageCompactada(frameBuf);
            client.sendBytes(sizeof(Raspberry::Comando), (Raspberry::Byte *) &comando);  

            // Alterna entre o controle manual ou automático
            if (comando == Raspberry::Comando::ALTERNA_MODO) {
                controle = static_cast<Raspberry::Controle>(~controle & 1);
            }

            // Controle Autômato
            if (controle == Raspberry::Controle::AUTOMATO) {
                // Converte a imagem recebida para float em escala de cinza
                Raspberry::Cor2Flt(frameBuf, frameBufFlt);

                // Realiza o template matching pelas diferentes escalas
                std::vector<Raspberry::FindPos> findPos(NUM_ESCALAS);
                
                #pragma omp parallel for
                for (auto n = 0; n < NUM_ESCALAS; n++) {
                    // Template matching usando CCOEFF_NORMED
                    Mat_<Raspberry::Flt> correlacao = Raspberry::matchTemplateSame(frameBufFlt, modelosPreProcessados[n], TM_CCOEFF_NORMED);

                    Raspberry::CorrelacaoPonto correlacaoPonto;
                    minMaxLoc(correlacao, NULL, &correlacaoPonto.correlacao, NULL, &correlacaoPonto.posicao);

                    // Captura somente os pontos encontrados acima do limiar   
                    if (correlacaoPonto.correlacao > THRESHOLD) {
                        #pragma omp critical
                        findPos.push_back(Raspberry::FindPos {escala*n + ESCALA_MIN, correlacaoPonto});
                    }
                }

                // Busca a maior correlação encontrada
                Raspberry::FindPos maxCorr = findPos[0];

                for (auto find : findPos) {
                    if (find.ponto.correlacao > maxCorr.ponto.correlacao) {
                        maxCorr = find;
                    }
                }

                // Altera as velocidades dos motores para o carrinho seguir o template
                int velocidades[4] = {0};

                if (maxCorr.escala > 0) {
                    velocidades[1] = velocidades[3] = PWM_MAX;

                    // Se o ponto encontrado estiver na extrema direita => 100, extrema esquerda => -100, centro => 0
                    int pos_normalizada = (int) ((maxCorr.ponto.posicao.x - (CAMERA_FRAME_WIDTH >> 1)) / ((CAMERA_FRAME_WIDTH >> 1)/100.0)); 
                    
                    if (pos_normalizada > 0) {
                        velocidades[1] -= pos_normalizada; 
                    }
                    else {
                        velocidades[3] += pos_normalizada;
                    }
                    
                    // Desenha um retangulo na posição de maior correlação encontrada
                    Raspberry::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*templateSize);

                    // Cálculo dos pontos de recorte
                    Point a {
                        std::max(int(maxCorr.ponto.posicao.x - maxCorr.escala * NUM_SIZE * 0.5), 0), 
                        std::max(int(maxCorr.ponto.posicao.y - maxCorr.escala * NUM_SIZE * 0.5), 0)
                    };
                    
                    Point b {
                        std::min(int(maxCorr.ponto.posicao.x + maxCorr.escala * NUM_SIZE * 0.5), frameBufFlt.cols), // frameBufFlt.cols dá a largura da imagem
                        std::min(int(maxCorr.ponto.posicao.y + maxCorr.escala * NUM_SIZE * 0.5), frameBufFlt.rows) // frameBufFlt.rows dá a altura da imagem
                    };

                    // Recorte da imagem usando as coordenadas calculadas
                    Rect region(a.x, a.y, b.x - a.x, b.y - a.y); // Definir a região do recorte
                    Mat numEncontrado = frameBufFlt(region);
                    imshow("encontrado", numEncontrado);
                }

                client.sendBytes(sizeof(velocidades), (Raspberry::Byte*) velocidades);
                putText(frameBuf, "Seguindo", Point(160, 220), FONT_HERSHEY_DUPLEX, 1.0, Raspberry::Paleta::red, 1.8);                                
            }

            // Coloca o teclado 
            hconcat(teclado, frameBuf, frameBuf);  

            // Exibi o quadro
            imshow("RaspCam", frameBuf);
            if ((waitKey(1) & 0xFF)  == 27) { // Esc
                break;
            }

            auto t2 = Raspberry::timeSinceEpoch();
            std::cout << "\r" << 1.0/(t2 - t1) << " FPS" << std::flush;
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}

/*
#include <opencv2/opencv.hpp>

int main() {
    // Carregar a imagem
    cv::Mat img = cv::imread("../quadrado.png");
    
    if (img.empty()) {
        std::cerr << "Erro ao carregar a imagem!" << std::endl;
        return -1;
    }

    // Obter as dimensões da imagem
    int width = img.cols;
    int height = img.rows;

    // Calcular o centro da imagem
    cv::Point2f center(width / 2.0f, height / 2.0f);

    // Definir o ângulo de rotação (em graus) e o fator de escala (1.0 significa sem escala)
    double angle = 45.0; // Ângulo de rotação
    double scale = 1.0;

    // Criar a matriz de rotação
    cv::Mat rotMat = cv::getRotationMatrix2D(center, angle, scale);

    // Calcular as dimensões da imagem resultante (para evitar corte após a rotação)
    cv::Rect bbox = cv::RotatedRect(center, img.size(), angle).boundingRect();

    // Ajustar a matriz de rotação para compensar o deslocamento da imagem
    rotMat.at<double>(0, 2) += bbox.width / 2.0 - center.x;
    rotMat.at<double>(1, 2) += bbox.height / 2.0 - center.y;

    // Aplicar a rotação usando warpAffine
    cv::Mat rotatedImg;
    cv::warpAffine(img, rotatedImg, rotMat, bbox.size(), cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    // Exibir as imagens
    cv::imshow("Imagem Original", img);
    cv::imshow("Imagem Rotacionada", rotatedImg);

    // Aguardar tecla para fechar as janelas
    cv::waitKey(0);

    return 0;
}
*/