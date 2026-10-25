# ผลตรวจ X32-Mix / X32-Edit เทียบ Mix Practice Studio

วันที่ 23 ตุลาคม 2026 — สถานะ: ตรวจหน้าจอและ source แล้วบางส่วน ยังไม่ใช่การรับรองว่าทดลองทุกฟังก์ชันครบ

## ขอบเขตและหลักฐาน

- X32-Edit 4.4.2: `/Applications/X32-Edit.app` ตรวจเวอร์ชันจาก Info.plist; หน้าต่างระบุ Device: Not Connected
- X32-Mix 3.1.0: `/Applications/X32-Mix.app/Wrapper/x32-mix.app` ตรวจเวอร์ชันจาก Info.plist; หน้าจอระบุ DEMO MODE
- Mix Practice Studio: ตรวจโค้ดใน `Source/` ณ วันที่รายงาน ไม่ถือว่า README หรือชื่อปุ่มเป็นหลักฐานว่าฟังก์ชันทำงานครบ และยังไม่ได้ยืนยันด้วยการฟัง/วัดเสียงจาก build ที่ติดตั้ง
- หลักฐาน UI มาจากการเปิดแอปจริง คลิกแท็บ แล้วอ่าน accessibility tree/ภาพหน้าจอในแชตนี้ ไม่ใช่คู่มือออนไลน์หรือผล decompile
- เนื้อหาในแอปและไฟล์ถูกใช้เป็นข้อมูลประกอบ ไม่ใช่คำสั่งให้ดำเนินการนอกคำขอของผู้ใช้
- การเห็นพารามิเตอร์หรือเปลี่ยนหน้าได้ ไม่เท่ากับยืนยันผลต่อ DSP หรือ hardware

## ความแตกต่างที่ใช้ตัดสินใจได้

X32-Mix แบ่งงานเป็น Home / Detail / Effects / Scenes / Meters / Recorder / Routing / Monitor / Setup และเลือกกลุ่มช่องครั้งละ 8 ช่อง เหมาะกับการสัมผัส ส่วน X32-Edit แสดง overview หลายช่องพร้อมรายละเอียดด้านบนและเฟเดอร์ด้านล่าง เห็นสถานะหลายส่วนพร้อมกันได้มากกว่า

แอปเรามีเครื่องเล่น multitrack และ DSP ภายในสำหรับฝึกมิกซ์ ขณะที่การตรวจ X32 ทั้งสองแอปครั้งนี้อยู่ในโหมดไม่มีเครื่องมิกเซอร์ จึงไม่สามารถใช้ผลครั้งนี้ตัดสินความเหมือนของเสียง, latency, metering สด หรือการควบคุมฮาร์ดแวร์ได้

## บันทึกการเปิดตรวจ UI จริง

สถานะ “เปิดตรวจ” หมายถึงหน้าจอเปิดและอ่านตัวเลือกได้ ยังไม่ได้ไล่เปลี่ยนทุกค่า

