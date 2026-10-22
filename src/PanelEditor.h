// PanelEditor: the panel window of JT-4000 Controller (Studio 87), built on cat-ui.
// Navy panel, black knobs with a white pointer, OLED style display with a scope trace, LED buttons.
// Controls with a known CC are real parameters. Controls without a known CC are drawn dashed and do nothing (marked "not mapped yet").
// Included by Plugin.cpp after CtlProcessor is defined.
#pragma once

namespace panel
{
inline juce::Colour navyTop()   { return juce::Colour(0xff41568a); }
inline juce::Colour navyBot()   { return juce::Colour(0xff2c3a63); }
inline juce::Colour frameCol()  { return juce::Colour(0xff14161d); }
inline juce::Colour orangeLbl() { return juce::Colour(0xffffb040); }
inline juce::Colour labelCol()  { return juce::Colour(0xffeef2fa); }
inline juce::Colour dimLabel()  { return juce::Colour(0xffd6ddf0); }
inline juce::Colour tealCol()   { return juce::Colour(0xff16788a); }
inline juce::Colour oledBg()    { return juce::Colour(0xff0b1220); }
inline juce::Colour oledText()  { return juce::Colour(0xff3fd0ff); }
inline juce::Colour oledSoft()  { return juce::Colour(0xff8fe6ff); }

inline float textWidth(const juce::Font& f, const juce::String& t) { juce::GlyphArrangement ga; ga.addLineOfText(f, t, 0.f, 0.f); return ga.getBoundingBox(0, -1, true).getWidth(); }

struct Spec { const char* label; int cc; const char* full; };   // cc -1: no known CC yet

// ----- look and feel for knobs and faders -----
class PanelLaf : public catui::CatLookAndFeel
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const bool sel = (bool) s.getProperties()["sel"];
        const bool live = (bool) s.getProperties()["live"];
        auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
        const float d = juce::jmin(b.getWidth(), b.getHeight()); auto ring = juce::Rectangle<float>(d, d).withCentre(b.getCentre());
        const auto tick = sel ? orangeLbl() : (live ? juce::Colour(0xffc9d3ea) : juce::Colour(0xff8e9abb));
        if (sel) { g.setColour(orangeLbl().withAlpha(0.22f)); g.fillEllipse(ring); }
        const int dots = 21;
        for (int i = 0; i < dots; ++i)
        {
            if (! live && (i % 2)) continue;
            const float a = a0 + (a1 - a0) * (float) i / (float) (dots - 1); const float r = d * 0.47f;
            const float dr = d * (i == 0 || i == dots - 1 ? 0.03f : 0.022f);
            g.setColour(tick); g.fillEllipse(ring.getCentreX() + r * std::sin(a) - dr, ring.getCentreY() - r * std::cos(a) - dr, dr * 2, dr * 2);
        }
        auto body = ring.reduced(d * 0.17f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3c46), body.getX() + body.getWidth() * 0.35f, body.getY() + body.getHeight() * 0.3f,
                                               juce::Colour(0xff101116), body.getRight(), body.getBottom(), true));
        g.fillEllipse(body);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawEllipse(body, 1.2f);
        const float a = a0 + pos * (a1 - a0); const auto c = body.getCentre(); const float r = body.getWidth() * 0.5f;
        juce::Line<float> ln(c.x + r * 0.28f * std::sin(a), c.y - r * 0.28f * std::cos(a), c.x + r * 0.92f * std::sin(a), c.y - r * 0.92f * std::cos(a));
        g.setColour(juce::Colours::white); g.drawLine(ln, juce::jmax(2.f, d * 0.055f));
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, const juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const bool sel = (bool) s.getProperties()["sel"];
        auto tr = juce::Rectangle<float>((float) x + (float) w * 0.5f - 4.f, (float) y, 8.f, (float) h);
        g.setColour(juce::Colour(0xff0b0c10)); g.fillRoundedRectangle(tr, 4.f);
        g.setColour(sel ? orangeLbl() : juce::Colour(0xff8e9abb));
        juce::Path p; p.addRoundedRectangle(tr, 4.f); juce::PathStrokeType st(1.f); float dash[] = { 3.f, 3.f }; juce::Path out; st.createDashedStroke(out, p, dash, 2); g.fillPath(out);
        const float capH = juce::jmin(18.f, (float) h * 0.14f), cw = juce::jmin(26.f, (float) w * 0.9f);
        auto cap = juce::Rectangle<float>(cw, capH).withCentre({ tr.getCentreX(), pos });
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34363f), cap.getX(), cap.getY(), juce::Colour(0xff0d0e12), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle(cap, 3.f);
        g.setColour(sel ? orangeLbl() : juce::Colour(0xff0b0c10)); g.drawRoundedRectangle(cap, 3.f, 2.f);
        g.setColour(juce::Colours::white); g.fillRect(cap.getX() + 3, cap.getCentreY() - 1, cap.getWidth() - 6, 2.f);
    }
};

