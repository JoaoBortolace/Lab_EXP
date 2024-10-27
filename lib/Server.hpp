#ifndef SERVER_HPP
#define SERVER_HPP

#include "Device.hpp"

class Server : public Device
{
    private:
        int serverSocket = SOCKET_ERROR;
    public:
        Server(const char* port, time_t timeout);
        ~Server();
        
        void waitConnection();
        
        void sendBytes(uint32_t numBytes, const Raspberry::Byte* txBuffer);
        void receiveBytes(uint32_t numBytes, Raspberry::Byte* rxBuffer);
};

#endif
