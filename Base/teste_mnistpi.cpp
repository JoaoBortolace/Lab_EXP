
#include <torch/script.h> // Cabeçalho principal do LibTorch
#include <opencv2/opencv.hpp> // Cabeçalhos do OpenCV
#include <iostream>
#include <memory>

int main() {
    // Carregar o modelo TorchScript
    std::shared_ptr<torch::jit::script::Module> module;
    try {
        module = torch::jit::load("model.pt");
    }
    catch (const c10::Error& e) {
        std::cerr << "Erro ao carregar o modelo\n";
        return -1;
    }

    std::cout << "Modelo carregado com sucesso\n";

    // Iniciar captura de vídeo
    cv::VideoCapture cap(0); // 0 é o ID da webcam padrão
    if (!cap.isOpened()) {
        std::cerr << "Erro ao abrir a webcam\n";
        return -1;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame; // Capturar o quadro atual

        if (frame.empty()) {
            std::cerr << "Quadro vazio\n";
            break;
        }

        // Pré-processamento da imagem
        cv::Mat grayscale;
        cv::cvtColor(frame, grayscale, cv::COLOR_BGR2GRAY); // Converter para escala de cinza

        // Redimensionar a imagem para 28x28
        cv::Mat resized;
        cv::resize(grayscale, resized, cv::Size(28, 28));

        // Normalizar os pixels para o intervalo [0, 1]
        resized.convertTo(resized, CV_32F, 1.0 / 255.0);

        // Converter o cv::Mat em tensor
        auto img_tensor = torch::from_blob(resized.data, {1, 1, 28, 28});

        // Ajustar a memória se necessário
        img_tensor = img_tensor.clone();

        // Executar o modelo
        torch::Tensor output = module->forward({img_tensor}).toTensor();

        // Obter a classe prevista
        int predicted_class = output.argmax(1).item<int>();

        // Mostrar o resultado
        std::cout << "Dígito previsto: " << predicted_class << std::endl;

        // Exibir a imagem capturada (opcional)
        cv::imshow("Webcam", frame);

        // Sair do loop se a tecla 'q' for pressionada
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    // Liberar recursos
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
