#ifndef __WLAN_MESH_ENGINE__
#define __WLAN_MESH_ENGINE__

#include "definitions.hpp"

#include "Arduino.h"
#include "cppQueue.h"
#include "painlessMesh.h"

class WlanMeshEngine : public painlessMesh {
private:
public:
    uint32_t masterAddr;
    WLAN_MESH_STATE currentState;
    bool calcDelay = false;
    SimpleList<uint32_t> knownNodes;
    SimpleList<uint32_t> masterNodes;
    // Constructors
    WlanMeshEngine();

    // getter + setter
    uint32_t getMasterAddr();
    void setMasterAddr(uint32_t masterAddr);
    WLAN_MESH_STATE getCurrentState();
    void setCurrentState(WLAN_MESH_STATE newState);

    // methods
    void askWhoIsMaster();
    void answerIAmMaster(uint32_t dest);
    void setStandalone();
    void decideMaster();
    void broadcastEffect(effect FX);
    void sendQueue(uint32_t dest, cppQueue q);
    void sendAck(uint32_t dest);
    void broadcastIAmNewMaster();

    // overrides
    void init(TSTRING ssid, TSTRING password, Scheduler* scheduler,
        uint16_t port);
    void update(void);
};
#endif