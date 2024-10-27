#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Device.hpp"

class Client : public Device
{
    public:
        Client(const char* serverName, const char* port);
        ~Client();
        
        void waitConnection();
        
        void sendBytes(uint32_t numBytes, const Raspberry::Byte* txBuffer);
        void receiveBytes(uint32_t numBytes, Raspberry::Byte* rxBuffer);
};

#endif