// ----- one knob with its label -----
class KnobCell : public juce::Component, public juce::SettableTooltipClient
{
public:
    KnobCell(const Spec& sp, juce::AudioParameterInt* p, std::function<void(KnobCell*)> sel) : spec(sp), param(p), onSel(std::move(sel))
    {
        addAndMakeVisible(slider); slider.setSliderStyle(juce::Slider::RotaryVerticalDrag); slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters(juce::degreesToRadians(225.f), juce::degreesToRadians(495.f), true);
        slider.getProperties().set("live", p != nullptr);
        if (p != nullptr) { attach = std::make_unique<juce::SliderParameterAttachment>(*p, slider); slider.addMouseListener(this, false); slider.onDragStart = [this] { onSel(this); }; }
        else { slider.setEnabled(false); slider.setInterceptsMouseClicks(false, false); slider.setRange(0, 127, 1); }
        setTooltip(juce::String(spec.full) + (p != nullptr ? ", CC " + juce::String(spec.cc) + ", not tested on hardware yet. Drag, or use the mouse wheel."
                                                           : ". No CC is known for this control yet, so it does nothing."));
    }
    void setSelected(bool b) { sel = b; slider.getProperties().set("sel", b); slider.repaint(); repaint(); }
    void mouseDown(const juce::MouseEvent&) override { onSel(this); }
    void resized() override
    {
        const int w = getWidth(), h = getHeight(); const int d = juce::jmax(8, juce::jmin(w, int((float) h * 0.74f))); const int th = juce::jmax(12, int((float) d * 0.36f));
        const int top = juce::jmax(0, (h - d - th) / 2);
        slider.setBounds((w - d) / 2, top, d, d); textArea = juce::Rectangle<int>(0, top + d, w, th);
    }
    void paint(juce::Graphics& g) override
    {
        auto t = textArea.toFloat(); const float fs = juce::jmax(8.f, juce::jmin(t.getHeight() * 0.48f, (float) getWidth() * 0.14f));
        g.setColour(labelCol()); g.setFont(juce::FontOptions(fs, juce::Font::bold));
        g.drawText(spec.label, t.removeFromTop(t.getHeight() * 0.55f), juce::Justification::centred, true);
        g.setColour(dimLabel()); g.setFont(juce::FontOptions(fs * 0.85f));
        g.drawText(spec.cc >= 0 ? "CC " + juce::String(spec.cc) : juce::String("CC -"), t, juce::Justification::centred, true);
    }
    const Spec spec; juce::AudioParameterInt* param; juce::Slider slider;
    int slotIndex = -1; bool sel = false;
private:
    std::function<void(KnobCell*)> onSel; std::unique_ptr<juce::SliderParameterAttachment> attach; juce::Rectangle<int> textArea;
};

// ----- an envelope fader without a known CC (display only) -----
class FaderCell : public juce::Component, public juce::SettableTooltipClient
{
public:
    FaderCell(const char* l, const char* grp, const char* full) : label(l), group(grp)
    {
        addAndMakeVisible(slider); slider.setSliderStyle(juce::Slider::LinearVertical); slider.setRange(0, 127, 1); slider.setValue(64, juce::dontSendNotification);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0); slider.setEnabled(false); slider.setInterceptsMouseClicks(false, false);
        setTooltip(juce::String(full) + ". No CC is known for the envelopes yet, so this fader does nothing.");
    }
    void resized() override { auto r = getLocalBounds(); textArea = r.removeFromBottom(juce::jmax(24, int((float) r.getHeight() * 0.22f))); slider.setBounds(r.reduced(0, 2)); }
    void paint(juce::Graphics& g) override
    {
        auto t = textArea.toFloat(); const float fs = juce::jmax(9.f, t.getHeight() * 0.42f);
        g.setColour(labelCol()); g.setFont(juce::FontOptions(fs, juce::Font::bold)); g.drawText(label, t.removeFromTop(t.getHeight() * 0.5f), juce::Justification::centred);
        g.setColour(dimLabel()); g.setFont(juce::FontOptions(fs * 0.8f)); g.drawText(group, t, juce::Justification::centred);
    }
    juce::Slider slider; const juce::String label, group; juce::Rectangle<int> textArea;
};

