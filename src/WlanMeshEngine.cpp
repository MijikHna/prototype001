#include "WlanMeshEngine.hpp"

#include "AnimationEngine.hpp"
#include "ArduinoJson.h"
#include "definitions.hpp"

// #define DEBUG 1
// #define DEBUG_STATE 1
// #define DEBUG_JSON 1
// #define DEBUG_STATE_CHANGED 1

extern hw_timer_t* timerWlanMeshEngine1;
extern hw_timer_t* timerWlanMeshEngine2;

#ifdef DEBUG_STATE
extern String states[];
#endif

extern AnimationEngine animationEngine;

// declaration of callback functions
void messageReceivedCallback(uint32_t from, String& msg);
void meshTopologyChangedCallback();
void onTimedOutCallback();

// Constructors
WlanMeshEngine::WlanMeshEngine()
    : painlessMesh()
{
    this->masterAddr = 0;
    this->currentState = WLAN_MESH_STATE::WLAN_ON;
    this->calcDelay = false;
    this->knownNodes = SimpleList<uint32_t>();
    this->masterNodes = SimpleList<uint32_t>();
}

// getter + setter
uint32_t WlanMeshEngine::getMasterAddr() { return this->masterAddr; }

void WlanMeshEngine::setMasterAddr(uint32_t masterAddr)
{
    this->masterAddr = masterAddr;
}

WLAN_MESH_STATE
WlanMeshEngine::getCurrentState() { return this->currentState; }

void WlanMeshEngine::setCurrentState(WLAN_MESH_STATE newState)
{
    this->currentState = newState;
}

// methods
void WlanMeshEngine::askWhoIsMaster()
{
    const size_t capacity = JSON_SIZE;
    StaticJsonDocument<capacity> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::WHO_IS_MASTER);
#ifdef DEBUG_STATE
    jsonDoc["nodeId"] = this->getNodeId();
    jsonDoc["STATE"] = states[(int)this->getCurrentState()];
#endif

    String msg = "";
    serializeJson(jsonDoc, msg);

    this->sendBroadcast(msg);
#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}

void WlanMeshEngine::answerIAmMaster(uint32_t dest)
{
    const size_t jsonSize = JSON_SIZE;
    StaticJsonDocument<jsonSize> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::I_AM_MASTER);
#ifdef DEBUG_STATE
    jsonDoc["nodeId"] = this->getNodeId();
    jsonDoc["STATE"] = states[(int)this->getCurrentState()];
#endif

    String msg = "";
    serializeJson(jsonDoc, msg);

    this->sendSingle(dest, msg);
#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}

void WlanMeshEngine::setStandalone()
{
    this->currentState = WLAN_MESH_STATE::STANDALONE;
}

void WlanMeshEngine::decideMaster()
{
#ifdef DEBUG
    Serial.println("Master Nodes Size: " + String(this->masterNodes.size(), DEC));
#endif
    if (this->masterNodes.size() == 1 || this->masterNodes.size() == 0) {

        std::list<uint32_t> nodes = this->getNodeList(true);
        uint32_t ownNodeAddress = this->getNodeId();
        uint32_t min = nodes.front();

#ifdef DEBUG
        Serial.println("OWN: " + String(ownNodeAddress, DEC));
#endif

        for (const auto& it : nodes) {
#ifdef DEBUG
            Serial.println("it: " + String(it, DEC));
#endif

            if (it < min) {
                min = it;
            }
        }

        if (min == ownNodeAddress) {
            this->setCurrentState(WLAN_MESH_STATE::MASTER);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: MASTER");
#endif
        } else {
            this->setCurrentState(WLAN_MESH_STATE::MINION);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: MINION");
#endif
        }

        this->setMasterAddr(min);
        this->masterNodes.push_back(min);
    } else if (this->masterNodes.size() > 1) {
        std::list<uint32_t> nodes = this->masterNodes;
        uint32_t ownNodeAddress = this->getNodeId();
        uint32_t min = nodes.front();

        for (const auto& it : nodes) {
            if (it < min) {
                min = it;
            }
        }

        if (min == ownNodeAddress) {
            this->setCurrentState(WLAN_MESH_STATE::MASTER);
            this->broadcastIAmNewMaster();
        } else {
            this->setCurrentState(WLAN_MESH_STATE::MINION);
#ifdef DEBUG_STATE_CHANGED
            Serial.println("STATE: MINION");
#endif
        }

        this->setMasterAddr(min);
        this->masterNodes.clear();
        this->masterNodes.push_back(min);
    }
}

