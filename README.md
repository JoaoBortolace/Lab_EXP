# Processamento de Imagens com a Raspberry Pi 3b (projeto da EPUSP~PSI)

Este projeto implementa um sistema baseado em OpenCV que se comunica com uma Raspberry Pi 3b. Ele processa imagens capturadas pela câmera da Pi, realiza operações de template matching (correspondência de modelos) e envia comandos de controle de volta para a Pi, como parte de um sistema automatizado de controle de robôs. Mais tarde, o sistema é evoluído para utilizar o PyTorch, classificando, baseado no LeNet-5, imagens dos digitos MNIST.

**Visão e Função:**
- Recebe imagens da câmera da Raspberry Pi.
- Realiza processamento em tempo real, como template matching, para detectar e seguir um objeto específico.
- Envia comandos para controlar motores com base na posição do objeto detectado.
- Suporta modos de operação manual e automático, alternando entre eles conforme os comandos do usuário.
- Exibe a imagem processada com indicações visuais de controle.

## Como Funciona o Programa de Processamento de Imagens

### Estrutura Geral
1. **Recebimento de Imagens:** As imagens são recebidas da Raspberry Pi e armazenadas em buffers.
2. **Processamento:** 
   - As imagens são convertidas para escala de cinza e redimensionadas.
   - Um algoritmo de template matching é executado para identificar a localização do objeto de interesse.
   - Se a correlação da correspondência for alta o suficiente, são calculadas velocidades para os motores, que são enviadas para a Pi.
3. **Modo de Operação:**
   - **Manual:** Controle direto baseado na entrada do usuário.
   - **Automático:** O sistema toma decisões com base na detecção de objetos.
4. **Exibição:** As imagens processadas são exibidas em uma janela, mostrando as informações de controle em tempo real.

---


![gifzinhooo](https://media.giphy.com/media/iGJNOadhvBMuk/giphy.gif)

---

