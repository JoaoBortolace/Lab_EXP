/*
 *  Template Matching: Realiza a busca do modelo em um vídeo
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include <queue>

#define NUM_ESCALAS 24
#define ESCALA_MIN  0.3f 
#define ESCALA_MAX  0.02f
#define THRESHOLD   0.6
#define TEMPLATE_SIZE   401

/* -------- Main -------- */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        Raspberry::erro("Poucos argumentos.");
    }

    try {
        // Inicia a camera e configura a camera
        VideoCapture camera(CAMERA_VIDEO);
        if (!camera.isOpened()) {
            Raspberry::erro("Falha ao abrir a camera.");
        }

        camera.set(CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
        camera.set(CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);
        
        // Obtem o modelo a ser buscado
        Mat_<Raspberry::Flt> modelo;
        Raspberry::Cor2Flt(imread(argv[1], 1), modelo);
        
        // Para conseguir detectar o modelo para diferentes distâncias é nescessário obtê-lo em diferêntes escalas
        Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
        Raspberry::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, ESCALA_MIN, ESCALA_MAX);
        
        // Para armazenar as imagens recebidas
        Mat_<Raspberry::Cor> frameBuf;
        Mat_<Raspberry::Flt> frameBufFlt;

        while (true) {
            auto t1 = Raspberry::timeSinceEpoch();

            camera >> frameBuf;
            Raspberry::Cor2Flt(frameBuf, frameBufFlt);

            // Realiza o template matching pelas diferentes escalas
            std::vector<Raspberry::FindPos> findPos(NUM_ESCALAS);
            
            #pragma omp parallel for
            for (auto n = 0; n < NUM_ESCALAS; n++) {
                // Template matching usando CCOEFF_NORMED
                Mat_<Raspberry::Flt> correlacao = Raspberry::matchTemplateSame(frameBufFlt, modelosPreProcessados[n], TM_CCOEFF_NORMED);

                Raspberry::CorrelacaoPonto correlacaoPonto;
                minMaxLoc(correlacao, nullptr, &correlacaoPonto.correlacao, nullptr, &correlacaoPonto.posicao);

                // Caso os pontos encontrados estejam acima do limiar e próximos entre si, salva a média das posições     
                if (correlacaoPonto.correlacao > THRESHOLD) {
                    auto fator = ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS)*n + ESCALA_MIN;

                    #pragma omp critical
                    findPos.push_back(Raspberry::FindPos {fator, correlacaoPonto});
                }
            }

            // Maior correlação encontrada
            Raspberry::FindPos maxCorr;
            maxCorr.ponto.correlacao = 0.0;
            
            for (auto find : findPos) {
                if (find.ponto.correlacao > maxCorr.ponto.correlacao) {
                    maxCorr = find;
                }
            }

            // Desenha um retangulo na posição de 
            Raspberry::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*TEMPLATE_SIZE);

            imshow("Camera", frameBuf);

            auto t2 = Raspberry::timeSinceEpoch();
            std::cout << "\r" << 1.0/(t2 - t1) << " FPS" << std::flush;

            if (waitKey(1) == 27) { // Esc
                break;
            }
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}
