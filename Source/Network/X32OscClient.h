// Mix Practice Studio - Behringer X32 / X32 Rack Real-time OSC Network Client
#pragma once

#include "../Audio/AudioEngine.h"
#include "../Model/ProjectModel.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <atomic>
#include <functional>

namespace mps
{

class X32OscClient : private juce::Thread, private juce::Timer
{
public:
    X32OscClient (AudioEngine& engine, ProjectModel& project);
    ~X32OscClient() override;

    bool connectToConsole (const juce::String& ipAddress, int port = 10023);
    void disconnect();

    bool isConnected() const noexcept { return connected.load(); }
    const juce::String& getConsoleIp() const noexcept { return consoleIp; }
    int getConsolePort() const noexcept { return consolePort; }

    const juce::String& getConsoleModel() const noexcept { return consoleModel; }
    const juce::String& getFirmwareVersion() const noexcept { return firmwareVersion; }

    // Outgoing commands to hardware console
    void sendFader (int channel, float faderDb);
    void sendMute (int channel, bool muted);
    void sendPan (int channel, float pan);
    void sendMainFader (float faderDb);
    void sendMainMute (bool muted);
    void sendBusFader (int busIndex, float faderDb);
    void sendDcaFader (int dcaIndex, float faderDb);
    void sendChannelName (int channel, const juce::String& name);
    void sendChannelColor (int channel, int colorIndex);
    void sendBusSend (int channel, int busIndex, float sendDb);

    // Auto-discover X32 on local network
    void autoDiscover();

    std::function<void (bool connected)> onConnectionChanged;
    std::function<void (const juce::String& info)> onConsoleIdentified;

private:
    void run() override;
    void timerCallback() override;

    void sendOscRaw (const juce::String& address, const juce::String& typeTag, const juce::MemoryBlock& args);
    void sendOscMessage (const juce::String& address);
    void sendOscMessage (const juce::String& address, float value);
    void sendOscMessage (const juce::String& address, int value);
    void sendOscMessage (const juce::String& address, const juce::String& str);

    void processIncomingOscPacket (const char* data, int size);

    AudioEngine& engine;
    ProjectModel& project;

    juce::DatagramSocket socket { false };
    juce::String consoleIp;
    int consolePort = 10023;

    std::atomic<bool> connected { false };
    std::atomic<bool> ignoreIncoming { false };
    int keepAliveCounter = 0;

    juce::String consoleModel { "Not Connected" };
    juce::String firmwareVersion;
};

} // namespace mps
