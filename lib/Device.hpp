#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <exception>

#include "Raspberry.hpp"

#define SOCKET_ERROR -1
#define CHUNK_SIZE  (size_t) 65535

class Device
{
    protected:
        struct sockaddr_in transferAddr;
        int transferSocket = SOCKET_ERROR;

        std::vector<Raspberry::Byte> imgBuf;
        std::vector<int> compressaoParam{IMWRITE_JPEG_QUALITY, 80};
    public:
        virtual void waitConnection() = 0;
        
        virtual void sendBytes(uint32_t numBytes, const Raspberry::Byte* txBuffer) = 0;
        virtual void receiveBytes(uint32_t numBytes, Raspberry::Byte* rxBuffer) = 0;

        void setCompressaoQualidade(int8_t porcentagemComp);

        void sendUInt(const uint32_t value);
        void receiveUInt(uint32_t& value);

        void sendVectorByte(const std::vector<Raspberry::Byte>& vec);
        void receiveVectorByte(std::vector<Raspberry::Byte>& vec);

        void sendImage(const Mat_<Raspberry::Cor>& image);
        void receiveImage(Mat_<Raspberry::Cor>& image);

        void sendImageCompactada(const Mat_<Raspberry::Cor>& image);
        void receiveImageCompactada(Mat_<Raspberry::Cor>& image);
};

#endif 