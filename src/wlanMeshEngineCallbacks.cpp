#include "WlanMeshEngine.hpp"

#include "AnimationEngine.hpp"

#include "definitions.hpp"

#include "Arduino.h"
#include "ArduinoJson.h"
#include "cppQueue.h"

// #define DEBUG 1
// #define DEBUG_STATE 1
// #define DEBUG_STATE2 1
// #define DEBUG_JSON 1
// #define DEBUG_MSG 1
// #define DEBUG_STATE_CHANGED 1
// #define DEBUG_CORE 1

hw_timer_t* timerWlanMeshEngine1 = NULL;
hw_timer_t* timerWlanMeshEngine2 = NULL;
uint8_t preMinionCounter = 0;
uint8_t masterCounter = 0;

extern WlanMeshEngine wlanMeshEngine;
extern AnimationEngine animationEngine;

#ifdef DEBUG_STATE2
String states[] = {
    "WLAN_ON",
    "STANDALONE",
    "PRE_MASTER",
    "MASTER",
    "PRE_MINION",
    "MINION",
};
#endif

void messageReceivedCallback(uint32_t from, String& msg)
{
#ifdef DEBUG_CORE
    Serial.println("Core: " + String(xPortGetCoreID(), DEC));
#endif
    // deserialize the Message
    const size_t jsonSize = JSON_SIZE;
    StaticJsonDocument<jsonSize> jsonDoc;

    DeserializationError error = deserializeJson(jsonDoc, msg);
    if (error) {
        Serial.println("deserialize error");
    }

    MSG_TYPE msgType = convertIntToMSG_TYPE(jsonDoc["msgType"]);

    WLAN_MESH_STATE currentState = wlanMeshEngine.getCurrentState();

// DEBUG prints
#ifdef DEBUG_MSG
    Serial.println("message recieved: " + msg);
    Serial.println("msgType: " + String((convertMSG_TYPE_ToInt(msgType))));
#endif
#ifdef DEBUG_STATE
    Serial.println("STATE: " + states[(int)currentState]);
#endif

    switch (currentState) {
    // in state WLAN_ON
    case WLAN_MESH_STATE::WLAN_ON: {
// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: WLAN_ON");
#endif
        break;
    }
    // in state STANDALONE
    case WLAN_MESH_STATE::STANDALONE: {
// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: STANDALONE");
#endif
        break;
    };
    // in state PRE_MINION
    case WLAN_MESH_STATE::PRE_MINION: {
        if (msgType == MSG_TYPE::I_AM_MASTER) {
            if (timerAlarmEnabled(timerWlanMeshEngine1)) {
                timerAlarmDisable(timerWlanMeshEngine1);
                timerWrite(timerWlanMeshEngine1, 0);
            }

            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::MINION);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: MINION");
#endif
            wlanMeshEngine.setMasterAddr(from);
        }
// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: PRE_MINION");
#endif
        break;
    };
    // in state MINION
    case WLAN_MESH_STATE::MINION: {
        if (msgType == MSG_TYPE::EFFECT_MSG && wlanMeshEngine.getMasterAddr() == from) {
            JsonObject params = jsonDoc["params"];
            effect newEffect = {
                params["fxId"],
                params["R"],
                params["G"],
                params["B"],
                params["cycleMod"]
            };
            animationEngine.addNextAnimation(newEffect);
        } else if (msgType == MSG_TYPE::NEW_MASTER && wlanMeshEngine.getMasterAddr() != from) {
            wlanMeshEngine.setMasterAddr(from);
        }

// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: MINION");
#endif
        break;
    }
    // in state PRE_MASTER
    case WLAN_MESH_STATE::PRE_MASTER: {
// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: PRE_MASTER");
#endif
    } break;
    // in state MASTER
    case WLAN_MESH_STATE::MASTER: {
        if (msgType == MSG_TYPE::WHO_IS_MASTER) {
            wlanMeshEngine.answerIAmMaster(from);
        }
        if (msgType == MSG_TYPE::I_AM_MASTER) {
            wlanMeshEngine.masterNodes.push_back(from);
        }

// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: MASTER");
#endif
        break;
    }
    default: {
        wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::WLAN_ON);
#ifdef DEBUG_STATE_CHANGED
        Serial.println("STATE: WLAN_ON");
#endif
// DEBUG prints
#ifdef DEBUG_STATE
        Serial.println("switch-case: DEFAULT");
#endif
    }
    }
}