// ----- round LED button with an orange label chip -----
class LedButton : public juce::Button
{
public:
    LedButton(const juce::String& l, const juce::String& s, const juce::String& tip) : juce::Button(l), label(l), sub(s) { setTooltip(tip); }
    void setLit(bool b) { lit = b; repaint(); }
    void setSub(const juce::String& s) { sub = s; repaint(); }
    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat(); const float a = isEnabled() ? 1.f : 0.5f; const float fs = juce::jmax(9.f, r.getHeight() * 0.2f);
        auto chip = r.removeFromTop(r.getHeight() * 0.3f); auto subA = r.removeFromBottom(r.getHeight() * 0.26f);
        g.setFont(juce::FontOptions(fs, juce::Font::bold)); const float tw = textWidth(g.getCurrentFont(), label) + fs;
        auto c = chip.withSizeKeepingCentre(juce::jmin(tw, chip.getWidth()), chip.getHeight() * 0.9f);
        g.setColour(frameCol().withAlpha(a)); g.fillRoundedRectangle(c, c.getHeight() * 0.4f);
        g.setColour(orangeLbl().withAlpha(a)); g.drawText(label, c, juce::Justification::centred, true);
        const float d = juce::jmin(r.getWidth(), r.getHeight()) * 0.9f; auto led = juce::Rectangle<float>(d, d).withCentre(r.getCentre());
        if (lit) { g.setColour(juce::Colour(0x55ff3030)); g.fillEllipse(led.expanded(d * 0.18f)); }
        g.setGradientFill(lit ? juce::ColourGradient(juce::Colour(0xffff9a9a), led.getX() + d * 0.4f, led.getY() + d * 0.35f, juce::Colour(0xffb00000), led.getRight(), led.getBottom(), true)
                              : juce::ColourGradient(juce::Colour(down ? 0xff6f7ea5 : over ? 0xff9aa8c9 : 0xff8d9bbd), led.getX() + d * 0.4f, led.getY() + d * 0.35f, juce::Colour(0xff5d6c92), led.getRight(), led.getBottom(), true));
        g.setOpacity(a); g.fillEllipse(led); g.setColour(frameCol().withAlpha(a)); g.drawEllipse(led, 2.f);
        g.setColour(labelCol().withAlpha(a)); g.setFont(juce::FontOptions(fs * 0.8f, juce::Font::bold)); g.drawText(sub, subA, juce::Justification::centred, true);
    }
private:
    juce::String label, sub; bool lit = false;
};

// ----- the OLED style display -----
class Display : public juce::Component
{
public:
    juce::String topL, topR, name, value, botL, botR; std::vector<float> scope; juce::Colour botRCol = oledText();
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour(frameCol()); g.fillRoundedRectangle(r, 5.f); r = r.reduced(3.f);
        g.setColour(oledBg()); g.fillRoundedRectangle(r, 3.f);
        g.setColour(oledText().withAlpha(0.08f)); g.fillRoundedRectangle(r.reduced(1.f), 3.f);
        r = r.reduced(r.getWidth() * 0.03f, r.getHeight() * 0.05f);
        const auto mono = juce::Font::getDefaultMonospacedFontName(); const float fs = juce::jmax(9.f, r.getHeight() * 0.12f);
        auto top = r.removeFromTop(fs * 1.5f), bot = r.removeFromBottom(fs * 1.4f), mid = r.removeFromBottom(fs * 2.1f);
        g.setFont(juce::FontOptions(mono, fs, juce::Font::plain)); g.setColour(oledText());
        g.drawText(topL, top, juce::Justification::centredLeft, true); g.drawText(topR, top, juce::Justification::centredRight, true);
        g.setFont(juce::FontOptions(mono, fs * 1.15f, juce::Font::plain)); g.setColour(oledSoft());
        g.drawText(name, mid.withTrimmedRight(mid.getWidth() * 0.22f), juce::Justification::centredLeft, true);
        g.setFont(juce::FontOptions(mono, fs * 1.7f, juce::Font::plain)); g.setColour(oledText()); g.drawText(value, mid, juce::Justification::centredRight, true);
        g.setFont(juce::FontOptions(mono, fs * 0.9f, juce::Font::plain)); g.setColour(oledText()); g.drawText(botL, bot, juce::Justification::centredLeft, true);
        g.setColour(botRCol); g.drawText(botR, bot, juce::Justification::centredRight, true);
        g.setColour(oledText().withAlpha(0.2f)); g.drawHorizontalLine(int(r.getCentreY()), r.getX(), r.getRight());
        if (scope.size() > 1)
        {
            juce::Path p; for (size_t i = 0; i < scope.size(); ++i)
            { const float px = r.getX() + r.getWidth() * (float) i / (float) (scope.size() - 1), py = r.getCentreY() - scope[i] * r.getHeight() * 0.46f; if (i == 0) p.startNewSubPath(px, py); else p.lineTo(px, py); }
            g.setColour(oledText()); g.strokePath(p, juce::PathStrokeType(juce::jmax(1.5f, r.getHeight() * 0.012f)));
        }
    }
};

