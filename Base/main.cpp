/*
 *  Base: programa que recebe as imagens provindas da Raspberry Pi 3b
 *  realiza processamento nestas imagens e retorna comandos para a Pi
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Client.hpp"

/* -------- Defines -------- */
#define NUM_ESCALAS     16
#define ESCALA_MIN      0.3f 
#define ESCALA_MAX      0.02f
#define THRESHOLD       0.6
#define TEMPLATE_SIZE   401

/* -------- Variáveis Globais -------- */
static Raspberry::Teclado comando = Raspberry::Teclado::NAO_SELECIONADO;
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
        comando = Raspberry::Teclado::NAO_SELECIONADO;
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

     // Para conseguir detectar o modelo para diferentes distâncias é nescessário obtê-lo em diferêntes escalas
    Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
    const float escala = ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS);
    Raspberry::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, escala, ESCALA_MAX);

    // Para armazenar as imagens recebidas
    Mat_<Raspberry::Cor> frameBuf;
    Mat_<Raspberry::Flt> frameBufFlt;
        
    try {
        // Conecta à Raspberry
        Client client(argv[1], argv[2]);
        client.waitConnection();

        // Recebe os quadros e envia o comando
        while (true) {
            client.receiveImageCompactada(frameBuf);
            client.sendBytes(sizeof(Raspberry::Teclado), (Raspberry::Byte *) &comando);  

            // Converte a imagem recebida para float em escala de cinza
            Raspberry::Cor2Flt(frameBuf, frameBufFlt);

            // Realiza o template matching pelas diferentes escalas
            std::vector<Raspberry::FindPos> findPos(NUM_ESCALAS);
            
            #pragma omp parallel for
            for (auto n = 0; n < NUM_ESCALAS; n++) {
                // Template matching usando CCOEFF_NORMED
                Mat_<Raspberry::Flt> correlacao = Raspberry::matchTemplateSame(frameBufFlt, modelosPreProcessados[n], TM_CCOEFF_NORMED);

                Raspberry::CorrelacaoPonto correlacaoPonto;
                minMaxLoc(correlacao, 0, &correlacaoPonto.correlacao, 0, &correlacaoPonto.posicao);

                // Captura somente os pontos encontrados acima do limiar   
                if (correlacaoPonto.correlacao > THRESHOLD) {
                    auto fator = escala*n + ESCALA_MIN;

                    #pragma omp critical
                    findPos.push_back(Raspberry::FindPos {fator, correlacaoPonto});
                }
            }

            // Busca a maior correlação encontrada
            Raspberry::FindPos maxCorr;
            maxCorr.ponto.correlacao = 0.0;
            
            for (auto find : findPos) {
                if (find.ponto.correlacao > maxCorr.ponto.correlacao) {
                    maxCorr = find;
                }
            }

            // Desenha um retangulo na posição de maior correlação encontrada
            Raspberry::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*TEMPLATE_SIZE);

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