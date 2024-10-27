#include "Device.hpp"

/*
 * Envia um número inteiro sem sinal de 32 bits (4 bytes) na forma Big-Endian
 */
void Device::sendUInt(const uint32_t value)
{
    uint32_t net_value = htonl(value);
    this->sendBytes(sizeof(uint32_t), (const Raspberry::Byte*) &net_value);
}

/*
 * Recebe um número inteiro sem sinal de 32 bits (4 bytes) que foi transmitido na forma Big-Endian - Aguarda até o recebimento ou timeout do servidor 
 */
void Device::receiveUInt(uint32_t& value)
{
    uint32_t net_value;
    this->receiveBytes(sizeof(uint32_t), (Raspberry::Byte*) &net_value);
    value = ntohl(net_value);
}

/*
 * Transmite um vector de bytes
 */
void Device::sendVectorByte(const std::vector<Raspberry::Byte>& vec)
{
    // Primeiro transmite o tamanho do vector
    uint32_t vecSize = vec.size(); 
    this->sendUInt(vecSize);
    
    this->sendBytes(vecSize, vec.data());
}

/*
 * Recebe um vector de bytes - Aguarda até o recebimento ou timeout do servidor 
 */
void Device::receiveVectorByte(std::vector<Raspberry::Byte>& vec)
{
    // Recebe o tamanho do vector
    uint32_t vecSize; 
    this->receiveUInt(vecSize);

    // Redimensiona o array
    vec.resize(vecSize);

    this->receiveBytes(vecSize, vec.data());
}

/*
 * Envia uma imagem colorida (BGR-8bits) não compactada, caso não seja continua joga um excessão
 */
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
        throw std::runtime_error("Erro: A imagem não é continua!");
    }
}

/*
 * Recebe uma imagem colorida (BGR-8bits) não compactada - Aguarda até o recebimento ou timeout do servidor 
 */
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

/*
 * Envia uma imagem colorida (BGR-8bits) compactada em jpeg, com um fator definido no atributo compressaoParam
 */
void Device::sendImageCompactada(const Mat_<Raspberry::Cor>& image)
{
    // Comprime a imagem
    imencode(".jpeg", image, imgBuf, compressaoParam);

    // Transfere o buffer
    this->sendVectorByte(imgBuf);
}

/*
 * Recebe uma imagem colorida (BGR-8bits) compactada em jpeg, retorna esta descompactada
 */
void Device::receiveImageCompactada(Mat_<Raspberry::Cor>& image)
{
    // Recebe a imagem
    this->receiveVectorByte(imgBuf);
    image = imdecode(imgBuf, 1);
}

/*
 * Defini o fator de compressão, satura nos limites [0, 100]
 */
void Device::setCompressaoQualidade(int8_t porcentagemComp)
{
    compressaoParam[1] = porcentagemComp > 100 ? 100 : porcentagemComp < 0 ? 0 : porcentagemComp;
}
