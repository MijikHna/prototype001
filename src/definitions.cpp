#include "definitions.hpp"

//#define DEBUG 1

int convertMSG_TYPE_ToInt(MSG_TYPE msgType)
{
#ifdef DEBUG
    Serial.println("converted: " + String((int)msgType, DEC));
#endif

    return (int)msgType;
}

MSG_TYPE convertIntToMSG_TYPE(int msgType)
{
    MSG_TYPE result = MSG_TYPE::NO_MSG_TYPE;

    switch (msgType) {
    case 0:
        break;
    case 1:
        result = MSG_TYPE::EFFECT_MSG;
        break;
    case 2:
        result = MSG_TYPE::WHO_IS_MASTER;
        break;
    case 3:
        result = MSG_TYPE::I_AM_MASTER;
        break;
    case 4:
        result = MSG_TYPE::ACK;
        break;
    case 5:
        result = MSG_TYPE::EFFECT_QUEUE;
        break;
    case 6:
        result = MSG_TYPE::NEW_MASTER;
        break;
    };

#ifdef DEBUG
    Serial.println("msgType (int): " + String(msgType, DEC));
    Serial.println("converted: " + String((int)result, DEC));
#endif

    return result;
}