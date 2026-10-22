// JT-4000 Controller controller plugin (Studio 87). Sends and receives the MIDI CC / NRPN listed in map/params.csv.
// Parameters are automatable. Incoming CC from the device moves the parameters; the plugin never echoes it back.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <memory>
#include "BinaryData.h"
#include "MidiCore.h"
#include "catui/CatFrame.h"
#include "catui/RenderGate.h"
#include <array>
#include <set>

namespace
{
constexpr int kMaxParams = 400;     // keeps the host's parameter list reasonable
constexpr int kPerBlock = 16;       // at most this many parameters are sent per audio block, so a device is never flooded

ctl::ParseResult loadMap()
{
    return ctl::parseMap(std::string(BinaryData::params_csv, size_t(BinaryData::params_csvSize)));
}
}

class CtlProcessor : public juce::AudioProcessor, private juce::AudioProcessorParameter::Listener, private juce::Timer, private juce::MidiInputCallback
{
public:
    CtlProcessor() : AudioProcessor(BusesProperties()), mapRes(loadMap()), decoder(mapRes.params)
    {
        auto ch = std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ "midi_channel", 1 }, "MIDI channel", 1, 16, 1);
        channelParam = ch.get(); addParameter(ch.release());
        for (auto& p : mapRes.params)
        {
            if (p.kind == ctl::Kind::Pair || int(slots.size()) >= kMaxParams) continue;
            auto ip = std::make_unique<juce::AudioParameterInt>(juce::ParameterID{ p.id, 1 }, p.name, p.minV, p.maxV, p.defV);
            Slot s; s.param = ip.get(); s.map = int(&p - mapRes.params.data()); slots.push_back(s);
            addParameter(ip.release());
        }
        reserved = ctl::reservedCCs(mapRes.params);
        pending = std::make_unique<std::atomic<int>[]>(slots.size()); learnedIn = std::make_unique<std::atomic<int>[]>(slots.size()); lastRecv = std::make_unique<std::atomic<int>[]>(slots.size()); incoming = std::make_unique<std::atomic<int>[]>(slots.size());
        for (size_t i = 0; i < slots.size(); ++i) { learnedIn[i] = -1; pending[i] = -1; lastRecv[i] = -1; incoming[i] = -1; slots[i].param->addListener(this); }
        collector.reset(48000); startTimerHz(30);
    }
    ~CtlProcessor() override { stopTimer(); devIn.reset(); { juce::SpinLock::ScopedLockType l(devLock); devOut.reset(); } for (auto& s : slots) s.param->removeListener(this); }

    const ctl::ParseResult mapRes;
    struct Slot { juce::AudioParameterInt* param = nullptr; int map = 0; };
    std::vector<Slot> slots;
    juce::AudioParameterInt* channelParam = nullptr;

    void sendAll() { for (size_t i = 0; i < slots.size(); ++i) pending[i] = slots[i].param->get(); }

    // MIDI learn: a CC from another controller can move one parameter. The UI arms a slot; the next free CC binds to it.
    ctl::LearnMap learn;
    std::atomic<int> learnTarget{ -1 };

    void prepareToPlay(double sr, int) override { decoder.reset(); collector.reset(sr > 0 ? sr : 48000.0); }

    // Direct connection: the plugin looks for the JT-4000 among the system MIDI ports and talks to it itself, so no DAW routing is needed.
    bool deviceConnected() const { return connected.load(); }
    juce::String deviceName() const { return devNameStr; }
    void scanNow() { scanDevices(); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }

    void processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals nd; audio.clear();
        { juce::MidiBuffer dev; collector.removeNextBlockOfMessages(dev, juce::jmax(1, audio.getNumSamples()));   // messages that arrived from the directly connected device
          for (const auto m : dev) midi.addEvent(m.getMessage(), juce::jmin(m.samplePosition, juce::jmax(0, audio.getNumSamples() - 1))); }
        // 1. incoming from the device: CC messages are consumed here, everything else passes through unchanged
        juce::MidiBuffer passthru;
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isController())
            {
                const int ccn = msg.getControllerNumber(); const int target = learnTarget.load();
                if (target >= 0 && ! reserved[size_t(ccn)]) { learn.bind(ccn, target); learnTarget = -1; continue; }   // MIDI learn: this CC now drives that knob
                const int ls = learn.slotFor(ccn);
                if (ls >= 0 && ls < int(slots.size())) { learnedIn[size_t(ls)] = msg.getControllerValue(); continue; }
                if (auto e = decoder.feedCC(ccn, msg.getControllerValue()))
                {
                    for (size_t i = 0; i < slots.size(); ++i) if (slots[i].map == e->paramIndex) { incoming[i] = e->value; break; }
                }
                else if (ccn != 6 && ccn != 98 && ccn != 99) passthru.addEvent(msg, meta.samplePosition);   // CCs that are not in the map (sustain pedal, ...) pass through
            }
            else passthru.addEvent(msg, meta.samplePosition);
        }
        midi.swapWith(passthru);
        // 2. outgoing: changed parameters, a few per block
        const int ch = channelParam->get(); const int n = int(slots.size()); int sent = 0, pos = 0; juce::MidiBuffer outgoing;
        for (int k = 0; k < n && sent < kPerBlock; ++k)
        {
            const int i = (rotate + k) % n; const int v = pending[size_t(i)].exchange(-1); if (v < 0) continue;
            for (const auto& m : ctl::encode(mapRes.params[size_t(slots[size_t(i)].map)], ch, v))
            { const juce::uint8 bytes[3] = { m.status, m.d1, m.d2 }; outgoing.addEvent(bytes, 3, juce::jmin(pos++, juce::jmax(0, audio.getNumSamples() - 1))); }
            ++sent; rotate = (i + 1) % n;
        }
        if (! outgoing.isEmpty())
        {
            bool direct = false;
            { juce::SpinLock::ScopedTryLockType l(devLock); if (l.isLocked() && devOut) { devOut->sendBlockOfMessagesNow(outgoing); direct = true; } }
            if (! direct) for (const auto m : outgoing) midi.addEvent(m.getMessage(), m.samplePosition);   // no device found: send to the host's MIDI output instead
        }
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "JT-4000 Controller"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override
    {
        juce::XmlElement x("STATE"); x.setAttribute("v", 1); x.setAttribute("midi_channel", channelParam->get());
        for (auto& s : slots) x.setAttribute(s.param->paramID, s.param->get());
        for (size_t i = 0; i < slots.size(); ++i) { const int c = learn.ccFor(int(i)); if (c >= 0) x.setAttribute("learn_" + slots[i].param->paramID, c); }
        copyXmlToBinary(x, dest);
    }
    void setStateInformation(const void* data, int size) override
    {
        if (auto x = getXmlFromBinary(data, size))
        {
            if (!x->hasTagName("STATE")) return;
            const int ch = x->getIntAttribute("midi_channel", 1); *channelParam = juce::jlimit(1, 16, ch);
            for (auto& s : slots) if (x->hasAttribute(s.param->paramID)) setFromState(s, x->getIntAttribute(s.param->paramID));
            learn.clear();
            for (size_t i = 0; i < slots.size(); ++i) { const auto key = "learn_" + slots[i].param->paramID; if (x->hasAttribute(key)) { const int c = x->getIntAttribute(key, -1); if (c >= 0 && c < 128 && ! reserved[size_t(c)]) learn.bind(c, int(i)); } }
        }
    }