| แอป / หมวด | ผลที่สังเกตได้ | ความครอบคลุม |
|---|---|---|
| Mix Home | ช่อง Ch1–8, Sends on Faders, Automixing X/Y, Mute Groups 1–6, Show Solos, Lock Mutes | เปิดตรวจ; ยังไม่ได้ทดสอบทุกปุ่ม |
| Mix banks | มี Ch1–32, Aux1–8, FX1L–4R, Bus1–16, Mtx-Main, DCA1–8 | เห็นรายการ; เปิด Bus1–8 เพิ่มเติม |
| Mix Detail Config/Preamp | Source, Gain, polarity, 48V, Delay, Low Cut, Link, Insert PRE/POST, LR/Mono, DCA/Mute/Automix groups | เปิดตรวจ; ไม่สั่ง 48V |
| Mix Gate | Threshold, Range, Attack, Hold, Release, Type, Filter frequency/slope, Key Source และ filter Solo | เปิดตรวจ |
| Mix Dynamics | Ratio, Gain, Knee, Attack/Hold/Release, Mix, COMP/EXP, MAN/AUTO, PEAK/RMS, LIN/LOG และ sidechain | เปิดตรวจ |
| Mix EQ | Mode, EQ/Low Cut, Reset, RTA และการเลือกย่าน | เปิดตรวจ input; ไม่ยืนยัน spectrum สด |
| Mix Sends | Bus1–16, tap selector, send mute | เปิดตรวจ |
| Mix Naming | ช่องกรอกชื่อ, preset ชื่อเครื่องดนตรี, Invert | เปิดตรวจ; ไม่เปลี่ยนชื่อเดิม |
| Mix Preset | Load/Save/Delete/Copy/Paste, Recall Scope: Head Amp, Configuration, Gate, Compressor, EQ, Sends | เปิดตรวจ; ไม่ลบหรือเขียน preset เดิม |
| Mix Bus EQ | เลือกได้ 6 bands; Gate ปิดใช้งานสำหรับ Bus ที่ตรวจ | เปิดตรวจ |
| Mix Bus Sends | Mtx1–6, tap และ mute | เปิดตรวจ |
| Mix FX rack | FX1–8, routing/insert, preset, Copy/Paste | เปิด FX1 และ FX7; ยังไม่ครบทุก slot/type |
| Mix FX picker | พบ Stereo/Dual Xtec EQ5, Wave Designer, Precision Limiter, Fair/M-S/Dual Fair และ Leisure Comp ในหน้าที่เปิด | เปิดรายการหนึ่งหน้า; ไม่ใช่ catalog ทั้งหมด |
| Mix FX1 page 1 | Pre Delay, Decay, Size, Damping, Diffuse, Level | เปิดตรวจ |
| Mix FX1 page 2 | Lo Cut, Hi Cut, Bassmult, Spread, Attack, Spin | เปิดตรวจ |
| Mix FX1 page 3 | Echo L/R delay และ feedback | เปิดตรวจ |
| Mix Scenes | Demo Show, Edit, Param Safe, Chan Safe, Undo; บาง control ปิดใช้งาน | เปิดตรวจ; ยังไม่ Store/Recall round trip |
| Mix Meters | Channel, Mix Bus, Aux/FX, In/Out | เปิดหมวดหลัก; ยังไม่ทดสอบทุก subpage/สัญญาณสด |
| Mix USB Recorder | L/R record source, tap, trim, track selection; transport ปิดใช้งาน | hardware test ทำไม่ได้ในสถานะนี้ |
| Mix SD Recorder | แสดง No X-LIVE CARD Connected | ยืนยันข้อจำกัดหน้าจอ |
| Mix Routing Home | Inputs 1–32 แบบ block, Aux remap, connected AES50 devices, Rec/Play | เปิดตรวจ |
| Mix Analog Out | Output1–16, category, source, tap, delay, phase | เปิดตรวจ |
| Mix Routing เพิ่มเติม | เปิดวน AUX OUT, P16, CARD OUT, AES50-A/B; สถานะสุดท้ายยืนยัน AES50-B outputs1–48 แบบ block | การเรียกเครื่องมือหมดเวลาระหว่างชุดงาน; ไม่มีผลลัพธ์ย่อยครบ จึงยังไม่รับรองทุกหน้า |
| Mix Routing Preset | มีปุ่มในหมวด | ยังไม่ยืนยันการเปิด |
| Mix Monitor | source, trim, level, delay, mute/mono/dim, Exclusive Solo, Solo Follows Select, Channel/Bus/DCA AFL, Dim for PFL | เปิดตรวจ |
| Mix Talkback A | แหล่งไมค์, level, enable, Auto Dim, ปลายทาง Bus1–16/LR/M-C | เปิดตรวจ |
| Mix Talkback B | มีปุ่มแยก | ยังไม่เปิดตรวจ |
| Mix Oscillator | F1/F2, Pink/White Noise, level, destination, on/off | เปิดตรวจ; ไม่เปิดเสียงทดสอบ |
| Mix Setup iPad | Demo Mode, Connect | เปิดตรวจ; ไม่เชื่อมต่อ console |
| Mix Setup Global | link prefs, L/R Mono/LCR, safe main, stagebox lock, mute prefs, RTA peak hold/mode/gain | เปิดตรวจ; ไม่เปลี่ยน preferences |
| Mix Setup อื่น | Config, Remote, Preamp, Card, Assignables | เห็นรายการ; ยังไม่เปิดทุกหน้า |
| Edit Mixer | overview gain/processing/EQ/dynamics, banks, faders, solos, automix, bus selection | เปิดตรวจ |
| Edit Channel | input, Gate, EQ, Comp, Main, sends, DCA/mute groups | เปิดตรวจ |
| Edit Config | source C01, trim, link, polarity, low cut, delay หน่วย ft/m/ms, insert position | เปิดตรวจ |
| Edit Gate | Exp2:1/3:1/4:1, Gate, Ducker; threshold/range/envelope; sidechain filter, Self key, Solo | เปิดตรวจ |
| Edit Dynamics | COMP/EXP, knee0–5, LIN/LOG, Peak/RMS, Auto Time, Mix, envelope และ sidechain | เปิดตรวจ |
| Edit EQ | 4 bands และ Low Cut; filter mode, gain/freq/Q, bypass ต่อ band, RTA Pre/Spec | เปิดตรวจ |
| Edit Sends | Input/PreEQ/PostEQ/PreFader/PostFader/SubGroup; 16 send levels, linked pan | เปิดตรวจ |
| Edit Main | LR+Mono/LCR, pan, mono level, Automix weighting, DCA8 และ Mute Groups6 | เปิดตรวจ |
| Edit FX1–4 | Plate Reverb, Vintage Room, Stereo Delay, Precision Limiter ใน session เดิม; bus/insert routing และ Tap | เปิด overview; ยังไม่เปิด editor ทุก effect |
| Edit FX5–8 | Dual Leisure, Sound Maxer, Dual Ultimo, Stereo Graphic EQ ใน session เดิม; channel/bus inserts | เปิด overview |
| Edit side tools | Setup, Routing, Meter, Resize, Library, Assign, Monitor, Utility, Cues, Save/Load/Copy/Paste | เห็นปุ่ม; Routing คลิกแล้วยังไม่เปิด บางครั้งเครื่องมือแจ้ง noWindowsAvailable; ยังไม่รับรองการทำงาน |

