#pragma once

#include <string>
#include <vector>
#include <unordered_map>
//
#include "GoodComponents.h"
#include "ringbuffer.h"

using namespace std;

static constexpr int m_ringBufferSize = 100;

class PlayerInput : public GoodSerializable
{
public:
    GoodId m_playerEntityId;
    int m_frameIndex;

    int m_someInput;
    bool m_someOtherInput;

    GOOD_SERIALIZABLE(
        PlayerInput, GOOD_VERSION(1)
        , GOOD(m_playerEntityId)
        , GOOD(m_frameIndex)
        , GOOD(m_someInput)
        , GOOD(m_someOtherInput)
    );

};

class PlayerInputs
{
public:
    unordered_map<GoodId, PlayerInput> m_playerInputForEntityId;

};


class SimulationState
{
public:
    ComponentGrid m_state;
};





class Simulation
{

public:


    void ReceivePlayerInput(const PlayerInput& input)
    {
        int frame = input.m_frameIndex;
        GoodId playerEntityId = input.m_playerEntityId;

        while (m_playerInputs.GetCurrentIndex() < frame)
        {
            m_playerInputs.Push();
        }
        
        unordered_map<GoodId, PlayerInput>& inputsForThisFrame = m_playerInputs.Get(frame);

        inputsForThisFrame[playerEntityId] = input;
    }

    void Update()
    {
        m_currentFrameIndex++;

        while (m_playerInputs.GetCurrentIndex() < m_currentFrameIndex)
        {
            m_playerInputs.Push();
        }

    }

    // call update, then work on state looking at currentplayerinput
    SimulationState& GetState()
    {
        return m_simulationState;
    }

    const unordered_map<GoodId, PlayerInput>& GetCurrentPlayerInputs() const
    {
        return m_playerInputs.Get(m_currentFrameIndex);
    }

private:
    int m_currentFrameIndex = 0;

    SimulationState m_simulationState;

    RingBuffer<unordered_map<GoodId, PlayerInput>, m_ringBufferSize> m_playerInputs;



};

