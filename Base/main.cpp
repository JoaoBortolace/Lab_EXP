/*
 *  Base: programa que recebe as imagens provindas da Raspberry Pi 3b
 *  realiza processamento nestas imagens e retorna comandos para a Pi
 */

/* -------- Includes -------- */
#include "Raspberry.hpp"
#include "Client.hpp"

/* -------- Defines -------- */
#define TEMPLATE_SIZE   401
#define NUM_SIZE        150

#define NUM_ESCALAS     20
#define ESCALA_MAX      0.4f 
#define ESCALA_MIN      0.03f
#define ESCALA          ((ESCALA_MAX - ESCALA_MIN) / NUM_ESCALAS)
#define THRESHOLD       0.6f

#define ESCALA_DIST_MIN 0.08f

/* -------- Variáveis Globais -------- */
static Mat_<Raspberry::Cor> teclado;
static Raspberry::Controle controle = Raspberry::Controle::MANUAL;
static Raspberry::Comando comando = Raspberry::Comando::NAO_SELECIONADO;

/* -------- Callbacks -------- */
void mouse_callback(int event, int x, int y, int flags, void *usedata)
{
    (void)flags;
    (void)usedata;
    
    if (event == EVENT_LBUTTONDOWN) {
        uint32_t col = x / BUTTON_WIDTH;
        uint32_t row = y / BUTTON_HEIGHT;
        
        // Obtem qual comando foi pressionado, e qual deve ser os valores dos PWMs
        Raspberry::getComando(col, row, teclado, comando);

        // Alterna entre o controle manual ou automático
        if (comando == Raspberry::Comando::ALTERNA_MODO) {
            controle = static_cast<Raspberry::Controle>(~controle & 1);
            Raspberry::limpaTeclado(teclado, comando);
        }
    }
    else if (event == EVENT_LBUTTONUP) {
        Raspberry::limpaTeclado(teclado, comando);
        comando = Raspberry::Comando::NAO_SELECIONADO;
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
    float escalas[NUM_ESCALAS];
    for (auto n = 0; n < NUM_ESCALAS; n++) {
        escalas[n] = ESCALA*n + ESCALA_MIN;
    }
    
    Mat_<Raspberry::Flt> modelo;
    ImageProcessing::Cor2Flt(imread(argv[4], 1), modelo);
    Mat_<Raspberry::Flt> modelosPreProcessados[NUM_ESCALAS];
    ImageProcessing::TemplateMatching::getModeloPreProcessados(modelo, modelosPreProcessados, NUM_ESCALAS, escalas);
    Raspberry::FindPos corrBuf[NUM_ESCALAS];

    // Variáveis auxliares para o controle automático
    ControleAutomatico::Estados controleEstado = ControleAutomatico::Estados::BUSCA;
    int numPredito;

    // Modelo para reconhecer o número do MNIST
    torch::jit::script::Module module;

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
            if (controle == Raspberry::Controle::AUTOMATICO) {
                putText(frameBuf, "Automatico", Point(20, 220), FONT_HERSHEY_DUPLEX, 1.0, Raspberry::Paleta::red, 1.8);  

                ImageProcessing::Cor2Flt(frameBuf, frameBufFlt);

                // Obtem o ponto de maior correlação com o modelo
                Raspberry::FindPos maxCorr = ImageProcessing::TemplateMatching::getMaxCorrelacao(frameBufFlt, modelosPreProcessados, corrBuf, NUM_ESCALAS, escalas);

                // Caso tenha encontrado um template
                bool enquadrado = false;

                if (maxCorr.ponto.correlacao > THRESHOLD) {
                    if (maxCorr.escala > ESCALA_DIST_MIN) {
                        enquadrado = true;
                    } 

                    // Desenha um retangulo ao redor da posição de maior correlação encontrada
                    ImageProcessing::ploteRetangulo(frameBuf, maxCorr.ponto.posicao, maxCorr.escala*TEMPLATE_SIZE);
                    // Captura o número de dentro do modelo encontrado
                    Mat_<Raspberry::Flt> numEncontrado = MNIST::getMNIST(frameBufFlt, maxCorr.ponto.posicao, maxCorr.escala*NUM_SIZE);
                    // Realiza a predição do numero encontrado
                    numPredito = MNIST::inferencia(numEncontrado, module);
                    // Adiciona o número predito ao quadro
                    putText(frameBuf, std::to_string(numPredito), Point(220, 220), FONT_HERSHEY_DUPLEX, 1.0, Raspberry::Paleta::blue03, 1.2); 
                } 
                
                // Processa a máquina de estados
                ControleAutomatico::maquinaEstados(controleEstado, comando, enquadrado, numPredito);   
            } 
            
            // Envias o comando de controle dos motores            
            client.sendBytes(sizeof(comando), (Raspberry::Byte*) &comando);
            
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