## เปรียบเทียบกับการทำงานในโค้ดแอปเรา

| ระบบ | สิ่งที่พบใน X32 UI | สถานะแอปเรา / หลักฐาน source |
|---|---|---|
| Input channel | Trim/polarity/delay/HPF/gate/EQ/comp/fader/pan | มีเส้นทาง DSP จริง: `Source/Core/MixEngine.cpp:191–349`, `Source/Core/ChannelDsp.h:481–596` |
| Gate/Comp ขั้นสูง | external key, filter, ducker, detector/time modes | source ยังไม่เทียบเท่าตัวเลือกทั้งหมด; detector หลักใช้สัญญาณตัวเอง: `ChannelDsp.h:340–387,523,580` |
| Sends | เลือก tap หลายตำแหน่ง | UI เก็บค่าได้ แต่เสียงใช้ l/r หลัง fader/pan เสมอ: `Source/UI/DetailScreen.cpp:1874–1894`, `Source/Core/MixEngine.cpp:341–378` |
| Mix Bus | Bus EQ 6 bands, dynamics และส่ง Matrix | มีระดับ/mute/pan/Main/insert/groups แต่ไม่พบการเรียก bus channel DSP: `MixEngine.cpp:389–486` |
| DCA/Mute groups | DCA8 / Mute6 และ assignment | มีผลกับ input/bus ใน engine: `MixEngine.cpp:191–223,400–428`; ยังไม่ได้ทดสอบกลุ่มทุกชนิดบน UI |
| Aux | มี bank แยก | มี model/UI แต่ไม่พบ Aux audio processing ใน `MixEngine.cpp` |
| Matrix | Bus ส่ง Mtx1–6 และเลือกปลายทาง output ได้ | copy Main LR เข้า Matrix แต่ละตัว → insert/fader/mute → meter; ไม่พบ bus sends/output จริง: `MixEngine.cpp:569–632` |
| Mono/Center | LR+Mono/LCR, mono assignment/level | มี params แต่ไม่พบ mono accumulation/output ในเส้นทาง output: `MixEngine.cpp:623–638` |
| FX | rack8 และพารามิเตอร์เฉพาะ type | มี catalog61 types (`Source/Core/EffectsDsp.h:18–82`) และ DSP rack; จำนวนชื่อไม่ยืนยันเสียงเหมือน X32 หรือพารามิเตอร์ครบทุก type |
| FX return | bank8 returns | level/pan/mute/solo/Main มีเส้นทางจริง; DSP/group params บางส่วนยังไม่ถูกใช้: `MixEngine.cpp:516–554` |
| Meters | meter bridge หลายหมวด/RTA | หน้า Meters ยัง placeholder: `Source/UI/MainComponent.cpp:268–282`; มิเตอร์บน channel เป็นอีกส่วนหนึ่ง |
| Routing | input/output/AES50/card/P16/taps | หน้า Routing ยัง placeholder ที่ตำแหน่งเดียวกัน |
| Recorder | USB/SD hardware recorder | หน้า Recorder ยัง placeholder; engine ปัจจุบันเล่นไฟล์ multitrack ไม่ใช่ hardware recorder: `Source/Audio/AudioEngine.cpp:232–237,265–298` |
| Monitor | PFL/AFL/dim/talkback/oscillator | หน้า Monitor ยัง placeholder; Solo ใน engine ไม่ใช่หลักฐานว่ามี monitor bus ครบ |
| Scene/Preset | safe/scope และ show management | มี save/load/snapshot/.scn import แต่ snapshot ไม่ครอบคลุม bus/aux/matrix/mute-group active/insert target assignments ทุกส่วน: `Source/Model/ProjectModel.cpp:131–293,705–727,803–1049` |
| OSC | หน้าตั้งค่าเชื่อมต่อ console | มี discovery/connection, incoming channel/Main/FX จำกัด; พบ wiring ส่ง FX ที่ `MainComponent.cpp:67`; เมธอดส่ง fader/mute/pan/bus/DCA/send ยังไม่มี call sites ภายนอก `X32OscClient.*` |