void meshTopologyChangedCallback()
{
#ifdef DEBUG_CORE
    Serial.println("Core: " + String(xPortGetCoreID(), DEC));
#endif
    WLAN_MESH_STATE currentState = wlanMeshEngine.getCurrentState();

// DEBUG prints
#ifdef DEBUG
    Serial.println("Mesh topology changed");
    Serial.println(wlanMeshEngine.subConnectionJson(true));
#endif
#ifdef DEBUG_STATE
    Serial.println("STATE: " + states[(int)wlanMeshEngine.getCurrentState()]);
#endif

    switch (currentState) {
    // in state WLAN_ON
    case WLAN_MESH_STATE::WLAN_ON: {
        if (wlanMeshEngine.getNodeList(true).size() == 2) {
            if (timerAlarmEnabled(timerWlanMeshEngine1)) {
                timerAlarmDisable(timerWlanMeshEngine1);
                timerWrite(timerWlanMeshEngine1, 0);
            }
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::PRE_MASTER);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MASTER");
#endif
            wlanMeshEngine.decideMaster();
        } else if (wlanMeshEngine.getNodeList(true).size() > 2) {
            if (timerAlarmEnabled(timerWlanMeshEngine1)) {
                timerAlarmDisable(timerWlanMeshEngine1);
                timerWrite(timerWlanMeshEngine1, 0);
            }

            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::PRE_MINION);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MINION");
#endif
            wlanMeshEngine.askWhoIsMaster();

            // set Timeout for 5 sec
            timerAlarmWrite(timerWlanMeshEngine1, 500000, true);
            timerAlarmEnable(timerWlanMeshEngine1);
        }
        break;
    }
    // in state STANDALONE
    case WLAN_MESH_STATE::STANDALONE: {
        if (wlanMeshEngine.getNodeList(true).size() == 2) {
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::PRE_MASTER);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MASTER");
#endif
            wlanMeshEngine.decideMaster();
        } else if (wlanMeshEngine.getNodeList(true).size() > 2) {
            if (timerAlarmEnabled(timerWlanMeshEngine1)) {
                timerAlarmDisable(timerWlanMeshEngine1);
                timerWrite(timerWlanMeshEngine1, 0);
            }

            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::PRE_MINION);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MINION");
#endif
            wlanMeshEngine.askWhoIsMaster();

            // set Timeout for 5 sec
            timerAlarmWrite(timerWlanMeshEngine1, 500000, true);
            timerAlarmEnable(timerWlanMeshEngine1);
        }
        break;
    }
    // in state PRE_MINION
    case WLAN_MESH_STATE::PRE_MINION: {
        if (wlanMeshEngine.getNodeList(true).size() == 1) {
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::STANDALONE);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: STANDALONE");
#endif
        }
        break;
    }
    // in state MINION
    case WLAN_MESH_STATE::MINION: {
        if (wlanMeshEngine.getNodeList(true).size() == 1) {
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::STANDALONE);
            wlanMeshEngine.masterNodes.clear();
            wlanMeshEngine.setMasterAddr(0);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: STANDALONE");
#endif
        } else if (wlanMeshEngine.getNodeList(true).size() >= 2) {
#ifdef DEBUG
            Serial.println("MASTER Addr: " + String(wlanMeshEngine.getMasterAddr(), DEC));
#endif
            std::list<uint32_t> currentNodes = wlanMeshEngine.getNodeList(true);
            auto findIt = std::find(
                currentNodes.begin(),
                currentNodes.end(),
                wlanMeshEngine.getMasterAddr());

            if (findIt == currentNodes.end()) {
#ifdef DEBUG
                Serial.println("MASTER is gone");
#endif
                wlanMeshEngine.setMasterAddr(0);
#ifdef DEBUG
                Serial.println("master to 0");
#endif
                wlanMeshEngine.masterNodes.clear();
#ifdef DEBUG
                Serial.println("masterNodes cleared");
#endif
                wlanMeshEngine.decideMaster();
#ifdef DEBUG
                Serial.println("masterNode new decided");
#endif
            }
        }
        break;
    }
    // in state PRE_MASTER
    case WLAN_MESH_STATE::PRE_MASTER: {
        if (wlanMeshEngine.getNodeList(true).size() == 1) {
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::STANDALONE);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: STANDALONE");
#endif
        }
        break;
    }
    // in state MASTER
    case WLAN_MESH_STATE::MASTER: {
        int32_t nodeDiff = wlanMeshEngine.getNodeList(true).size() - wlanMeshEngine.knownNodes.size();
        if (wlanMeshEngine.getNodeList(true).size() == 1) {
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::STANDALONE);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: STANDALONE");
#endif
        } else if (nodeDiff > 1) {
// ask who is else master
#ifdef DEBUG
            Serial.println("knownNodes: " + String(wlanMeshEngine.knownNodes.size(), DEC));
            Serial.println("new Mesh added; ask who is Master");
#endif
            wlanMeshEngine.askWhoIsMaster();

            // set Timeout for 5 sec
            timerAlarmWrite(timerWlanMeshEngine1, 500000, true);
            timerAlarmEnable(timerWlanMeshEngine1);
        }

        break;
    }
    default: {
    }
    }