void WlanMeshEngine::broadcastEffect(effect FX)
{
    const size_t jsonSize = JSON_SIZE;
    StaticJsonDocument<jsonSize> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::EFFECT_MSG);

    JsonObject params = jsonDoc.createNestedObject("params");

    params["fxId"] = FX.fxId;
    params["R"] = FX.red;
    params["G"] = FX.green;
    params["B"] = FX.blue;
    params["cycleMod"] = FX.cycleMod;
#ifdef DEBUG_STATE
    jsonDoc["nodeId"] = this->getNodeId();
    jsonDoc["STATE"] = states[(int)this->getCurrentState()];
#endif

    String msg = "";
    serializeJson(jsonDoc, msg);

    this->sendBroadcast(msg);

    if (this->calcDelay) {
        SimpleList<uint32_t>::iterator node = this->knownNodes.begin();
        while (node != this->knownNodes.end()) {
            this->startDelayMeas(*node);
            node++;
        }
        this->calcDelay = false;
    }

#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}

void WlanMeshEngine::sendQueue(uint32_t dest, cppQueue fx)
{
    const size_t jsonSize = JSON_SIZE * 10;
    StaticJsonDocument<jsonSize> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::EFFECT_QUEUE);
    jsonDoc["length"] = (int)fx.getCount();

    JsonObject params = jsonDoc.createNestedObject("params");

    effect eff;
    for (int i = 0; i < fx.getCount(); i++) {
        JsonObject param = params.createNestedObject(String(i, DEC));

        fx.peekIdx(&eff, i);

        param["fxId"] = eff.fxId;
        param["R"] = eff.red;
        param["G"] = eff.green;
        param["B"] = eff.blue;
        param["cycleMod"] = eff.cycleMod;
    }

    String msg = "";
    serializeJson(jsonDoc, msg);

#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}
void WlanMeshEngine::sendAck(uint32_t dest)
{
    const size_t jsonSize = JSON_SIZE;
    StaticJsonDocument<jsonSize> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::ACK);

    String msg = "";
    serializeJson(jsonDoc, msg);

    this->sendSingle(dest, msg);
#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}

// overrides
void WlanMeshEngine::init(TSTRING ssid, TSTRING password, Scheduler* scheduler,
    uint16_t port)
{
    painlessMesh::init(ssid, password, scheduler, port);
#ifdef DEBUG
    Serial.println("painlessMesh init done");
#endif

    // Callbacks setzen
    this->onReceive(messageReceivedCallback);
    this->onChangedConnections(meshTopologyChangedCallback);

    // TimerOut Interrupt
    timerWlanMeshEngine1 = timerBegin(0, 800, true);
    timerAttachInterrupt(timerWlanMeshEngine1, onTimedOutCallback, true);
    timerAlarmWrite(timerWlanMeshEngine1, 5000000,
        true); // set Timer for 50 sec, so within the Node can
    // connect to Mesh within this time
    timerAlarmEnable(timerWlanMeshEngine1);
}

void WlanMeshEngine::update(void) { painlessMesh::update(); }

void WlanMeshEngine::broadcastIAmNewMaster()
{
    const size_t jsonSize = JSON_SIZE;
    StaticJsonDocument<jsonSize> jsonDoc;

    jsonDoc["msgType"] = convertMSG_TYPE_ToInt(MSG_TYPE::NEW_MASTER);
#ifdef DEBUG_STATE
    jsonDoc["nodeId"] = this->getNodeId();
    jsonDoc["STATE"] = states[(int)this->getCurrentState()];
#endif

    String msg = "";
    serializeJson(jsonDoc, msg);

    this->sendBroadcast(msg);
#ifdef DEBUG_JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
#endif
}