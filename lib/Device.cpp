#include "Device.hpp"

void Device::sendUInt(const uint32_t value)
{
    uint32_t net_value = htonl(value);
    this->sendBytes(sizeof(uint32_t), (const Raspberry::Byte*) &net_value);
}

void Device::receiveUInt(uint32_t& value)
{
    uint32_t net_value;
    this->receiveBytes(sizeof(uint32_t), (Raspberry::Byte*) &net_value);
    value = ntohl(net_value);
}

void Device::sendVectorByte(const std::vector<Raspberry::Byte>& vec)
{
    // Primeiro transmite o tamanho do vector
    uint32_t vecSize = vec.size(); 
    this->sendUInt(vecSize);
    
    this->sendBytes(vecSize, vec.data());
}

void Device::receiveVectorByte(std::vector<Raspberry::Byte>& vec)
{
    // Recebe o tamanho do vector
    uint32_t vecSize; 
    this->receiveUInt(vecSize);

    // Redimensiona o array
    vec.resize(vecSize);

    this->receiveBytes(vecSize, vec.data());
}

void Device::sendImage(const Mat_<Raspberry::Cor>& image)
{
    if (image.isContinuous()) {
        // Envia o tamanho da imagem
        this->sendUInt(image.rows);
        this->sendUInt(image.cols);

        // Envia o tamanho total da imagem
        this->sendBytes(3*image.total(), image.data);
    }
    else {
        Raspberry::erro("Erro: A imagem não é continua!");
    }
}

void Device::receiveImage(Mat_<Raspberry::Cor>& image)
{
    // Recebe o tamanho da imagem
    u_int32_t numColunas, numLinhas;
    this->receiveUInt(numLinhas);
    this->receiveUInt(numColunas);

    // Redimensiona a imagem e recebe
    image.create(numLinhas, numColunas);
    this->receiveBytes(3*image.total(), image.data);
}

void Device::sendImageCompactada(const Mat_<Raspberry::Cor>& image)
{
    // Comprime a imagem
    imencode(".jpeg", image, imgBuf, compressaoParam);

    // Transfere o buffer
    this->sendVectorByte(imgBuf);
}

void Device::receiveImageCompactada(Mat_<Raspberry::Cor>& image)
{
    // Recebe a imagem
    this->receiveVectorByte(imgBuf);
    image = imdecode(imgBuf, 1);
}

void Device::setCompressaoQualidade(uint8_t porcentagemComp)
{
    porcentagemComp > 100 ? compressaoParam[1] = 100 : compressaoParam[1] = porcentagemComp;
}
