// Mix Practice Studio - Console Bank Selector Bar Implementation
#include "BankSelectorBar.h"
#include "../Core/DspMath.h"

namespace mps
{

BankSelectorBar::BankSelectorBar()
{
    setOpaque (false);
}

void BankSelectorBar::setSelectedBank (int bankIndex)
{
    if (selectedBank != bankIndex && bankIndex >= 0 && bankIndex < NumBanks)
    {
        selectedBank = bankIndex;
        repaint();
        if (onBankSelected)
            onBankSelected (selectedBank);
    }
}

void BankSelectorBar::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float h = bounds.getHeight();

    // 1. Logo Block on the Left (width ~92px)
    const float logoW = 90.0f;
    const juce::Rectangle<float> logoArea (bounds.getX() + 2.0f, bounds.getY() + 2.0f, logoW, h - 4.0f);

    // Styled MPS Logo
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("MPS", (int) logoArea.getX(), (int) logoArea.getY() + 2, (int) logoW, 20, juce::Justification::centred);

    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.setColour (juce::Colour (0xffb0bec5));
    g.drawText ("DEMO MODE", (int) logoArea.getX(), (int) logoArea.getY() + 22, (int) logoW, 12, juce::Justification::centred);

    // Info (i) and bulb icons
    g.setColour (juce::Colour (0xff78909c));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("(i)  \U0001F4A1", (int) logoArea.getX(), (int) logoArea.getY() + 34, (int) logoW, 14, juce::Justification::centred);

    // 2. 10 Bank Buttons
    const char* bankNames[NumBanks] = {
        "Ch 1-8", "Ch 9-16", "Ch 17-24", "Ch 25-32",
        "Aux 1-8", "FX 1L-4R", "Bus 1-8", "Bus 9-16",
        "Mtx-Main", "DCA 1-8"
    };

    const float banksStartX = logoArea.getRight() + 4.0f;
    const float availW = bounds.getRight() - banksStartX - 4.0f;
    const float buttonW = availW / (float) NumBanks;

    for (int i = 0; i < NumBanks; ++i)
    {
        const float bx = banksStartX + (float) i * buttonW;
        const juce::Rectangle<float> btnRect (bx + 1.0f, bounds.getY() + 2.0f, buttonW - 2.0f, h - 4.0f);

        const bool isSel = (i == selectedBank);

        // Background
        g.setColour (isSel ? juce::Colour (0xff252c38) : juce::Colour (0xff15181e));
        g.fillRoundedRectangle (btnRect, 2.0f);

        // Border
        g.setColour (isSel ? juce::Colour (0xff00d2ff) : juce::Colour (0xff2a303b));
        g.drawRoundedRectangle (btnRect, 2.0f, isSel ? 1.5f : 1.0f);

        // Miniature 8-column LED meters
        const float meterTopY = btnRect.getY() + 4.0f;
        const float meterH = 26.0f;
        const float meterAreaW = btnRect.getWidth() - 6.0f;
        const float colW = meterAreaW / 8.0f;

        for (int c = 0; c < 8; ++c)
        {
            const float mx = btnRect.getX() + 3.0f + (float) c * colW;
            float chDb = -90.0f;
            if (meters != nullptr)
            {
                if (i <= 3) // Channels 1..32
                {
                    const int chIdx = i * 8 + c;
                    if (chIdx < kMaxInputChannels)
                    {
                        const float pL = meters->channels[(size_t) chIdx].preL.read();
                        const float pR = meters->channels[(size_t) chIdx].preR.read();
                        chDb = gainToDb (std::max (pL, pR));
                    }
                }
                else if (i == 4) // Aux 1-8
                {
                    chDb = gainToDb (meters->auxes[(size_t) c].read());
                }
                else if (i == 5) // FX 1L-4R (FX Returns)
                {
                    chDb = gainToDb (meters->fxReturns[(size_t) c].read());
                }
                else if (i == 6) // Bus 1-8
                {
                    chDb = gainToDb (meters->buses[(size_t) c].read());
                }
                else if (i == 7) // Bus 9-16 (Buses 13..16 are FX 1..4 Sends)
                {
                    chDb = gainToDb (meters->buses[(size_t) (8 + c)].read());
                }
                else if (i == 8) // Mtx-Main
                {
                    if (c < 6)
                        chDb = gainToDb (meters->matrices[(size_t) c].read());
                    else if (c == 6)
                        chDb = gainToDb (meters->mainC.read());
                    else
                        chDb = gainToDb (std::max (meters->main.l.read(), meters->main.r.read()));
                }
            }

            // 6 discrete LED segments: s=0 is 0 dB (top/clip), s=5 is -50 dB (bottom)
            const float segThresh[6] = { 0.0f, -6.0f, -12.0f, -18.0f, -30.0f, -50.0f };
            for (int s = 0; s < 6; ++s)
            {
                const float sy = meterTopY + (float) s * 4.2f;
                const juce::Rectangle<float> dotRect (mx + 1.0f, sy, colW - 2.0f, 3.0f);

                const bool lit = (chDb >= segThresh[s]);
                juce::Colour segCol = juce::Colour (0xff1e232b);
                if (lit)
                {
                    if (s == 0) segCol = juce::Colour (0xffff2222);       // Red clip
                    else if (s <= 2) segCol = juce::Colour (0xffffa000);  // Amber
                    else segCol = juce::Colour (0xff00e650);              // Green
                }
                g.setColour (segCol);
                g.fillRect (dotRect);
            }
        }

        // Bank Title at bottom
        g.setFont (juce::FontOptions (9.0f, isSel ? juce::Font::bold : juce::Font::plain));
        g.setColour (isSel ? juce::Colour (0xffe5eaf2) : juce::Colour (0xff8a94a2));
        g.drawText (bankNames[i], (int) btnRect.getX(), (int) btnRect.getBottom() - 14,
                    (int) btnRect.getWidth(), 12, juce::Justification::centred);
    }
}

void BankSelectorBar::resized()
{
    repaint();
}

void BankSelectorBar::mouseDown (const juce::MouseEvent& e)
{
    const auto bounds = getLocalBounds().toFloat();
    const float logoW = 90.0f;
    const float banksStartX = bounds.getX() + 2.0f + logoW + 4.0f;
    const float availW = bounds.getRight() - banksStartX - 4.0f;
    const float buttonW = availW / (float) NumBanks;

    if (e.position.x >= banksStartX && e.position.x <= bounds.getRight() - 4.0f)
    {
        const int idx = (int) ((e.position.x - banksStartX) / buttonW);
        if (idx >= 0 && idx < NumBanks)
            setSelectedBank (idx);
    }
}

} // namespace mps