// ----- the whole panel -----
class PanelView : public juce::Component
{
public:
    explicit PanelView(CtlProcessor& p) : proc(p), gate(*this)
    {
        setLookAndFeel(&laf);
        addAndMakeVisible(display);
        auto knob = [this](std::vector<std::unique_ptr<KnobCell>>& into, const Spec& s)
        {
            juce::AudioParameterInt* ap = nullptr; int slot = -1;
            if (s.cc >= 0) for (size_t i = 0; i < proc.slots.size(); ++i)
            { auto& m = proc.mapRes.params[size_t(proc.slots[i].map)]; if (m.kind == ctl::Kind::CC && m.a == s.cc) { ap = proc.slots[i].param; slot = int(i); break; } }
            auto k = std::make_unique<KnobCell>(s, ap, [this](KnobCell* c) { select(c); }); k->slotIndex = slot; addAndMakeVisible(*k); all.push_back(k.get()); into.push_back(std::move(k));
        };
        for (auto& s : oscSpec) knob(osc, s);
        for (auto& s : filterSpec) knob(filter, s);
        for (auto& s : lfo1Spec) knob(lfo1, s);
        for (auto& s : lfo2Spec) knob(lfo2, s);
        for (auto& s : mixSpec) knob(mix, s);
        const char* env[8] = { "A", "D", "S", "R", "A", "D", "S", "R" };
        const char* names[8] = { "VCF ATTACK", "VCF DECAY", "VCF SUSTAIN", "VCF RELEASE", "VCA ATTACK", "VCA DECAY", "VCA SUSTAIN", "VCA RELEASE" };
        for (int i = 0; i < 8; ++i) { faders.push_back(std::make_unique<FaderCell>(env[i], i < 4 ? "VCF" : "VCA", names[i])); addAndMakeVisible(*faders.back()); }

        prevB = std::make_unique<LedButton>("PRESET", "< PREV", "Preset change needs a program change or SysEx the device documents. Not known yet.");
        nextB = std::make_unique<LedButton>("PRESET", "NEXT >", "Preset change needs a program change or SysEx the device documents. Not known yet.");
        arpB = std::make_unique<LedButton>("ARP", "NO CC YET", "No CC is known for the arpeggiator yet.");
        downB = std::make_unique<LedButton>("VALUE -", "DOWN", "Lowers the selected control by 8.");
        upB = std::make_unique<LedButton>("VALUE +", "UP", "Raises the selected control by 8.");
        learnB = std::make_unique<LedButton>("LEARN", "OFF", "MIDI learn: select a knob, then move a knob on your own MIDI controller. That CC then moves the knob (and the device).");
        for (auto* b : { prevB.get(), nextB.get(), arpB.get() }) b->setEnabled(false);
        for (auto* b : { prevB.get(), nextB.get(), arpB.get(), downB.get(), upB.get(), learnB.get() }) addAndMakeVisible(*b);
        downB->onClick = [this] { nudge(-8); }; upB->onClick = [this] { nudge(8); };
        learnB->onClick = [this] { learnOn = ! learnOn; proc.learnTarget = -1; learnB->setLit(learnOn); learnB->setSub(learnOn ? "ARMED" : "OFF"); armIfSelected(); };

        rescan.setTooltip("Direct connection: the plugin finds a JT-4000 on the system MIDI ports and talks to it itself. Turn it OFF if your DAW already uses the JT-4000 MIDI port (on Windows a port is often usable by one program only)."); rescan.onClick = [this] { proc.directOn = ! proc.directOn.load(); proc.scanNow(); updateDirectText(); }; updateDirectText();
        sendAll.setButtonText("Send all"); sendAll.setTooltip("Sends every parameter value to the device now."); sendAll.onClick = [this] { proc.sendAll(); };
        clearLearn.setButtonText("Clear learned"); clearLearn.setTooltip("Removes every MIDI learn assignment."); clearLearn.onClick = [this] { proc.learn.clear(); proc.learnTarget = -1; };
        chLabel.setText("MIDI channel", juce::dontSendNotification); chLabel.setColour(juce::Label::textColourId, labelCol());
        chSlider.setSliderStyle(juce::Slider::IncDecButtons); chSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 36, 24);
        chAttach = std::make_unique<juce::SliderParameterAttachment>(*proc.channelParam, chSlider);
        status.setText("CC list from the community, not tested on the device yet.", juce::dontSendNotification);
        status.setColour(juce::Label::textColourId, dimLabel()); status.setFont(juce::FontOptions(12.f));
        for (auto* w : std::initializer_list<juce::Component*>{ &sendAll, &rescan, &clearLearn, &chLabel, &chSlider, &status }) addAndMakeVisible(*w);

