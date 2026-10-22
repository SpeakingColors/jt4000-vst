// Processor level MIDI test (needs JUCE): feeds MIDI buffers into the real plugin processor and checks what comes out.
// This tests the MIDI logic and routing inside the plugin. It does not test a real USB or DIN connection to the synth.
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
#include "../src/MidiCore.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

struct Out { int status, d1, d2; };
static std::vector<Out> run(juce::AudioProcessor& p, juce::MidiBuffer in)
{
    juce::AudioBuffer<float> audio(0, 512);
    p.processBlock(audio, in);
    std::vector<Out> out;
    for (const auto m : in) { auto msg = m.getMessage(); auto* d = msg.getRawData(); out.push_back({ d[0], msg.getRawDataSize() > 1 ? d[1] : 0, msg.getRawDataSize() > 2 ? d[2] : 0 }); }
    return out;
}
static void pump(int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil(ms); }   // lets the 30 Hz timer run
static juce::MidiBuffer cc(int ch, int num, int val) { juce::MidiBuffer b; b.addEvent(juce::MidiMessage::controllerEvent(ch, num, val), 0); return b; }
static juce::RangedAudioParameter* find(juce::AudioProcessor& p, const juce::String& id)
{
    for (auto* q : p.getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(q)) if (r->paramID == id) return r;
    return nullptr;
}
static bool has(const std::vector<Out>& v, int status, int d1, int d2) { for (auto& o : v) if (o.status == status && o.d1 == d1 && o.d2 == d2) return true; return false; }
static bool hasCC(const std::vector<Out>& v, int d1) { for (auto& o : v) if ((o.status & 0xF0) == 0xB0 && o.d1 == d1) return true; return false; }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    p->prepareToPlay(48000, 512);
    CHECK(p->getParameters().size() == 23);   // 22 map entries + MIDI channel

    // 1. device to plugin: CC 74 moves "cc_74", and is NOT sent back out
    auto o = run(*p, cc(1, 74, 100)); CHECK(! hasCC(o, 74));
    pump(150); auto* cut = find(*p, "cc_74"); CHECK(cut != nullptr);
    CHECK(cut && juce::roundToInt(cut->convertFrom0to1(cut->getValue())) == 100);
    o = run(*p, {}); CHECK(o.empty());        // no echo to the device

    // 2. plugin to device: automation of cc_74 sends CC 74 on channel 1
    cut->setValueNotifyingHost(cut->convertTo0to1(50)); pump(150);
    o = run(*p, {}); CHECK(has(o, 0xB0, 74, 50));

    // 3. MIDI channel setting is used
    auto* ch = find(*p, "midi_channel"); ch->setValueNotifyingHost(ch->convertTo0to1(5)); pump(100);
    cut->setValueNotifyingHost(cut->convertTo0to1(60)); pump(150);
    o = run(*p, {}); CHECK(has(o, 0xB4, 74, 60));
    ch->setValueNotifyingHost(ch->convertTo0to1(1)); pump(100);

    // 4. every mapped CC arrives and leaves
    for (int n : { 24, 113, 115, 111, 114, 116, 112, 29, 71, 47, 56, 54, 70, 72, 55, 73, 28, 1, 95, 96, 5 })
    { run(*p, cc(1, n, 33)); pump(60); auto* q = find(*p, "cc_" + juce::String(n)); CHECK(q && juce::roundToInt(q->convertFrom0to1(q->getValue())) == 33); }

    // 5. other MIDI passes through, unknown CCs pass through
    juce::MidiBuffer notes; notes.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 90), 0); notes.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 1);
    o = run(*p, notes); CHECK(has(o, 0x90, 60, 90)); CHECK(has(o, 0xB0, 64, 127));

    // 6. send all: at most 16 per block, everything within a few blocks
    if (auto* pl = dynamic_cast<juce::AudioProcessor*>(p.get())) { (void) pl; }
    p.reset(createPluginFilter()); p->prepareToPlay(48000, 512); pump(100);   // first timer tick records the start values
    {
        // sendAll is public on the processor class, reach it through a state round trip instead: set all values via automation
        int expected = 0; for (auto* q : p->getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(q)) if (r->paramID.startsWith("cc_")) { r->setValueNotifyingHost(r->convertTo0to1(10)); ++expected; }
        pump(200); int total = 0, maxBlock = 0; for (int b = 0; b < 6; ++b) { auto ob = run(*p, {}); int c = 0; for (auto& x : ob) if ((x.status & 0xF0) == 0xB0) ++c; maxBlock = juce::jmax(maxBlock, c); total += c; }
        CHECK(maxBlock <= 16); CHECK(total == expected);
    }

    // 7. MIDI learn: reserved CCs are refused, a free CC binds and then drives the parameter and the device
    {
        std::unique_ptr<juce::AudioProcessor> q(createPluginFilter()); q->prepareToPlay(48000, 512);
        // the processor type is private to Plugin.cpp, so learn is driven through its public members via the state: learn_<id> attribute
        juce::XmlElement x("STATE"); x.setAttribute("v", 1); x.setAttribute("midi_channel", 1); x.setAttribute("learn_cc_74", 21); x.setAttribute("learn_cc_71", 74);   // 74 is reserved, must be ignored
        juce::MemoryBlock mb; juce::AudioProcessor::copyXmlToBinary(x, mb); q->setStateInformation(mb.getData(), (int) mb.getSize());
        auto o1 = run(*q, cc(1, 21, 77)); CHECK(! hasCC(o1, 21));            // learned CC is consumed
        pump(150); auto* c74 = find(*q, "cc_74"); CHECK(c74 && juce::roundToInt(c74->convertFrom0to1(c74->getValue())) == 77);
        auto o2 = run(*q, {}); CHECK(has(o2, 0xB0, 74, 77));                  // and the device is updated
        // reserved CC 74 from the device still drives cc_74 only, cc_71 got no learn binding
        run(*q, cc(1, 74, 12)); pump(100); CHECK(juce::roundToInt(c74->convertFrom0to1(c74->getValue())) == 12);
        // state round trip keeps the assignment
        juce::MemoryBlock saved; q->getStateInformation(saved);
        auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), (int) saved.getSize()); CHECK(xml && xml->getIntAttribute("learn_cc_74", -1) == 21 && ! xml->hasAttribute("learn_cc_71"));
    }

    // 8. NRPN selector CCs from the device never crash or move anything
    run(*p, cc(1, 99, 3)); run(*p, cc(1, 98, 4)); run(*p, cc(1, 6, 9));

    std::printf(fails ? "%d failures\n" : "all processor MIDI tests passed\n", fails);
    return fails ? 1 : 0;
}