#ifdef DEBUG_STATE
    Serial.println("STATE: " + states[(int)wlanMeshEngine.getCurrentState()]);
#endif
    wlanMeshEngine.knownNodes = wlanMeshEngine.getNodeList(true);
}

void onTimedOutCallback()
{
#ifdef DEBUG_CORE
    Serial.println("Core: " + String(xPortGetCoreID(), DEC));
#endif
    WLAN_MESH_STATE currentState = wlanMeshEngine.getCurrentState();
    // DEBUG prints
#ifdef DEBUG
    Serial.println("Timer Interrupt");
    Serial.println("STATE: " + states[(int)wlanMeshEngine.getCurrentState()]);
#endif

    if (currentState == WLAN_MESH_STATE::WLAN_ON) {
        // disable Timer
        timerAlarmDisable(timerWlanMeshEngine1);
        timerWrite(timerWlanMeshEngine1, 0);

        wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::STANDALONE);
#ifdef DEBUG_STATE_CHANGED
        Serial.println("STATE: STANDALONE");
#endif
    }

    else if (currentState == WLAN_MESH_STATE::PRE_MINION) {
        if (preMinionCounter >= 5) {
            // disable Timer
            timerAlarmDisable(timerWlanMeshEngine1);
            timerWrite(timerWlanMeshEngine1, 0);

            preMinionCounter = 0;
            wlanMeshEngine.setCurrentState(WLAN_MESH_STATE::PRE_MASTER);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MASTER");
#endif

            wlanMeshEngine.decideMaster();
            return;
        }
        wlanMeshEngine.askWhoIsMaster();
        preMinionCounter++;
    }

    else if (currentState == WLAN_MESH_STATE::MASTER) {
        if (masterCounter >= 5) {

            // disable Timer
            timerAlarmDisable(timerWlanMeshEngine1);
            timerWrite(timerWlanMeshEngine1, 0);

            masterCounter = 0;

            if (wlanMeshEngine.masterNodes.size() > 1) {
#ifdef DEBUG
                Serial.println("new Mesh added: decide");
#endif
                wlanMeshEngine.decideMaster();
            }

#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: PRE_MASTER");
#endif
            return;
        }
        masterCounter++;
    }
    // if there was mesh changes and timer has been started and the node get in the state where it doesn't need runing timers
    else {
        // disable Timer
        timerAlarmDisable(timerWlanMeshEngine1);
        timerWrite(timerWlanMeshEngine1, 0);
        masterCounter = 0;
        preMinionCounter = 0;
    }

#ifdef DEBUG
    Serial.println("STATE: " + states[(int)wlanMeshEngine.getCurrentState()]);
#endif
}
