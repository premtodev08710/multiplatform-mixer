// Mix Practice Studio - Behringer X32 / X32 Rack Real-time OSC Network Client
#include "X32OscClient.h"

namespace mps
{

static void writePaddedString (juce::MemoryOutputStream& out, const juce::String& s)
{
    out.writeString (s);
    out.writeByte (0);
    while (out.getDataSize() % 4 != 0)
        out.writeByte (0);
}

static juce::String readPaddedString (const char* data, int maxLen, int& offset)
{
    if (offset >= maxLen) return {};
    juce::String s;
    int i = offset;
    while (i < maxLen && data[i] != 0)
    {
        s += data[i];
        ++i;
    }
    ++i;
    while (i % 4 != 0 && i < maxLen)
        ++i;
    offset = i;
    return s;
}

static float readBigEndianFloat (const char* data, int maxLen, int& offset)
{
    if (offset + 4 > maxLen) return 0.0f;
    uint32_t raw = 0;
    std::memcpy (&raw, data + offset, 4);
    raw = juce::ByteOrder::swapIfLittleEndian (raw);
    offset += 4;
    float val = 0.0f;
    std::memcpy (&val, &raw, 4);
    return val;
}

static int readBigEndianInt (const char* data, int maxLen, int& offset)
{
    if (offset + 4 > maxLen) return 0;
    uint32_t raw = 0;
    std::memcpy (&raw, data + offset, 4);
    raw = juce::ByteOrder::swapIfLittleEndian (raw);
    offset += 4;
    return (int) raw;
}

X32OscClient::X32OscClient (AudioEngine& eng, ProjectModel& proj)
    : juce::Thread ("X32OscReceiverThread"), engine (eng), project (proj)
{
}

X32OscClient::~X32OscClient()
{
    disconnect();
}

bool X32OscClient::connectToConsole (const juce::String& ipAddress, int port)
{
    disconnect();

    consoleIp = ipAddress.trim();
    consolePort = port;

    if (! socket.bindToPort (0))
    {
        if (! socket.bindToPort (10023))
            return false;
    }

    connected.store (true);
    startThread (juce::Thread::Priority::normal);
    startTimer (1000); // 1-second timer for keep-alive

    // Initial handshake
    sendOscMessage ("/xinfo");
    sendOscMessage ("/xremote");

    if (onConnectionChanged)
        onConnectionChanged (true);

    return true;
}

void X32OscClient::disconnect()
{
    if (connected.load())
    {
        connected.store (false);
        stopTimer();
        signalThreadShouldExit();
        socket.shutdown();
        waitForThreadToExit (1000);

        if (onConnectionChanged)
            onConnectionChanged (false);
    }
}

void X32OscClient::timerCallback()
{
    if (! connected.load()) return;

    ++keepAliveCounter;
    if (keepAliveCounter >= 8) // X32 requires /xremote every ~8-9 seconds
    {
        keepAliveCounter = 0;
        sendOscMessage ("/xremote");
    }
}

void X32OscClient::autoDiscover()
{
    if (! connected.load())
    {
        socket.bindToPort (0);
    }

    // Broadcast query to local subnet
    juce::MemoryOutputStream out;
    writePaddedString (out, "/xinfo");
    writePaddedString (out, ",");
    socket.write ("255.255.255.255", 10023, out.getData(), (int) out.getDataSize());
}

void X32OscClient::sendOscRaw (const juce::String& address, const juce::String& typeTag, const juce::MemoryBlock& args)
{
    if (! connected.load() && address != "/xinfo") return;

    juce::MemoryOutputStream out;
    writePaddedString (out, address);
    writePaddedString (out, typeTag);
    if (args.getSize() > 0)
        out.write (args.getData(), args.getSize());

    socket.write (consoleIp, consolePort, out.getData(), (int) out.getDataSize());
}

void X32OscClient::sendOscMessage (const juce::String& address)
{
    sendOscRaw (address, ",", {});
}

void X32OscClient::sendOscMessage (const juce::String& address, float value)
{
    juce::MemoryBlock args;
    uint32_t raw = 0;
    std::memcpy (&raw, &value, 4);
    raw = juce::ByteOrder::swapIfLittleEndian (raw);
    args.append (&raw, 4);

    sendOscRaw (address, ",f", args);
}

void X32OscClient::sendOscMessage (const juce::String& address, int value)
{
    juce::MemoryBlock args;
    uint32_t raw = juce::ByteOrder::swapIfLittleEndian ((uint32_t) value);
    args.append (&raw, 4);

    sendOscRaw (address, ",i", args);
}

void X32OscClient::sendOscMessage (const juce::String& address, const juce::String& str)
{
    juce::MemoryOutputStream out;
    writePaddedString (out, str);
    juce::MemoryBlock args (out.getData(), out.getDataSize());

    sendOscRaw (address, ",s", args);
}

void X32OscClient::sendFader (int channel, float faderDb)
{
    if (channel < 0 || channel >= kMaxInputChannels) return;
    const float pos = faderDbToPos (faderDb);
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/mix/fader";
    sendOscMessage (addr, pos);
}

void X32OscClient::sendMute (int channel, bool muted)
{
    if (channel < 0 || channel >= kMaxInputChannels) return;
    // In X32: ON = 1 (unmuted), OFF = 0 (muted)
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/mix/on";
    sendOscMessage (addr, muted ? 0 : 1);
}

void X32OscClient::sendPan (int channel, float pan)
{
    if (channel < 0 || channel >= kMaxInputChannels) return;
    const float norm = (pan + 1.0f) * 0.5f; // -1..+1 -> 0..1
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/mix/pan";
    sendOscMessage (addr, norm);
}

void X32OscClient::sendMainFader (float faderDb)
{
    sendOscMessage ("/main/st/mix/fader", faderDbToPos (faderDb));
}

void X32OscClient::sendMainMute (bool muted)
{
    sendOscMessage ("/main/st/mix/on", muted ? 0 : 1);
}

void X32OscClient::sendBusFader (int busIndex, float faderDb)
{
    if (busIndex < 0 || busIndex >= 16) return;
    const juce::String addr = "/bus/" + juce::String (busIndex + 1).paddedLeft ('0', 2) + "/mix/fader";
    sendOscMessage (addr, faderDbToPos (faderDb));
}

void X32OscClient::sendDcaFader (int dcaIndex, float faderDb)
{
    if (dcaIndex < 0 || dcaIndex >= 8) return;
    const juce::String addr = "/dca/" + juce::String (dcaIndex + 1) + "/fader";
    sendOscMessage (addr, faderDbToPos (faderDb));
}

void X32OscClient::sendChannelName (int channel, const juce::String& name)
{
    if (channel < 0 || channel >= kMaxInputChannels) return;
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/config/name";
    sendOscMessage (addr, name);
}

void X32OscClient::sendChannelColor (int channel, int colorIndex)
{
    if (channel < 0 || channel >= kMaxInputChannels) return;
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/config/color";
    sendOscMessage (addr, colorIndex);
}

void X32OscClient::sendBusSend (int channel, int busIndex, float sendDb)
{
    if (channel < 0 || channel >= kMaxInputChannels || busIndex < 0 || busIndex >= 16) return;
    const juce::String addr = "/ch/" + juce::String (channel + 1).paddedLeft ('0', 2) + "/mix/"
                            + juce::String (busIndex + 1).paddedLeft ('0', 2) + "/level";
    sendOscMessage (addr, faderDbToPos (sendDb));
}

void X32OscClient::run()
{
    char buffer[4096];
    juce::String senderIp;
    int senderPort = 0;

    while (! threadShouldExit() && connected.load())
    {
        if (socket.waitUntilReady (true, 50) > 0)
        {
            const int bytes = socket.read (buffer, sizeof (buffer), false, senderIp, senderPort);
            if (bytes > 0)
            {
                processIncomingOscPacket (buffer, bytes);
            }
        }
    }
}

void X32OscClient::processIncomingOscPacket (const char* data, int size)
{
    if (size < 4) return;

    int offset = 0;
    const juce::String address = readPaddedString (data, size, offset);
    if (! address.startsWithChar ('/')) return;

    const juce::String typeTag = readPaddedString (data, size, offset);

    // 1. Console Info response
    if (address == "/xinfo")
    {
        if (typeTag.length() >= 5) // ,ssss
        {
            const auto ip = readPaddedString (data, size, offset);
            consoleModel = readPaddedString (data, size, offset);
            firmwareVersion = readPaddedString (data, size, offset);
            const auto name = readPaddedString (data, size, offset);

            const auto infoStr = consoleModel + " (" + name + ") v" + firmwareVersion;
            juce::MessageManager::callAsync ([this, infoStr] {
                if (onConsoleIdentified)
                    onConsoleIdentified (infoStr);
            });
        }
        return;
    }

    // 2. Main Fader & Mute
    if (address == "/main/st/mix/fader" && typeTag.containsChar ('f'))
    {
        const float pos = readBigEndianFloat (data, size, offset);
        engine.getMixerParams().main.faderDb.set (faderPosToDb (pos));
        return;
    }
    if (address == "/main/st/mix/on" && typeTag.containsChar ('i'))
    {
        const int onVal = readBigEndianInt (data, size, offset);
        engine.getMixerParams().main.mute.setBool (onVal == 0);
        return;
    }

    // 3. Channels (/ch/XX/...)
    if (address.startsWith ("/ch/") && address.length() >= 7)
    {
        const int ch = address.substring (4, 6).getIntValue() - 1;
        if (ch >= 0 && ch < kMaxInputChannels)
        {
            const auto sub = address.substring (6);
            auto& cp = engine.getMixerParams().channels[(size_t) ch];

            if (sub == "/mix/fader" && typeTag.containsChar ('f'))
            {
                const float pos = readBigEndianFloat (data, size, offset);
                cp.faderDb.set (faderPosToDb (pos));
            }
            else if (sub == "/mix/on" && typeTag.containsChar ('i'))
            {
                const int onVal = readBigEndianInt (data, size, offset);
                cp.mute.setBool (onVal == 0);
            }
            else if (sub == "/mix/pan" && typeTag.containsChar ('f'))
            {
                const float norm = readBigEndianFloat (data, size, offset);
                cp.pan.set (norm * 2.0f - 1.0f);
            }
            else if (sub == "/config/name" && typeTag.containsChar ('s'))
            {
                const auto name = readPaddedString (data, size, offset);
                juce::MessageManager::callAsync ([this, ch, name] {
                    project.setChannelName (ch, name);
                });
            }
        }
    }
}

} // namespace mps
