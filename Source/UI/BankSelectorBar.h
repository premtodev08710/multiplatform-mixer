// Mix Practice Studio - Console Bank Selector Bar with Miniature Multi-Meter Displays
#pragma once

#include "ConsoleLookAndFeel.h"
#include "../Core/MixerParams.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace mps
{

class BankSelectorBar : public juce::Component
{
public:
    enum BankIndex
    {
        BankCh1_8 = 0,
        BankCh9_16,
        BankCh17_24,
        BankCh25_32,
        BankAux1_8,
        BankFx1_4,
        BankBus1_8,
        BankBus9_16,
        BankMtxMain,
        BankDca1_8,
        NumBanks
    };

    BankSelectorBar();
    ~BankSelectorBar() override = default;

    void setSelectedBank (int bankIndex, bool sendNotification = true);
    int getSelectedBank() const noexcept { return selectedBank; }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

    void setMeters (const MixerMeters* m) { meters = m; }
    void refreshMeters();

    std::function<void (int)> onBankSelected;

private:
    int computeLitSegments (int bankIdx, int colIdx) const noexcept;

    int selectedBank = 0;
    const MixerMeters* meters = nullptr;
    std::array<uint8_t, NumBanks * 8> cachedLitSegments {};
};

} // namespace mps
