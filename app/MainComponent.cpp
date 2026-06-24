#include "MainComponent.h"

#include "../dmx/Outputs/ArtNetOutput.h"

MainComponent::MainComponent()
{
    titleLabel.setText ("DMX Output Controller", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    addAndMakeVisible (titleLabel);

    // --- Output selection -------------------------------------------------
    addAndMakeVisible (outputLabel);

    outputBox.setTextWhenNothingSelected ("Press Detect to scan for outputs");
    addAndMakeVisible (outputBox);

    detectButton.onClick = [this] { refreshOutputs(); };
    addAndMakeVisible (detectButton);

    // --- Universe selection ----------------------------------------------
    addAndMakeVisible (universeLabel);

    // Flat, 0-based Art-Net Port-Address range (15-bit: Net 7 + SubUni 8).
    universeSlider.setRange (0.0, 32767.0, 1.0);
    universeSlider.setValue (0.0, juce::dontSendNotification);
    universeSlider.onValueChange = [this] { updateUniverseDecode(); };
    addAndMakeVisible (universeSlider);

    universeDecodeLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (universeDecodeLabel);
    updateUniverseDecode();

    // --- Transport control ------------------------------------------------
    startButton.onClick = [this] { toggleSending(); };
    addAndMakeVisible (startButton);

    statusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    statusLabel.setText ("Idle.", juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    refreshOutputs();
    setSize (520, 320);
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.stopSending();
}

void MainComponent::refreshOutputs()
{
    // The ArtPoll scan blocks for ~1s waiting for replies, so run it off the
    // message thread and update the UI when it returns.
    detectButton.setEnabled (false);
    statusLabel.setText ("Scanning for Art-Net nodes...", juce::dontSendNotification);

    juce::Component::SafePointer<MainComponent> safe (this);

    juce::Thread::launch ([safe]
    {
        auto found = discoverArtNetOutputs(); // static destinations

        const auto nodes = discoverArtNetNodes (1000); // active ArtPoll
        for (auto& node : nodes)
        {
            const std::string title = node.shortName.empty() ? "Art-Net node" : node.shortName;
            const std::string label = title + " - " + node.ip
                                    + " (" + std::to_string (node.numPorts) + " ports)";
            found.push_back ({ label, node.ip, 6454 });
        }

        const int nodeCount = (int) nodes.size();

        juce::MessageManager::callAsync ([safe, found, nodeCount]
        {
            if (auto* self = safe.getComponent())
                self->onOutputsDiscovered (std::move (found), nodeCount);
        });
    });
}

void MainComponent::onOutputsDiscovered (std::vector<DiscoveredOutput> found, int nodeCount)
{
    outputs = std::move (found);

    const int previousId = outputBox.getSelectedId();
    outputBox.clear (juce::dontSendNotification);

    int itemId = 1;                                  // ComboBox item ids are 1-based
    for (const auto& out : outputs)
        outputBox.addItem (juce::String (out.description.c_str()), itemId++);   // c_str(): juce::String treats it as UTF-8

    if (previousId > 0 && previousId <= (int) outputs.size())
        outputBox.setSelectedId (previousId, juce::dontSendNotification);
    else if (! outputs.empty())
        outputBox.setSelectedId (1, juce::dontSendNotification);

    detectButton.setEnabled (true);
    statusLabel.setText (juce::String ((int) outputs.size()) + " destination(s), "
                             + juce::String (nodeCount) + " node(s) replied to ArtPoll.",
                         juce::dontSendNotification);
}

void MainComponent::toggleSending()
{
    if (sending)
    {
        stopTimer();
        engine.clear();
        engine.flushOnce();        // push the blacked-out frame so nothing stays lit
        engine.stopSending();

        sending = false;
        startButton.setButtonText ("Start sending");
        statusLabel.setText ("Stopped.", juce::dontSendNotification);
        return;
    }

    const int sel = outputBox.getSelectedId();
    if (sel <= 0 || sel > (int) outputs.size())
    {
        statusLabel.setText ("Select an output first (press Detect).", juce::dontSendNotification);
        return;
    }

    const auto& out = outputs[(size_t) (sel - 1)];

    auto artnet = std::make_unique<ArtNetOutput> (out.host, out.port);
    if (! artnet->open())
    {
        statusLabel.setText (juce::String ("Could not open socket for ") + out.host.c_str() + ".",
                             juce::dontSendNotification);
        return;
    }

    engine.setOutput (std::move (artnet));
    engine.start (40.0);
    startTimer (60);               // refresh the sweep pattern ~16 times/sec

    sending = true;
    sweepStep = 0;
    startButton.setButtonText ("Stop sending");
    statusLabel.setText (juce::String ("Sending Art-Net to ") + out.host.c_str() + ":" + juce::String (out.port)
                             + " on universe " + juce::String ((int) universeSlider.getValue()) + ".",
                         juce::dontSendNotification);
}

void MainComponent::updateUniverseDecode()
{
    const int universe = (int) universeSlider.getValue();
    const int subUni   = universe & 0xFF;          // low 8 bits  (matches ArtNetOutput)
    const int net      = (universe >> 8) & 0x7F;   // high 7 bits

    universeDecodeLabel.setText ("Art-Net  Net " + juce::String (net) + "  /  SubUni " + juce::String (subUni),
                                 juce::dontSendNotification);

    if (sending)
        statusLabel.setText ("Sending on universe " + juce::String (universe) + ".",
                             juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    // Slow RGB sweep across eight 3-channel fixtures on the selected universe, so
    // a monitor clearly shows the data landing on the universe we picked. Clearing
    // every frame also zeroes any universe we previously transmitted on, so changing
    // the universe live never leaves stale channels lit.
    const int universe = (int) universeSlider.getValue();

    constexpr int fixtures       = 8;
    constexpr int channelsPerFix = 3;
    const int litFixture = (sweepStep / 6) % fixtures;       // advance every ~0.4 s
    const int address    = 1 + litFixture * channelsPerFix;

    engine.clear();
    engine.setChannel (universe, address + 0, 255);   // R
    engine.setChannel (universe, address + 1, 255);   // G
    engine.setChannel (universe, address + 2, 255);   // B

    ++sweepStep;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (16);

    titleLabel.setBounds (area.removeFromTop (32));
    area.removeFromTop (12);

    auto row = [&area] (int h) { auto r = area.removeFromTop (h); area.removeFromTop (8); return r; };

    {
        auto r = row (28);
        outputLabel.setBounds (r.removeFromLeft (80));
        detectButton.setBounds (r.removeFromRight (90));
        r.removeFromRight (8);
        outputBox.setBounds (r);
    }

    {
        auto r = row (28);
        universeLabel.setBounds (r.removeFromLeft (80));
        universeSlider.setBounds (r.removeFromLeft (160));
    }

    universeDecodeLabel.setBounds (row (22).withTrimmedLeft (80));

    area.removeFromTop (8);
    startButton.setBounds (row (34).removeFromLeft (160));

    statusLabel.setBounds (area.removeFromBottom (28));
}