        select(osc.size() > 0 ? filter[0].get() : nullptr);   // starts on the cutoff knob
        gate.onFrame = [this] { refreshDisplay(); }; gate.start(15);
    }
    ~PanelView() override { gate.stop(); setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(navyTop(), 0, 0, navyBot(), 0, (float) getHeight(), false)); g.fillAll();
        for (auto& s : sections)
        {
            if (s.teal) { g.setColour(tealCol()); g.fillRoundedRectangle(s.r, 14.f); }
            g.setColour(frameCol()); g.drawRoundedRectangle(s.r.reduced(1.5f), 14.f, 3.f);
            const float fs = juce::jmax(11.f, (float) getHeight() * 0.026f); g.setFont(juce::FontOptions(fs, juce::Font::bold));
            const float tw = textWidth(g.getCurrentFont(), s.title) + fs * 1.6f;
            juce::Rectangle<float> tab(s.r.getX() + 18.f, s.r.getY() - fs * 0.75f, tw, fs * 1.5f);
            g.setColour(frameCol()); g.fillRoundedRectangle(tab, fs * 0.5f); g.setColour(orangeLbl()); g.drawText(s.title, tab, juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().toFloat(); const float W = r.getWidth(), H = r.getHeight(); const float m = W * 0.018f, gap = W * 0.016f;
        sections.clear();
        auto bottom = getLocalBounds().removeFromBottom(int(H * 0.085f)).reduced(int(m), 4);
        const float top = H * 0.04f + 4.f; const float hA = H * 0.45f, hB = H * 0.35f; const float gy = H * 0.06f;
        auto rowRects = [&](float y, float h, std::vector<float> weights)
        {
            std::vector<juce::Rectangle<float>> out; float total = 0; for (float w : weights) total += w; const float avail = W - 2 * m - gap * (float) (weights.size() - 1); float x = m;
            for (float w : weights) { const float cw = avail * w / total; out.push_back({ x, y, cw, h }); x += cw + gap; } return out;
        };
        auto A = rowRects(top, hA, { 0.34f, 0.38f, 0.28f }); auto B = rowRects(top + hA + gy, hB, { 0.215f, 0.215f, 0.25f, 0.32f });
        const char* tA[3] = { "DISPLAY", "OSC 1 / OSC 2", "ANALOG FILTER" }; const char* tB[4] = { "LFO 1", "LFO 2", "MIX / MOD", "VCF ENV / VCA ENV" };
        for (int i = 0; i < 3; ++i) sections.push_back({ A[(size_t) i], tA[i], false });
        for (int i = 0; i < 4; ++i) sections.push_back({ B[(size_t) i], tB[i], i == 2 });
        auto inner = [&](juce::Rectangle<float> s) { return s.toNearestInt().reduced(int(W * 0.012f)).withTrimmedTop(int(H * 0.022f)); };
        auto grid = [&](std::vector<std::unique_ptr<KnobCell>>& v, juce::Rectangle<int> a, int cols, int rows)
        { for (size_t i = 0; i < v.size(); ++i) { const int c = int(i) % cols, rr = int(i) / cols; v[i]->setBounds(a.getX() + a.getWidth() * c / cols, a.getY() + a.getHeight() * rr / rows, a.getWidth() / cols, a.getHeight() / rows); } };
        // display section
        { auto a = inner(A[0]); display.setBounds(a.removeFromTop(int((float) a.getHeight() * 0.52f))); a.removeFromTop(6);
          LedButton* bs[6] = { prevB.get(), nextB.get(), arpB.get(), downB.get(), upB.get(), learnB.get() };
          for (int i = 0; i < 6; ++i) { const int c = i % 3, rr = i / 3; bs[i]->setBounds(a.getX() + a.getWidth() * c / 3, a.getY() + a.getHeight() * rr / 2, a.getWidth() / 3, a.getHeight() / 2); } }
        grid(osc, inner(A[1]), 4, 2);
        { auto a = inner(A[2]); auto big = a.removeFromTop(int((float) a.getHeight() * 0.62f)); filter[0]->setBounds(big.removeFromLeft(big.getWidth() / 2)); filter[1]->setBounds(big);
          filter[2]->setBounds(a.removeFromLeft(a.getWidth() / 2)); }
        grid(lfo1, inner(B[0]), 2, 2); grid(lfo2, inner(B[1]), 2, 2); grid(mix, inner(B[2]), 3, 2);
        { auto a = inner(B[3]); for (size_t i = 0; i < faders.size(); ++i) faders[i]->setBounds(a.getX() + a.getWidth() * int(i) / 8, a.getY(), a.getWidth() / 8, a.getHeight()); }
        // bottom bar
        sendAll.setBounds(bottom.removeFromLeft(100)); bottom.removeFromLeft(8); rescan.setBounds(bottom.removeFromLeft(100)); bottom.removeFromLeft(8); clearLearn.setBounds(bottom.removeFromLeft(110)); bottom.removeFromLeft(14);
        chLabel.setBounds(bottom.removeFromLeft(90)); chSlider.setBounds(bottom.removeFromLeft(96)); bottom.removeFromLeft(14); status.setBounds(bottom);
        refreshDisplay();
    }

    void select(KnobCell* c)
    {
        if (c == nullptr) return;
        sel = c; for (auto* k : all) k->setSelected(k == c);
        armIfSelected(); refreshDisplay();
    }

private:
    struct Section { juce::Rectangle<float> r; const char* title; bool teal; };
    void updateDirectText() { rescan.setButtonText(proc.directOn.load() ? "Direct: ON" : "Direct: OFF"); }
    void armIfSelected() { proc.learnTarget = (learnOn && sel != nullptr && sel->slotIndex >= 0) ? sel->slotIndex : -1; }
    void nudge(int d)
    {
        if (sel == nullptr || sel->param == nullptr) return;
        auto* p = sel->param; const int v = juce::jlimit(p->getRange().getStart(), p->getRange().getEnd(), p->get() + d);
        p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1((float) v)); p->endChangeGesture();
    }
    float val01(int cc) const
    {
        for (auto& s : proc.slots) { auto& m = proc.mapRes.params[size_t(s.map)]; if (m.kind == ctl::Kind::CC && m.a == cc) return (float) s.param->get() / 127.f; }
        return 0.5f;
    }
    void refreshDisplay()
    {
        display.topL = "MIDI CH " + juce::String(proc.channelParam->get()).paddedLeft('0', 2); display.topR = proc.deviceConnected() ? juce::String("JT-4000 CONNECTED") : (proc.directOn.load() ? juce::String("NO DEVICE, HOST MIDI") : juce::String("DIRECT OFF, HOST MIDI"));
        if (sel != nullptr)
        {
            display.name = sel->spec.full; display.value = sel->param != nullptr ? juce::String(sel->param->get()).paddedLeft(' ', 3) : juce::String("---");
            display.botL = sel->spec.cc >= 0 ? "DEVICE CC " + juce::String(sel->spec.cc) : juce::String("NO CC YET");
            const int lc = sel->slotIndex >= 0 ? proc.learn.ccFor(sel->slotIndex) : -1;
            if (learnOn && sel->slotIndex >= 0 && proc.learnTarget.load() == sel->slotIndex) display.botR = "LEARN: MOVE A KNOB..";
            else if (lc >= 0) display.botR = "LEARNED: CC " + juce::String(lc);
            else display.botR = learnOn ? "LEARN: SELECT A KNOB" : "LEARN: OFF";
        }
        const float f = 1.5f + val01(115) * 4.f, amp = 0.35f + 0.65f * val01(74), mixv = val01(29);
        display.scope.resize(86);
        for (size_t i = 0; i < display.scope.size(); ++i)
        {
            const float t = (float) i / 85.f * juce::MathConstants<float>::twoPi * f;
            const float s1 = std::sin(t) + 0.4f * std::sin(2 * t) + 0.2f * std::sin(3 * t), s2 = std::sin(t * 1.01f + 0.5f) * (0.4f + 0.6f * mixv);
            display.scope[i] = juce::jlimit(-1.f, 1.f, amp * (s1 * (1.f - mixv * 0.5f) + s2) / 1.9f * 1.6f);
        }
        display.repaint();
    }

    CtlProcessor& proc; PanelLaf laf; catui::RenderGate gate; Display display;
    std::vector<KnobCell*> all; KnobCell* sel = nullptr; bool learnOn = false;
    std::vector<std::unique_ptr<KnobCell>> osc, filter, lfo1, lfo2, mix; std::vector<std::unique_ptr<FaderCell>> faders;
    std::unique_ptr<LedButton> prevB, nextB, arpB, downB, upB, learnB; std::vector<Section> sections;
    juce::TextButton sendAll, rescan, clearLearn; juce::Label chLabel, status; juce::Slider chSlider; std::unique_ptr<juce::SliderParameterAttachment> chAttach;

    inline static const Spec oscSpec[8] = { { "WAVE", 24, "OSC1 Wave" }, { "PWM/DET/FB", 113, "OSC1 PWM / Supersaw detune / FM feedback" }, { "COARSE", 115, "OSC1 Coarse" }, { "FINE", 111, "OSC1 Fine" },
                                            { "WAVE", -1, "OSC2 Wave" }, { "PWM", 114, "OSC2 PWM" }, { "COARSE", 116, "OSC2 Coarse" }, { "FINE", 112, "OSC2 Fine" } };
    inline static const Spec filterSpec[3] = { { "CUTOFF", 74, "Filter cutoff" }, { "RESO", 71, "Filter resonance" }, { "ENV AMT", 47, "Filter envelope amount" } };
    inline static const Spec lfo1Spec[4] = { { "DEST", 56, "LFO1 destination" }, { "WAVE", 54, "LFO1 wave" }, { "RATE", 72, "LFO1 rate" }, { "AMOUNT", 70, "LFO1 amount" } };
    inline static const Spec lfo2Spec[4] = { { "DEST", -1, "LFO2 destination" }, { "WAVE", 55, "LFO2 wave" }, { "RATE", 73, "LFO2 rate" }, { "AMOUNT", 28, "LFO2 amount" } };
    inline static const Spec mixSpec[5] = { { "OSC BAL", 29, "Oscillator balance" }, { "RING AMT", 95, "Ring mod amount" }, { "RING ON", 96, "Ring mod on/off" }, { "PORTA", 5, "Portamento time" }, { "MOD WHL", 1, "Mod wheel" } };
};

class PanelEditor : public juce::AudioProcessorEditor
{
public:
    explicit PanelEditor(CtlProcessor& p) : AudioProcessorEditor(p), frame("JT-4000 Controller", 1180, 720), view(p), tips(this, 700)
    {
        addAndMakeVisible(frame); frame.content().addAndMakeVisible(view);
        frame.header().onInfo = [this] { juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "JT-4000 Controller",
            "Free and open source by Studio 87. Values come from a community CC list, not a Behringer manual. Nothing is sent that is not in the map, and nothing is invented. Support: ko-fi.com/studio87"); };
        setResizable(true, true); getConstrainer()->setFixedAspectRatio(1180.0 / 720.0); setResizeLimits(590, 360, 2360, 1440); setSize(1180, 720);
    }
    void resized() override { frame.setBounds(getLocalBounds()); }
private:
    catui::CatFrame frame; PanelView view; juce::TooltipWindow tips;
};
}