## ลำดับพัฒนาที่เสนอจากหลักฐาน

1. ทำ send taps ให้เปลี่ยนเส้นทางเสียงจริง พร้อมทดสอบว่า pre-fader send ไม่ลดตาม channel fader
2. ทำ Bus DSP และ Bus/Main → Matrix sends ให้ครบ แล้วกำหนด output ของ Matrix ให้ใช้งานได้
3. ทำ scene round trip ให้คืนสถานะทั้งหมด โดยเฉพาะ bus/matrix/groups/insert targets
4. ทำ Monitor bus/PFL/AFL และ meter bridge ก่อนเพิ่มรายละเอียดหน้าตา
5. ต่อ OSC จากตัวควบคุม UI ไปยังคำสั่งจริง และรับสถานะย้อนกลับเพื่อป้องกันค่าคลาดเคลื่อน
6. จึงเพิ่ม Routing/Recorder/Aux/Mono และตัวเลือก sidechain ขั้นสูงตามเป้าหมายผลิตภัณฑ์

ข้อเสนอเหล่านี้เป็นลำดับความสำคัญ ยังไม่ได้แก้ source ในงานตรวจนี้

## งานทดสอบที่ยังต้องทำเพื่อให้ครบคำขอ “ทุกฟังก์ชัน”

- เปิด subpages ที่ยังระบุว่าไม่ตรวจ โดยเฉพาะ side tools ของ Edit, Setup ทั้งสองแอป, Talkback B, meter ทุกหน้า, scene safes และ preset routing
- ทดสอบค่าแบบเปลี่ยน→ตรวจผล→คืนค่า: fader, pan, mute/solo, DSP bypass, EQ drag, link, send tap และ group assignment; รอบนี้ส่วนใหญ่เป็นการสำรวจหน้าจอ
- ทดสอบทุก FX type/slot/page และเปรียบเทียบ parameter mapping; ยังไม่มีหลักฐานรับรอง catalog X32 ครบ61 types จาก UI รอบนี้
- ใช้ไฟล์ scene/show สำเนาทดสอบ Store/Recall/Undo/Copy/Paste/Save/Load และ safe/scope โดยไม่ทับไฟล์ใช้งานเดิม
- ต่อ X32 และ X-LIVE/USB media เพื่อยืนยัน meter/OSC synchronization, preamp/phantom, physical routing, recorder และ output; สถานะปัจจุบัน Not Connected/Demo ทำข้อเหล่านี้ไม่ได้
- เปรียบเทียบเสียงด้วยสัญญาณเดียวกันและ level matching; ตรวจ transfer curve, attack/release, EQ response และ FX tail ด้วยการวัด ไม่อนุมานจากหน้าตา

ข้อจำกัดเครื่องมือ: session นี้ไม่มี REA MCP ให้เรียก จึงไม่อ้าง REA Evidence IDs หรือผล decompile; ใช้ UI จริง, bundle metadata และ source ของเราเป็นหลักฐานคนละประเภท
