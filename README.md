# Mix Practice Studio

**Mix Practice Studio** คือแอปพลิเคชันฝึกซ้อมและเรียนรู้การมิกซ์เสียงแบบมัลติแทร็ก (Multitrack Mixing Practice Studio) สำหรับ **Windows**, **macOS** และ **Android**
พัฒนาด้วย **C++17** และ **JUCE Framework 8** ออกแบบสถาปัตยกรรมระดับสตูดิโอมืออาชีพ โดยประมวลผลเสียงจริงในตัว (Real-time In-Engine DSP) ทำงานออฟไลน์ได้ 100% ไม่ใช่แอปรีโมตควบคุมมิกเซอร์ และไม่ต้องเชื่อมต่อฮาร์ดแวร์ภายนอก

---

## สารบัญ

1. [เป้าหมายและจุดเด่น](#เป้าหมายและจุดเด่น)
2. [สถาปัตยกรรมระบบเสียงและ Signal Flow](#สถาปัตยกรรมระบบเสียงและ-signal-flow)
3. [เทคโนโลยีและสิทธิ์การใช้งาน (Licensing)](#เทคโนโลยีและสิทธิ์การใช้งาน-licensing)
4. [โครงสร้างโค้ดโปรเจกต์](#โครงสร้างโค้ดโปรเจกต์)
5. [ฟังก์ชันที่พร้อมใช้งานในปัจจุบัน (Implemented Features)](#ฟังก์ชันที่พร้อมใช้งานในปัจจุบัน)
6. [การทดสอบและผลลัพธ์ (Automated Tests)](#การทดสอบและผลลัพธ์)
7. [คู่มือการติดตั้งแอปพลิเคชัน (Installation Guide)](#7-คู่มือการติดตั้งแอปพลิเคชัน-installation-guide)
   - [Android (.apk)](#android-ติดตั้งไฟล์-apk)
   - [Windows (.exe)](#windows-ติดตั้งไฟล์-exe)
   - [macOS (.app)](#macos-ติดตั้งไฟล์-app)
8. [ขั้นตอนการ Build จากซอร์สโค้ด (Developer Build Guide)](#8-ขั้นตอนการ-build-จากซอร์สโค้ด)
9. [คู่มือการใช้งานทีละขั้นตอน (Workflow Guide)](#9-คู่มือการใช้งานทีละขั้นตอน)
10. [ข้อจำกัดและแผนพัฒนาถัดไป (Roadmap)](#10-ข้อจำกัดและแผนพัฒนาถัดไป)

---

## 1. เป้าหมายและจุดเด่น

- **ประมวลผลเสียงจริงในตัว (True Audio DSP Engine):** ทุกตัวควบคุม (Trim, Polarity, Delay, HPF, Gate, EQ, Compressor, Fader, Pan) ส่งผลต่อเสียงจริงแบบเรียลไทม์
- **Zero-Drift Synchronized Multitrack Player:** เล่นไฟล์เสียงหลายแทร็กพร้อมกันด้วยตัวอ่านตำแหน่งเวลาเดียว (Single Shared Timeline Streamer) บน Ring Buffer แบบ Lock-Free ทำให้แทร็กไม่มีวันคลาดเคลื่อนจากกัน
- **Offline Zero-Phase Resampler:** แปลง Sample Rate ของไฟล์เสียงที่ต่างจาก Audio Device โดยอัตโนมัติด้วยอัลกอริทึม Kaiser-Windowed Sinc (Phase-linear) ลงไฟล์แคชชั่วคราว จึงไม่มี latency และไม่กิน CPU บน audio thread
- **Professional Console UX/UI:** อินเทอร์เฟซโทนสีเข้ม (Dark Charcoal/Slate) สไตล์ดิจิทัลคอนโซล X32/M32 วาดด้วย Custom Vector Graphics ทั้งหมด (ไม่มีการใช้รูปแคปเจอร์เป็นพื้นหลัง และไม่มีเครื่องหมายการค้าที่มีลิขสิทธิ์)
- **Lock-Free Thread Safety:** แยกเธรดเสียง (Audio Thread), เธรดอ่านดิสก์ (Disk Streaming Thread) และเธรดหน้าจอ (UI Thread 30 FPS) ออกจากกันอย่างเด็ดขาด ปราศจากการจองหน่วยความจำ (Zero Allocation) และ Lock-Free 100% ใน Audio Callback

---

## 2. สถาปัตยกรรมระบบเสียงและ Signal Flow

### เส้นทางสัญญาณต่อช่องสัญญาณอินพุต (Input Channel Strip Flow)

```
[Audio File (WAV/AIFF/FLAC)]
            │
            ▼
   [Source Mode Selector] ── Native Stereo / Mono Sum / Left Only / Right Only
            │
            ▼
   [Source Switch Fade] ── Crossfade ป้องกันเสียงคลิกเวลาสลับแทร็ก (10 ms)
            │
            ▼
   [Digital Trim & Polarity] ── ปรับเกนดิจิทัล -18 ถึง +18 dB พร้อมกลับเฟส 180°
            │
            ▼
      [Input Delay] ── หน่วงเวลา 0 ถึง 500 ms พร้อมสวิตช์ Bypass
            │
            ▼
    [High-Pass Filter] ── 12 dB/oct Butterworth Low-Cut (20 Hz - 400 Hz)
            │
            ▼
       [Noise Gate] ── Expander/Gate พร้อม Threshold, Range, Attack, Hold, Release
            │
            ▼
   [4-Band Parametric EQ] ── Low Shelf/Bell, Mid1, Mid2, High Shelf/Bell (20 Hz - 20 kHz)
            │
            ▼
  [Dynamics Compressor] ── Soft-Knee Compressor พร้อม Makeup Gain, Attack, Release, Dry/Wet
            │
            ▼
  ┌───────────────────────┐
  │ PRE-FADER METER TAP   │ ── วัดระดับสัญญาณและตรวจจับ Clip (>= 0 dBFS)
  └───────────────────────┘
            │
            ▼
   [Fader x Mute x Pan] ── คอนโซลเฟเดอร์ Taper จริง (-90 ถึง +10 dB), Mute, Pan Law (-3 dB Centre)
            │
            ▼
  ┌───────────────────────┐
  │ POST-FADER METER TAP  │
  └───────────────────────┘
            │
            ▼
  [Solo-In-Place & Main LR Gate] ── ระบบ Solo ตัดเสียงช่องอื่น และส่งเข้า Main LR Bus
            │
            ▼
     [Main LR Summing]
            │
            ▼
     [Main LR Fader & Mute] ── สเตอริโอมาสเตอร์เฟเดอร์
            │
            ▼
  ┌───────────────────────┐
  │ MAIN STEREO METERS    │ ── L/R Peak Meters พร้อม Peak Hold และ Clip LEDs
  └───────────────────────┘
            │
            ▼
   [DAC Hardware Output]
```

---

## 3. เทคโนโลยีและสิทธิ์การใช้งาน (Licensing)

- **ภาษาหลัก:** C++17
- **เฟรมเวิร์ก:** JUCE Framework 8 (เวอร์ชัน 8.0.15)
  - โมดูลที่ใช้: `juce_core`, `juce_events`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_processors`, `juce_audio_utils`
  - **เงื่อนไขสิทธิ์:** Dual-licensed ภายใต้ GNU AGPLv3 (โอเพนซอร์ส) หรือ Commercial License ของ JUCE
- **Build System:** CMake (ขั้นต่ำ 3.22) + Ninja
- **ไม่ใช้ Third-Party Black-Box DSP:** อัลกอริทึมคณิตศาสตร์ DSP ทั้งหมด (Biquad EQ, Butterworth HPF, Noise Gate, Compressor, Sinc Resampler, Ring Buffer) เขียนขึ้นเองแบบ Pure C++ ไม่มี dependency ซ่อนเร้น

---

## 4. โครงสร้างโค้ดโปรเจกต์

```
appmultitrack/
├── CMakeLists.txt                # ไฟล์จัดการการคอมไพล์โปรเจกต์หลักและ Tests
├── README.md                     # เอกสารอธิบายระบบและคู่มือ
├── external/
│   └── JUCE/                     # ซอร์สโค้ด JUCE Framework 8.0.15
├── Source/
│   ├── Main.cpp                  # จุดเริ่มต้นแอปพลิเคชัน (JUCEApplication & MainWindow)
│   ├── Core/                     # โค้ด Core DSP และ Transport (Pure C++17 แยกอิสระ)
│   │   ├── DspMath.h             # ค่าคงที่, การแปลง dB, Fader Taper, Pan Laws, Smoother, Param
│   │   ├── ChannelDsp.h          # Biquad Filter, DelayLine, GateDsp, CompressorDsp, ChannelDspProcessor
│   │   ├── MixerParams.h         # โครงสร้างตัวแปร Atomic Parameters และ Atomic Meter Peaks
│   │   ├── MixEngine.h/.cpp      # Engine การมิกซ์สัญญาณหลายช่องแบบ Real-time Safe
│   │   ├── TimelineStreamer.h/.cpp # Ring buffer แบบมัลติแทร็กตัวอ่านเดียว ป้องกันแทร็กเลื่อนหลุด
│   │   └── SincResampler.h/.cpp  # Kaiser-windowed zero-phase sinc resampler สำหรับออฟไลน์แคช
│   ├── Audio/                    # การเชื่อมต่อระบบเสียงของ JUCE
│   │   ├── AudioFileTrack.h/.cpp # ตัวอ่านไฟล์เสียงสตรีมมิ่งจากดิสก์ (WAV, AIFF, FLAC)
│   │   └── AudioEngine.h/.cpp    # โฮสต์ AudioDeviceManager และ Disk Streaming Worker Thread
│   ├── Model/                    # การจัดเก็บโปรเจกต์และ Scene
│   │   └── ProjectModel.h/.cpp   # บันทึก/โหลดไฟล์โปรเจกต์ (.mpsproj) แบบ Versioned JSON และ Scene Snapshots
│   └── UI/                       # ส่วนติดต่อผู้ใช้แบบ Vector Graphics
│       ├── ConsoleLookAndFeel.h/.cpp  # ธีมคอนโซล ลูกบิด ปุ่มกด เฟเดอร์
│       ├── LedMeter.h/.cpp            # มิเตอร์ LED แท่งแนวตั้ง เขียว/เหลือง/แดง พร้อม Clip Latch
│       ├── InteractiveEqGraph.h/.cpp  # กราฟ EQ ลอการิทึม 20Hz-20kHz ลากปรับจุดความถี่/เกนได้
│       ├── ChannelStripComponent.h/.cpp # Channel Strip: Scribble, Pan, Solo, Mute, Fader, Meter
│       ├── TransportBar.h/.cpp        # แถบควบคุม Play, Pause, Stop, Seek, Loop, เวลา, CPU
│       ├── HomeScreen.h/.cpp          # หน้า Home แสดง Channel Strips 8 ช่อง + Master Strip
│       ├── DetailScreen.h/.cpp        # หน้า Detail แสดงแท็บ Config, Gate, Dyn, EQ, Sends, Naming
│       ├── TracksScreen.h/.cpp        # หน้านำเข้าไฟล์ WAV และกำหนดแทร็กลงช่องมิกเซอร์
│       ├── ScenesScreen.h/.cpp        # หน้าบันทึก/โหลด Scene และ Project
│       └── MainComponent.h/.cpp       # Navigation Coordinator และคีย์ลัด Spacebar
├── Tests/
│   ├── TestHarness.h             # ชุดรันการทดสอบ Unit Test น้ำหนักเบา
│   └── CoreTests.cpp             # รวม 23 ชุดการทดสอบ DSP และ Transport ครอบคลุม 10,039 checks
└── android/                      # โครงสร้างสำหรับ Build บน Android
    ├── build.gradle              # Root Gradle build script
    ├── settings.gradle           # Gradle settings
    ├── gradle.properties         # JVM และ AndroidX configurations
    └── app/
        ├── build.gradle          # App Gradle script เชื่อม CMakeLists.txt
        └── src/main/
            ├── AndroidManifest.xml # กำหนดสิทธิ์และบังคับแนวนอน (Landscape)
            └── res/values/strings.xml
```

---

## 5. ฟังก์ชันที่พร้อมใช้งานในปัจจุบัน

### 1. Multitrack Playback & Transport
- [x] นำเข้าไฟล์เสียง WAV, AIFF, FLAC ได้หลายแทร็กพร้อมกัน
- [x] ตัวเล่นตำแหน่งเวลากลางตัวเดียว (Single Timeline Streamer) รับประกันว่าทุกแทร็กเล่นตรงกัน 100% ไม่หลุดจังหวะ
- [x] รองรับไฟล์ที่มีความยาวต่างกัน โดยแทร็กที่สั้นกว่าจะกลายเป็นความเงียบ (Silence) เมื่อเล่นจบ
- [x] แปลง Sample Rate อัตโนมัติด้วย Zero-Phase Sinc Resampler เพื่อรักษา Time Alignment
- [x] ควบคุม Play, Pause, Stop, Seek บนแถบ Timeline และ Loop A-B แบบไร้เสียงคลิก (Smooth Fade)

### 2. Channel Processing (Real DSP)
- [x] **Config / Preamp:** Digital Trim (-18 ถึง +18 dB), Polarity Invert (0°/180°), Source Selector, Input Delay (0-500 ms พร้อม Bypass), 48V Phantom (ระบุชัดเจนว่า Sim only)
- [x] **High-Pass Filter:** Butterworth 12 dB/oct (20 Hz - 400 Hz) พร้อมสวิตช์ Bypass
- [x] **Noise Gate:** Threshold (-80 ถึง 0 dB), Range (-60 ถึง 0 dB), Attack (0.1-100 ms), Hold (0-1000 ms), Release (10-2000 ms), Gain Reduction Meter
- [x] **4-Band Parametric EQ:** Low Shelf/Bell, Mid 1 Bell, Mid 2 Bell, High Shelf/Bell (20 Hz - 20 kHz, Gain ±15 dB, Q 0.3-10) แสดงผลกราฟความถี่ตามค่าสัมประสิทธิ์จริง พร้อมลากจุดปรับได้
- [x] **Dynamics Compressor:** Threshold (-60 ถึง 0 dB), Ratio (1:1 ถึง 20:1), Soft Knee (0-12 dB), Attack, Release, Makeup Gain (0-24 dB), Dry/Wet Mix, Gain Reduction Meter

### 3. Mixing & Console Controls
- [x] **Channel Strips:** Fader Taper มาตรฐานคอนโซล (-90 ถึง +10 dB), ป้อนค่าตัวเลข dB ได้
- [x] **Pan Law:** Constant-power sin/cos law (-3 dB ที่ตรงกลาง) สำหรับแทร็กโมโน และ Stereo Balance สำหรับสเตอริโอ
- [x] **Mute & Solo-In-Place:** Mute ตัดเสียงทันทีแบบมี ramp นุ่มนวล, Solo ฟังเฉพาะช่องที่เลือก โดย Mute มีสิทธิ์ตัดเสียงก่อน Solo เสมอ
- [x] **Master Stereo Strip:** มาสเตอร์เฟเดอร์แยกอิสระ พร้อม L/R Meter และ Clip Indicator
- [x] **Channel Banks:** สลับดูช่อง CH 1-8, CH 9-16, CH 17-24, CH 25-32 ได้ทันที

### 4. Project & Scene Storage
- [x] **Versioned JSON:** บันทึกโครงสร้างแทร็ก ค่ามิกเซอร์ การกำหนดช่อง และ Metadata
- [x] **Relative Audio Paths:** ใช้ Relative Path ทำให้ย้ายโฟลเดอร์โปรเจกต์ข้ามเครื่องได้โดยไฟล์ไม่หาย
- [x] **Copy Audio into Project:** มีตัวเลือกคัดลอกไฟล์เสียงเข้าโฟลเดอร์ `Audio/` ของโปรเจกต์
- [x] **Missing Files Detection:** ตรวจสอบไฟล์ที่หาไม่พบและแจ้งเตือนอย่างชัดเจน
- [x] **Scene Snapshots:** เก็บ Snapshot ค่ามิกเซอร์ไว้สลับเปรียบเทียบเสียงได้ทันทีโดยไม่หยุดเล่น

---

## 6. การทดสอบและผลลัพธ์

ชุดทดสอบอัตโนมัติ `MixPracticeCoreTests` รันและผ่านการตรวจสอบทั้งหมด **23 หมวดการทดสอบ (10,039 checks, 0 failed checks)**:

```
=== MixPracticeCoreTests: 23 tests ===
[ OK ] dbToLinearConversion                  # ตรวจสอบความถูกต้องของการแปลง dB เป็น Linear และย้อนกลับ
[ OK ] faderUnityAndTaper                    # ตรวจสอบ Unity Gain (0 dB = 1.0) และความต่อเนื่องของ Taper
[ OK ] panLaws                               # ตรวจสอบ Constant-power -3 dB centre และ Balance laws
[ OK ] mixUnityMonoCentre                    # ตรวจสอบการส่งสัญญาณโมโนออกสเตอริโอ L/R
[ OK ] mixTrimFaderPolarityMainAssign        # ตรวจสอบ Trim, Polarity, Main Assign ส่งผลถูกต้อง
[ OK ] mixMuteSoloBehaviour                  # ตรวจสอบกฎ Mute ชนะ Solo และ Solo-in-place
[ OK ] mixStereoSourceModes                  # ตรวจสอบโหมด Native, MonoSum, LeftOnly, RightOnly
[ OK ] mixMeterTaps                          # ตรวจสอบจุดวัด Pre-fader และ Post-fader ว่าถูกต้องตามตำแหน่ง
[ OK ] mixGainChangesAreSmoothed             # ตรวจสอบ Parameter Smoothing ปราศจากเสียงคลิก
[ OK ] transportImpulsesStayAligned          # ตรวจสอบว่าแทร็กที่มี Impulse ตำแหน่งเดียวกันเล่นตรงกัน 100%
[ OK ] transportSeekPauseKeepAlignment       # ตรวจสอบว่าการ Seek สุ่มและการ Pause ไม่ทำให้แทร็กหลุดแนว
[ OK ] transportSeekLandsOnRequestedFrame    # ตรวจสอบว่าการ Seek ไปที่ Sample ใด จะเริ่มเล่นที่ Sample นั้น
[ OK ] transportLoopAB                       # ตรวจสอบการวน Loop A-B ไม่หลุดจังหวะ
[ OK ] transportShortTrackPlaysSilenceAndStopsAtEnd # ตรวจสอบแทร็กสั้นปล่อย Silence และหยุดเมื่อจบแทร็กยาว
[ OK ] transportThreadedStress               # ทดสอบ Stress Test มัลติเธรดระหว่าง Disk Reader และ Audio Thread
[ OK ] resamplerLengthDcAndSine              # ตรวจสอบความแม่นยำของ Sinc Resampler
[ OK ] resamplerKeepsTimeAlignment           # ตรวจสอบว่า Resampling รักษาตำแหน่งเวลาของ Impulse เดิม
[ OK ] delayLineAccuracy                     # ตรวจสอบความแม่นยำของ Sample Delay
[ OK ] hpfResponse                           # ตรวจสอบการตอบสนองความถี่ Butterworth HPF (-3 dB ที่ cutoff)
[ OK ] parametricEqBiquadResponse            # ตรวจสอบความถูกต้องของค่าสัมประสิทธิ์ Biquad EQ ทั้ง 4 ย่าน
[ OK ] gateAction                            # ตรวจสอบการเปิด-ปิดของ Noise Gate และ Gain Reduction
[ OK ] compressorAction                      # ตรวจสอบการบีบอัดสัญญาณ Soft-Knee Compressor
[ OK ] channelDspEndToEndProcessing          # ตรวจสอบการประมวลผลทั้ง Chain ต่อเนื่อง ปราศจาก NaN หรือ Inf
=== MixPracticeCoreTests: 23/23 tests passed, 10039 checks, 0 failed checks ===
```

---

## 7. คู่มือการติดตั้งแอปพลิเคชัน (Installation Guide)

> 📖 **ดูคู่มือการติดตั้งฉบับเต็มอย่างละเอียดได้ที่:** [INSTALL.md](file:///Users/premtodev/Documents/ระบบต่างๆ/appmultitrack/INSTALL.md)

| ระบบปฏิบัติการ | ไฟล์ที่ใช้ | วิธีการติดตั้งโดยสังเขป |
| :--- | :--- | :--- |
| **Android** | `MixPracticeStudio.apk` | ติดตั้งผ่านตัวจัดการไฟล์ หรือ `adb install -r MixPracticeStudio.apk` |
| **Windows** | `Mix Practice Studio.exe` | เป็นแบบ Portable แตกไฟล์/วางโฟลเดอร์ที่ต้องการแล้วเปิดใช้งานได้ทันที |
| **macOS** | `Mix Practice Studio.app` | ลากตัวแอปเข้าโฟลเดอร์ `/Applications` |

---

### Android (ติดตั้งไฟล์ .apk)
1. คัดลอกไฟล์ `MixPracticeStudio.apk` ลงบนโทรศัพท์หรือแท็บเล็ต Android (แนะนำ Android 10+ ขึ้นไป)
2. แตะเปิดไฟล์ผ่านแอป **Files (ตัวจัดการไฟล์)**
3. หากระบบเตือนเรื่องความปลอดภัย:
   - ไปที่ **การตั้งค่า (Settings)** > เปิด **อนุญาตจากแหล่งที่มานี้ (Allow from this source)**
   - หาก Google Play Protect เตือน ให้กด **รายละเอียดเพิ่มเติม (More details)** > **ติดตั้งต่อไป (Install anyway)**
4. แอปถูกออกแบบให้ทำงานใน **แนวนอน (Landscape)** และขอแนะนำให้เสียบหูฟังแบบมีสายเพื่อลดความหน่วงเสียง

---

### Windows (ติดตั้งไฟล์ .exe)
1. นำไฟล์ `Mix Practice Studio.exe` ไปวางไว้ในโฟลเดอร์ที่คุณต้องการ (เช่น `C:\Program Files\Mix Practice Studio\`)
2. คลิกขวา > **Send to > Desktop** เพื่อสร้างทางลัด (Shortcut)
3. หากเปิดครั้งแรกแล้วมีหน้าต่างสีฟ้า **Windows protected your PC (SmartScreen)**:
   - คลิกที่คำว่า **"More info" (ข้อมูลเพิ่มเติม)**
   - คลิกปุ่ม **"Run anyway" (เรียกใช้ต่อไป)**
4. แนะนำให้เชื่อมต่อ Audio Interface และเลือกใช้งานระบบเสียงแบบ **ASIO Driver** เพื่อให้ได้ Latency ต่ำที่สุด

---

### macOS (ติดตั้งไฟล์ .app)
1. ลากไฟล์ `Mix Practice Studio.app` ไปวางในโฟลเดอร์ **/Applications**
2. **การปลดล็อก Gatekeeper สำหรับการเปิดครั้งแรก:**
   - **วิธีที่ 1 (ผ่าน Finder):** คลิกขวา (หรือกด Control ค้างไว้แล้วคลิก) ที่ไอคอนแอป เลือก **Open (เปิด)** แล้วกดปุ่ม **Open** ยืนยันอีกครั้ง
   - **วิธีที่ 2 (ผ่าน Terminal):** หากระบบแจ้งว่าแอปเสียหาย (is damaged) ให้รันคำสั่ง:
     ```bash
     xattr -cr "/Applications/Mix Practice Studio.app"
     ```
3. เมื่อเปิดแอปและระบบขอสิทธิ์เข้าถึง Microphone/Audio ให้กด **Allow (อนุญาต)**

---

## 8. ขั้นตอนการ Build จากซอร์สโค้ด (Developer Build Guide)

### การ Build บน macOS

#### ข้อกำหนดเบื้องต้น:
- macOS 12+ (รองรับทั้ง Apple Silicon M1/M2/M3/M4 และ Intel)
- Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.22 ขึ้นไป และ Ninja (`brew install cmake ninja`)

#### คำสั่งในการ Build:
```bash
# 1. เข้าสู่โฟลเดอร์โปรเจกต์
cd /path/to/appmultitrack

# 2. สร้างโฟลเดอร์ build และกำหนดค่าผ่าน CMake
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release

# 3. รันการทดสอบ Unit Tests
ninja -C build MixPracticeCoreTests
./build/MixPracticeCoreTests

# 4. Build แอปพลิเคชัน GUI สำหรับ macOS
ninja -C build MixPracticeStudio

# 5. รันแอปพลิเคชัน
open "build/MixPracticeStudio_artefacts/Release/Mix Practice Studio.app"
```

---

### การ Build บน Windows

#### ข้อกำหนดเบื้องต้น:
- Windows 10/11 64-bit
- Visual Studio 2022 Community (Desktop development with C++)
- CMake 3.22+

#### คำสั่งในการ Build:
```bash
cmake -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Release --target MixPracticeStudio
```

---

### การ Build สำหรับ Android

#### ข้อกำหนดเบื้องต้น:
- Android Studio (Hedgehog / Iguana หรือใหม่กว่า)
- Android SDK Platform 34
- Android NDK (เวอร์ชัน 26.1 ขึ้นไป)
- JDK 17

#### ขั้นตอนการเปิดและ Build ใน Android Studio:
1. เปิด Android Studio
2. เลือก **Open** แล้วชี้ไปที่โฟลเดอร์ `android/` ภายในโปรเจกต์
3. รอให้ Gradle ซิงค์เสร็จสิ้น (จะตรวจจับ CMake จาก `CMakeLists.txt` โดยอัตโนมัติ)
4. เลือก Build Variant เป็น **Release** หรือ **Debug**
5. เชื่อมต่ออุปกรณ์ Android หรือเปิด Emulator แล้วกด **Run (Shift + F10)** เพื่อติดตั้ง APK ลงบนอุปกรณ์

*หมายเหตุความซื่อสัตย์ทางเทคนิค (Verification Disclaimer):*
- ตัวซอฟต์แวร์บน macOS ได้รับการคอมไพล์ ลิงก์ และทดสอบรันผลจริงบนระบบ macOS arm64 แล้ว 100%
- โครงสร้างและ Configuration ฝั่ง Android ได้ถูกเตรียมไว้ครบถ้วนสมบูรณ์ตามข้อกำหนดของ JUCE/NDK แต่ยังไม่ได้รันบนอุปกรณ์ Android ฮาร์ดแวร์จริงในเซสชันนี้

---

## 9. คู่มือการใช้งานทีละขั้นตอน (Workflow Guide)

1. **เปิดแอปพลิเคชัน:** รัน `Mix Practice Studio` หน้าจอเริ่มต้นจะแสดงหน้า **Home** พร้อม Channel Strips
2. **นำเข้าไฟล์เสียง (Import Tracks):**
   - คลิกที่แท็บ **Tracks** บนแถบเมนูด้านบน
   - คลิกปุ่ม **IMPORT AUDIO FILES (WAV)**
   - เลือกไฟล์เสียงมัลติแทร็ก (เช่น Kick.wav, Snare.wav, Bass.wav, Vocal.wav เป็นต้น)
   - ระบบจะจัดไฟล์เข้าสู่ Track 1, 2, 3... โดยอัตโนมัติ พร้อมคำนวณและเตรียมแคช Sample Rate หากไฟล์มีค่าต่างจากฮาร์ดแวร์
3. **กำหนดช่องสัญญาณ (Channel Assignment):**
   - ระบบจะ Map แทร็กเสียงเข้ากับช่อง CH 1, CH 2, CH 3... โดยอัตโนมัติ หรือสามารถสลับในหน้า **Detail > Config > Source Selector** ได้
4. **เริ่มเล่นเสียง (Playback & Transport):**
   - กดปุ่ม **PLAY** หรือกดปุ่ม **Spacebar** บนคีย์บอร์ด
   - สังเกตแถบเวลาและเข็มวัดระดับสัญญาณ LED Meters ที่จะขยับตามเสียงจริง
   - สามารถลากแถบ Seek Bar เพื่อเลื่อนเวลา หรือเปิดปุ่ม **LOOP** เพื่อซ้อมฟังท่อนที่ต้องการซ้ำ ๆ
5. **ฝึกมิกซ์เสียง (Mix & DSP Tweaking):**
   - ปรับ Balance ระดับเสียงด้วย **Channel Faders** และ **Pan Dials**
   - กดปุ่ม **SOLO** หรือ **MUTE** เพื่อเช็กชิ้นดนตรี
   - คลิกที่ช่องที่ต้องการ แล้วเลือกแท็บ **Detail** เพื่อเข้าสู่โหมดปรับแต่งเชิงลึก:
     - **Config:** ปรับ Digital Trim และเปิด HPF ตัดย่านเสียงกวนต่ำ
     - **Gate:** ปรับ Threshold เพื่อตัดเสียงเล็ดลอด (Bleed) ระหว่างไมค์กลอง
     - **EQ:** ลากจุดบนกราฟเพื่อเพิ่มความคมชัดหรือตัดความถี่ที่บวม
     - **Dynamics:** ปรับคอมเพรสเซอร์เพื่อควบคุม Dynamic Range ของเครื่องดนตรี
6. **บันทึกโปรเจกต์และ Scene (Save & Recall):**
   - ไปที่แท็บ **Scenes**
   - พิมพ์ชื่อ Scene ในช่อง เช่น `"Vocal Up Mix"` แล้วกด **STORE SCENE**
   - กดปุ่ม **SAVE PROJECT** เพื่อบันทึกไฟล์โปรเจกต์ `.mpsproj` ลงเครื่อง พร้อมเลือกบันทึกสำเนาไฟล์เสียงด้วย **Copy audio into project folder**

---

## 10. ข้อจำกัดและแผนพัฒนาถัดไป (Roadmap)

- **เฟสปัจจุบันที่สำเร็จแล้ว:** Phase 1 (Working Vertical Slice) & Phase 2 (Channel DSP Processing) สมบูรณ์พร้อมใช้งาน
- **Phase 3 (Bus & FX):** การเพิ่ม Aux Bus 4-16 ช่อง, Sends on Faders, Effect Rack (Reverb / Delay)
- **Phase 4 (Expansion):** DCA Groups, Mute Groups, Real-time RTA Spectrum Analyzer บนหน้าต่าง EQ, Stereo Mixdown Export (WAV Bounce)

