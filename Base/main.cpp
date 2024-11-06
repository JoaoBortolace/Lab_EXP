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
    ImageProcessing::Cor2Flt(imread(argv[3], 1), modelo);
    const int templateSize = modelo.cols;

    // Para conseguir detectar o modelo para diferentes distâncias é nescessário obtê-lo em diferêntes escalas
    Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
    const float escala = ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS);
    ImageProcessing::TemplateMatching::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, escala, ESCALA_MIN);

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
                ImageProcessing::Cor2Flt(frameBuf, frameBufFlt);

                // Realiza o template matching pelas diferentes escalas
                std::vector<Raspberry::FindPos> findPos(NUM_ESCALAS);
                
                #pragma omp parallel for
                for (auto n = 0; n < NUM_ESCALAS; n++) {
                    // Template matching usando CCOEFF_NORMED
                    Mat_<Raspberry::Flt> correlacao = ImageProcessing::TemplateMatching::matchTemplateSame(frameBufFlt, modelosPreProcessados[n], TM_CCOEFF_NORMED);

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
                    ImageProcessing::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*templateSize);

                    // Captura o número do MNIST no modelo encontrado
                    Mat_<Raspberry::Flt> numEncontrado = ImageProcessing::MNIST::getMNIST(frameBufFlt, maxCorr.ponto.posicao, maxCorr.escala*NUM_SIZE);
                    imshow("encontrado", numEncontrado);
                }

                // Envias o comando com os valores dos PWMs dos motores
                client.sendBytes(sizeof(velocidades), (Raspberry::Byte*) velocidades);

                // Avisa que está no modo Autonomo
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