/*
 *  Base: programa que recebe as imagens provindas da Raspberry Pi 3b
 *  realiza processamento nestas imagens e retorna comandos para a Pi
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Client.hpp"
#include <thread>
#include <atomic>
#include <functional>

/* -------- Defines -------- */
#define TEMPLATE_SIZE   401
#define NUM_SIZE        150

#define NUM_ESCALAS     20
#define ESCALA_MAX      0.4f 
#define ESCALA_MIN      0.03f
#define ESCALA          ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS)
#define THRESHOLD       0.65f

#define ESCALA_DIST_MIN 0.2f

/* -------- Variáveis Globais -------- */
static Raspberry::Controle controle = Raspberry::Controle::MANUAL;
static Mat_<Raspberry::Cor> teclado;

static Raspberry::Comando comandoManual = Raspberry::Comando::NAO_SELECIONADO;
static int velocidadesManual[4] = {0};

/* -------- Callbacks -------- */
void mouse_callback(int event, int x, int y, int flags, void *usedata)
{
    (void)flags;
    (void)usedata;
    
    if (event == EVENT_LBUTTONDOWN) {
        uint32_t col = x / BUTTON_WIDTH;
        uint32_t row = y / BUTTON_HEIGHT;
        
        // Obtem qual comando foi pressionado, e qual deve ser os valores dos PWMs
        Raspberry::getComando(col, row, teclado, comandoManual);
        Raspberry::getVelocidades(comandoManual, PWM_MAX, velocidadesManual);
        
        // Alterna entre o controle manual ou automático
        if (comandoManual == Raspberry::Comando::ALTERNA_MODO) {
            controle = static_cast<Raspberry::Controle>(~controle & 1);
        }
    }
    else if (event == EVENT_LBUTTONUP) {
        Raspberry::limpaTeclado(teclado, comandoManual);
        comandoManual = Raspberry::Comando::NAO_SELECIONADO;
        memset(velocidadesManual, 0, sizeof(velocidadesManual));
    }
}

/* -------- Main -------- */
int main(int argc, char *argv[])
{
    if (argc != 5) {
        Raspberry::erro("Argumentos errados.");
    }

    // Configurações para exibir os quadros recebidos
    Mat_<Raspberry::Cor> frameBuf;
    Mat_<Raspberry::Flt> frameBufFlt;

    Raspberry::getTeclado(teclado);
    namedWindow("RaspCam", WINDOW_AUTOSIZE);
    setMouseCallback("RaspCam", mouse_callback);

    // Obtem o modelo a ser buscado, para conseguir detectar-lo em diferentes distâncias, é nescessário diferêntes escalas dele
    Mat_<Raspberry::Flt> modelo;
    ImageProcessing::Cor2Flt(imread(argv[4], 1), modelo);

    Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
    ImageProcessing::TemplateMatching::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, ESCALA, ESCALA_MIN);

    // Modelo para reconhecer o número do MNIST
    torch::jit::script::Module module;

    // Thread resposável por executar a máquina de estado do controle automático
    std::atomic<bool> templateEncontrado(false), templateEnquadrado(false);
    std::atomic<ControleAutomatico::ControleEstados> controleEstado(ControleAutomatico::ControleEstados::BUSCA_TEMPLATE);
    int velocidadesAutomatico[4] = {0};

    std::thread automatico(ControleAutomatico::modoAutomatico, std::ref(controleEstado), std::ref(templateEncontrado), std::ref(templateEnquadrado));

    try {
        module = torch::jit::load(argv[3], torch::Device(torch::kCPU));
        
        // Certifica que o modelo está no modo de inferência
        module = torch::jit::optimize_for_inference(module);
        
        // Conecta à Raspberry
        Client client(argv[1], argv[2]);
        client.waitConnection();

        while (true) {            
            // Recebe os quadros
            client.receiveImageCompactada(frameBuf);

            // Controle Autômato
            if (controle == Raspberry::Controle::AUTOMATO) {
                // Realiza o template matching pelas diferentes escalas e captura a maior correlação
                ImageProcessing::Cor2Flt(frameBuf, frameBufFlt);

                Raspberry::FindPos findPos[NUM_ESCALAS];
                
                #pragma omp parallel for
                for (auto n = 0; n < NUM_ESCALAS; n++) {
                    Mat_<Raspberry::Flt> correlacao = ImageProcessing::TemplateMatching::matchTemplateSame(frameBufFlt, modelosPreProcessados[n], TM_CCOEFF_NORMED);

                    Raspberry::CorrelacaoPonto correlacaoPonto;
                    minMaxLoc(correlacao, NULL, &correlacaoPonto.correlacao, NULL, &correlacaoPonto.posicao);
                    
                    findPos[n] = Raspberry::FindPos{ESCALA*n + ESCALA_MIN, correlacaoPonto};
                }

                Raspberry::FindPos maxCorr = findPos[0];
                for (auto i = 1; i < NUM_ESCALAS; i++) {
                    if (findPos[i].ponto.correlacao > maxCorr.ponto.correlacao) {
                        maxCorr = findPos[i];
                    }
                }

                // Caso tenha encontrado um template
                if (maxCorr.ponto.correlacao > THRESHOLD) { 
                    templateEncontrado = true;

                    if (maxCorr.escala > ESCALA_DIST_MIN && (maxCorr.ponto.posicao.x > 150 && maxCorr.ponto.posicao.x < 170)) {
                        templateEnquadrado = true;
                    }
                    else {
                        templateEnquadrado = false;
                    }

                    // Desenha um retangulo ao redor da posição de maior correlação encontrada
                    ImageProcessing::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*TEMPLATE_SIZE);

                    // Captura o número de dentro do modelo encontrado
                    Mat_<Raspberry::Flt> numEncontrado = MNIST::getMNIST(frameBufFlt, maxCorr.ponto.posicao, maxCorr.escala*NUM_SIZE);

                    // Realiza a predição do numero encontrado
                    int numPredito = MNIST::inferenciaMNIST(numEncontrado, module);

                    ControleAutomatico::getVelocidadesAutomatico(controleEstado, numPredito, maxCorr.ponto.posicao.x, PWM_MAX, velocidadesAutomatico);

                    // Adiciona o número predito ao quadro
                    putText(frameBuf, std::to_string(numPredito), Point(20, 220), FONT_HERSHEY_DUPLEX, 1.0, Raspberry::Paleta::blue03, 1.2); 
                }
                else {
                    templateEncontrado = false;
                    templateEnquadrado = false;
                }

                putText(frameBuf, "Seguindo", Point(160, 220), FONT_HERSHEY_DUPLEX, 1.0, Raspberry::Paleta::red, 1.8);

                // Envias o comando com os valores dos PWMs dos motores
                client.sendBytes(sizeof(velocidadesAutomatico), (Raspberry::Byte*) velocidadesAutomatico);                                
            } 
            else {
                client.sendBytes(sizeof(velocidadesManual), (Raspberry::Byte*) velocidadesManual);
            }

            // Coloca o teclado no quadro
            hconcat(teclado, frameBuf, frameBuf);  

            // Exibi o quadro
            imshow("RaspCam", frameBuf);
            if (waitKey(1)  == 27) { // Esc
                break;
            }
        }
    }
    catch (const std::exception& e) {
        Raspberry::erro(e.what());
    }
    
    return 0;
}