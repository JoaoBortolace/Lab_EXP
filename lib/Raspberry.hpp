// Raspberry.hpp
#ifndef RASPBERRY_HPP
#define RASPBERRY_HPP

#define RASP_PINS

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>

using namespace cv;

#define xdebug { string st = "File="+string(__FILE__)+" line="+to_string(__LINE__)+"\n"; cout << st; }
#define xprint(x) { ostringstream os; os << #x " = " << x << '\n'; cout << os.str(); }

namespace Raspberry
{
    typedef uint8_t Byte;
    typedef uint8_t Gry;
    typedef Vec3b Cor;

    typedef enum
    {
        FRENTE,
        ATRAS,
        DIAGONAL_FRENTE_DIREITA,
        DIAGONAL_FRENTE_ESQUERDA,
        DIAGONAL_ATRAS_DIREITA,
        DIAGONAL_ATRAS_ESQUERDA,
        GIRA_ESQUERDA,
        GIRA_DIREITA,
        NAO_FAZ_NADA,
        NAO_SELECIONADO,
    } Teclado;

    inline double timeSinceEpoch(void) 
    {
        using namespace std::chrono;
        return duration_cast<duration<double>>( system_clock::now().time_since_epoch() ).count();
    }

    using namespace std;

    inline void erro(string s1="") 
    {
        cerr << s1 << endl;
        exit(1);
    }

    inline void print(string s1="")
    {
        cout << s1 << endl;
    }

    inline bool testaVb(const std::vector<Raspberry::Byte>& vb, Raspberry::Byte b) {
        for (unsigned i=0; i<vb.size(); i++)
            if (vb[i]!=b) { 
                return false;
            }
        return true;
    }
#ifdef RASP_PINS
#include <wiringPi.h>

#define M1_A    0
#define M1_B    1
#define M2_A    2
#define M2_B    3

    inline void motorInit()
    {
        pinMode (M1_A, OUTPUT);
        pinMode (M1_B, OUTPUT);
        pinMode (M2_A, OUTPUT);
        pinMode (M2_B, OUTPUT);

        digitalWrite(M1_A, LOW);
        digitalWrite(M1_B, LOW);
        digitalWrite(M2_A, LOW);
        digitalWrite(M2_B, LOW);
    }

    inline void motorSetDir(Teclado comando) 
    {
        switch (comando) {
            case FRENTE:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case ATRAS:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, HIGH);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, HIGH);
                break;
            case DIAGONAL_FRENTE_DIREITA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_FRENTE_ESQUERDA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_ATRAS_DIREITA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case DIAGONAL_ATRAS_ESQUERDA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
            case GIRA_ESQUERDA:
                digitalWrite(M1_A, HIGH);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, HIGH);
                break;
            case GIRA_DIREITA:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, HIGH);
                digitalWrite(M2_A, HIGH);
                digitalWrite(M2_B, LOW);
                break;
            case NAO_FAZ_NADA:            
            case NAO_SELECIONADO:
            default:
                digitalWrite(M1_A, LOW);
                digitalWrite(M1_B, LOW);
                digitalWrite(M2_A, LOW);
                digitalWrite(M2_B, LOW);
                break;
        }
    }
#endif
}

namespace Paleta 
{
    const Raspberry::Cor red(0x58, 0x48, 0xFF);
    const Raspberry::Cor grey(0x7F, 0x7F, 0x74);
    const Raspberry::Cor blue01(0x79, 0x7F, 0x1B);
    const Raspberry::Cor blue02(0xC0, 0xCC, 0x00);
    const Raspberry::Cor blue03(0xEB, 0xF2, 0x72);
}

#endif