private:
    void setFromState(Slot& s, int v)
    {
        const size_t i = size_t(&s - slots.data()); const auto r = s.param->getRange();
        v = juce::jlimit(r.getStart(), r.getEnd(), v); lastRecv[i] = v; *s.param = v;   // restoring a project does not send MIDI
    }
    void parameterValueChanged(int, float) override {}
    void parameterGestureChanged(int, bool) override {}
    void timerCallback() override
    {
        if (++scanTick >= 60) { scanTick = 0; scanDevices(); }
        if (lastSeen.size() != slots.size()) { lastSeen.resize(slots.size()); for (size_t i = 0; i < slots.size(); ++i) lastSeen[i] = slots[i].param->get(); }
        // device to host: a parameter set from incoming MIDI is never sent back out
        for (size_t i = 0; i < slots.size(); ++i)
        {
            const int v = incoming[i].exchange(-1); if (v < 0 || slots[i].param->get() == v) continue;
            lastRecv[i] = v; *slots[i].param = v;
        }
        // learned controller to parameter: set it the way the UI does, so it is also sent on to the device
        for (size_t i = 0; i < slots.size(); ++i)
        {
            const int v = learnedIn[i].exchange(-1); if (v < 0) continue;
            const auto r = slots[i].param->getRange(); const int c = juce::jlimit(r.getStart(), r.getEnd(), v);
            if (slots[i].param->get() != c) { lastRecv[i] = -1; *slots[i].param = c; }
        }
        // host or UI to device
        for (size_t i = 0; i < slots.size(); ++i)
        {
            const int v = slots[i].param->get(); if (v == lastSeen[i]) continue;
            lastSeen[i] = v;
            if (lastRecv[i].load() == v) lastRecv[i] = -1; else pending[i] = v;
        }
    }

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m) override { collector.addMessageToQueue(m); }
    void scanDevices()
    {
        juce::MidiDeviceInfo fin, fout; bool hi = false, ho = false;
        for (auto& d : juce::MidiInput::getAvailableDevices()) if (ctl::isJt4000Name(d.name.toStdString())) { fin = d; hi = true; break; }
        for (auto& d : juce::MidiOutput::getAvailableDevices()) if (ctl::isJt4000Name(d.name.toStdString())) { fout = d; ho = true; break; }
        if (hi && (devIn == nullptr || devInId != fin.identifier)) { devIn.reset(); if (auto i = juce::MidiInput::openDevice(fin.identifier, this)) { devIn = std::move(i); devInId = fin.identifier; devIn->start(); } }
        if (! hi && devIn != nullptr) { devIn.reset(); devInId.clear(); }
        if (ho && (! hasOut || devOutId != fout.identifier))
        {
            auto o = juce::MidiOutput::openDevice(fout.identifier);
            { juce::SpinLock::ScopedLockType l(devLock); devOut = std::move(o); }
            hasOut = devOut != nullptr; devOutId = hasOut ? fout.identifier : juce::String();
        }
        if (! ho && hasOut) { { juce::SpinLock::ScopedLockType l(devLock); devOut.reset(); } hasOut = false; devOutId.clear(); }
        connected = hasOut; devNameStr = hasOut ? fout.name : juce::String();
    }

    juce::MidiMessageCollector collector; juce::SpinLock devLock; std::unique_ptr<juce::MidiInput> devIn; std::unique_ptr<juce::MidiOutput> devOut;
    juce::String devInId, devOutId, devNameStr; bool hasOut = false; int scanTick = 0; std::atomic<bool> connected{ false };
    ctl::Decoder decoder;
    std::unique_ptr<std::atomic<int>[]> pending, lastRecv, incoming, learnedIn; std::array<bool, 128> reserved{};
    std::vector<int> lastSeen; int rotate = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CtlProcessor)
};

#include "PanelEditor.h"

juce::AudioProcessorEditor* CtlProcessor::createEditor() { return new panel::PanelEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CtlProcessor(); }
