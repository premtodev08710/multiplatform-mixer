# ตรวจอาการเสียงแตกเมื่อเพิ่มแทร็ก — 1 พฤศจิกายน 2026

## ขอบเขตและข้อสรุป

ผู้ใช้ยืนยันอุปกรณ์ vivo X200 FE, ไฟล์ WAV, แตกทั้งลำโพงเครื่องและทางออกเสียงอื่น ผู้ใช้รายงาน Android เล่นหนึ่งแทร็กได้ แต่เมื่อเพิ่มแทร็กที่สองเสียงแตกต่อเนื่อง แม้มิเตอร์ไม่เกิน และรุ่นก่อนเพิ่ม iOS บนหน้าเว็บเคยทำงานปกติ

พบและแก้ข้อผิดพลาดในเส้นทางเพิ่มแทร็กขณะเล่นและเปลี่ยน sample rate มีการทดสอบให้โค้ดก่อนแก้ล้มเหลวแล้วผ่านหลังแก้ แต่ **ยังไม่ยืนยันว่าแก้เสียงแตกต่อเนื่องบนโทรศัพท์ของผู้ใช้ครบทั้งหมด**: ไม่มีอุปกรณ์ใน `adb devices` และยังไม่มีไฟล์/โปรเจกต์ที่เกิดอาการจริง การทดสอบด้านล่างใช้สัญญาณและไฟล์ที่สร้างขึ้นบน Mac

เทียบประวัติช่วงก่อน iOS โดยใช้ `376cbe9` (20 ต.ค.) เป็นจุดอ้างอิงที่มีใน repository ไม่ได้ยืนยันว่าเป็น APK ตัวเดียวกับที่ผู้ใช้จำได้ การเพิ่มหน้าเว็บ iOS เองยังไม่มีหลักฐานว่าเป็นสาเหตุ

## ข้อผิดพลาดและการแก้

1. **Live backfill ไม่ได้ขอสิทธิ์เขียนบัฟเฟอร์คืนจาก audio consumer** (`TimelineStreamer`, เพิ่มเข้ามาใน `9d0921e`, 29 ต.ค.)
   - เดิมเขียนทับเฟรมที่เผยแพร่ไปแล้ว โดยเผื่อระยะเพียง 1,024 เฟรม ขณะ consumer ยัง memcpy ทุกแทร็ก แม้แทร็กนั้นยังไม่เปิด channel count ระยะเผื่อเวลาไม่ใช่การรับประกันการทำงานคนละเธรดตาม C++ memory model
   - เพิ่ม request/acknowledgement แบบ atomic พร้อมหมายเลขการเปลี่ยนแทร็ก consumer ลดเสียงเฉพาะ slot ที่เปลี่ยนจนเป็นศูนย์และหยุดอ่าน slot นั้นก่อนตอบรับ producer จึงเขียนได้ แล้ว consumer ค่อยเพิ่มเสียงกลับ แทร็กที่ไม่เปลี่ยนยังเล่นต่อ
   - ไม่เพิ่ม mutex, การอ่านไฟล์ หรือการจองหน่วยความจำใน audio callback
   - snapshot จำนวนช่อง mono/stereo ให้ตรงกับ pull นั้น ป้องกัน metadata เปลี่ยนหลังปล่อยสิทธิ์เขียนแต่ก่อนมิกซ์

2. **แทร็กใหม่เริ่มแบบขั้นบันไดหลังช่วงเงียบ 1,024 เฟรม**
   - การทดสอบ DC 0.2 ซึ่งต่ำกว่า full scale พบ sample step 0.2 ในโค้ดเดิม
   - หลังแก้ step ไม่เกิน 0.2 / 256 + tolerance โดยแทร็กแรกไม่มี dropout

3. **Backfill ไม่รักษา fade ตรงจุดวน loop**
   - แทร็กเดิมมี loop fade แต่แทร็กที่เพิ่มใหม่ถูกอ่านดิบทับลงบัฟเฟอร์ ทำให้ช่วงวนไม่ตรงกันและเกิดเสียงกระแทกได้
   - เก็บ envelope ของแต่ละเฟรมแล้วใช้ซ้ำตอน backfill
   - ทดสอบก่อนแก้ล้มเหลว 2,044 checks; หลังแก้ผ่าน ครอบคลุม slot 32 และ stereo ด้วย

4. **เปลี่ยน sample rate แล้วเตรียม DSP/บัฟเฟอร์ซ้ำขณะ callback ทำงาน** (`AudioEngine::setSampleRate`, เพิ่มใน 29 ต.ค.)
   - การ pause transport ไม่ได้หยุด audio callback เดิมเรียก reinitialize หลัง device เริ่มกลับมาส่ง callback แล้ว
   - ถอด callback ก่อนเปลี่ยนการตั้งค่า ให้ aboutToStart เตรียมระบบครั้งเดียวก่อนกลับมาเล่น ไม่มีการ realloc scratch buffers แข่งกับ callback ในเส้นทางนี้
   - ยืนยันการเปิด/แปลงไฟล์และ state ด้วย integration tests; การสลับ hardware จริงยังไม่ได้ทดสอบกับ Android

5. **สถานะจบเพลงเดิมค้างเมื่อเพิ่ม/เปลี่ยนแทร็กระหว่างเล่น**
   - ล้าง end marker เดิมเมื่อ producer รับ track set ใหม่ แล้วประกาศจบใหม่ตามความยาวจริง

## การวินิจฉัยบนเครื่องจริง

เพิ่มสถานะใน Transport:
- `Drop: N`: ยอด device dropouts + stream underruns ใน session
- `Out CLIP`: พบสัญญาณเกิน full scale ที่เอาต์พุตหลัง monitor/solo gain ก่อน output protection
- Tooltip แยกยอด device, stream และ output overload blocks

Main meter อยู่ก่อนการเลือก monitoring/solo และ monitor gain จึงไม่ใช่ค่าของ hardware output เสมอไป ไม่ได้ปรับลดเสียงรวมอัตโนมัติเพื่อกลบปัญหา และไม่ได้เปลี่ยนลักษณะ limiter เดิม

## ผลทดสอบ

- Core DSP/transport/FX: **78/78 ผ่าน**, 139,371 checks
- สัญญาณ 1 / 2 / 8 / 32 แทร็ก ผสม mono/stereo ที่ 44.1 / 48 / 96 kHz: ตรงกับผลรวมอ้างอิง ค่าคลาดเคลื่อนต่ำกว่า 1e-6 และไม่ clip เมื่อผลรวมต่ำกว่า full scale
- ไฟล์ WAV จริง: mono 48 kHz + stereo 44.1 kHz ที่แปลงเป็น 48 kHz เพิ่มแทร็กขณะเล่น ผ่าน decoder → disk thread → callback → mixer → output protection

| Callback frames | ตัวอย่างที่ตรวจ | คลาดเคลื่อนสูงสุด | Stream underruns | Output overloads |
|---:|---:|---:|---:|---:|
| 64 | 3,840 | 0.000000656 | 0 | 0 |
| 192 | 11,520 | 0.000000679 | 0 | 0 |
| 256 | 15,360 | 0.000000669 | 0 | 0 |
| 1,024 | 61,440 | 0.000000689 | 0 | 0 |
| 5,000 | 75,000 | 0.000000689 | 0 | 0 |

- ThreadSanitizer: full core suite ผ่านในรอบก่อนเพิ่ม metadata snapshot; รอบสุดท้ายเฉพาะ live edit/loop ผ่าน 2/2, 23,298 checks รวมการจำลอง reader ช้า ไม่พบ race report การรัน stress กับโค้ดเดิมครั้งนี้ไม่ทำให้ TSan รายงาน race จึงไม่ถือว่าเป็นการ reproduce race แบบ deterministic
- Console state integration: 4,590 parameters, 0 failures รวม recording/export และ reloading/resampling ที่ 44.1 → 48 → 96 kHz
- Apple staging: ผ่าน 9 checks เมื่อรันแยกนอกข้อจำกัดการเข้าถึงระบบไฟล์ของสภาพแวดล้อมทดสอบ; การรัน CTest แบบจำกัดสิทธิ์ครั้งแรกทำให้ชุดนี้ล้มเหลว 3 checks (ชุด Core และ AudioFilePlayback ผ่าน)
- Android arm64: compile ไฟล์ที่แก้ใน AudioEngine, TimelineStreamer และ TransportBar ผ่าน มี warning API writer เดิมที่ deprecated
- `git diff --check`: ผ่าน

คำสั่งทดสอบที่เพิ่ม/ใช้:

```sh
cmake --build build-mac --target MixPracticeCoreTests UiSnapshotTest -j 4
build-mac/MixPracticeCoreTests
build-mac/UiSnapshotTest --audio-playback-tests
build-mac/UiSnapshotTest --console-state-tests
MPS_TEST_FILTER=transportLive /tmp/mps-core-tsan-final
```

เพิ่ม `AudioFilePlaybackTests` เข้า CTest และเพิ่ม `MPS_TEST_FILTER` ให้เลือกชุดทดสอบสำหรับ sanitizer ได้

## เปรียบเทียบภาระมิกซ์กับจุดอ้างอิงก่อน iOS

วัดบน Mac เดียวกัน, C++ -O2, 48 kHz, block 256, stereo, ค่าเริ่มต้น ไม่มี send/insert ที่เปิดเสียง, 2,000 callbacks หลัง warm-up หน่วยไมโครวินาที ค่าเหล่านี้ไม่ใช่ benchmark บน Android และไม่ครอบคลุม scene ที่เปิด FX หนัก

| แทร็ก | 20 ต.ค. mean / p99 | ปัจจุบัน mean / p99 |
|---:|---:|---:|
| 1 | 70.13 / 80.00 | 59.23 / 68.46 |
| 2 | 85.23 / 94.17 | 60.98 / 70.67 |
| 8 | 176.94 / 193.83 | 71.22 / 82.00 |
| 32 | 542.65 / 582.46 | 110.90 / 121.83 |

ไม่พบหลักฐานว่าเส้นทางมิกซ์พื้นฐานช้าลงในกรณีนี้ ยังสรุปแทนผลบนโทรศัพท์, Bluetooth, compressed files หรือ scene จริงไม่ได้

## สิ่งที่ยังขวางการรับรองก่อนขาย

- ต้อง reproduce และฟังบน Android ที่มีอาการ ด้วยไฟล์/scene เดิม พร้อมดู device dropout, stream underrun และ output overload โดยเฉพาะการเล่นต่อหลัง stop/play
- แก้ compile blocker ของ Android แล้ว โดยย้าย method `activateLifetimePro` ที่ JNI เดิมเรียก ไปส่วน public ของคลาส ไม่เปลี่ยน implementation การให้สิทธิ์
- สร้าง APK ทดสอบ arm64 สำเร็จแล้ว ยังไม่ได้ติดตั้ง/ฟังเสียงบน vivo จริง และไม่ได้เผยแพร่หรือแทนที่ไฟล์ release ที่แจกอยู่

## ติดตามหลังทราบรุ่นมือถือ

- เพิ่มรายงานที่เปิดได้ด้วยการแตะแถว sample rate/buffer ใน Transport เหมาะกับมือถือที่ไม่มี hover tooltip รายงานแสดง audio CPU, device dropouts, stream underruns, WAV read errors, output overload blocks, gain/routing และจำนวนช่องที่ผูก source
- เพิ่มตัวนับ read errors พร้อมทดสอบว่าความผิดพลาดของ reader หนึ่งแทร็กไม่ทำลายข้อมูลอีกแทร็ก
- จำกัดค่ารายงาน read-ahead ให้อยู่ใน 0–100% เมื่อ UI อ่านตัวนับพร้อมกับ producer/consumer
- ชุด Core 78/78 ผ่าน; CTest เฉพาะ Core และ AudioFilePlayback ผ่าน 2/2; console state integration ยังคง 0 failures
- Android `assembleRelease` ผ่าน โดยดึง Billing 7.1.1 ตาม dependency ที่โปรเจกต์ระบุไว้
- ตรวจ APK จริงแล้ว: package `com.mixpractice.studio`, versionCode 3 / versionName 1.0.2, มี arm64-v8a, certificate ตรงกับ APK เดิมใน workspace และพบข้อความ `Audio check 2026-11-01` ใน native library ที่แพ็กอยู่
- Gradle โหมด injected ABI วาง APK ใหม่ใน `build/intermediates/apk/release` ตาม redirect ของ build; ไม่ใช้ APK เก่าจาก `build/outputs/apk/release`
- ไฟล์ส่งมอบ: `dist/audio-check-20261101/MixPracticeStudio-audio-check-arm64.apk`
- SHA-256: `f2d18b0c293acf9f47fd8c81b7b59c221fed52032d754a62f6c1801630846523`

ขั้นตอนบนเครื่องจริง: ติดตั้ง APK ทดสอบ เปิดสอง WAV เดิม หากยังแตก ให้แตะแถว sample rate/buffer และบันทึกภาพ Audio report ขณะเกิดอาการ พร้อมไฟล์ WAV/scene ที่ใช้ เพื่อแยก source/scene distortion, output overload และ device underrun ต่อไป


## ติดตั้งจริงบน vivo X200 FE

- เชื่อมต่อโทรศัพท์รุ่น V2503 สำเร็จหลังเปิด USB debugging ใหม่ อุปกรณ์ใช้ Android 16 และ PAGE_SIZE 4096
- Android ปฏิเสธไฟล์ที่ส่งผ่าน USB ด้วย `INSTALL_PARSE_FAILED_NO_CERTIFICATES` / SHA-256 contents did not verify ตรวจ SHA-256 บนเครื่องพบว่าไม่ตรงต้นฉบับและเปลี่ยนไปในแต่ละการส่ง ทั้ง ADB 37 และ ADB 31, สายต่อตรง, binary/text/chunk transfers บางส่วน ยังไม่ได้ระบุ root cause ระดับ driver/สาย/อุปกรณ์
- หลังต่อ Wi-Fi วงเดียวกัน ส่งผ่าน ADB TCP ชั่วคราวแล้ว checksum ตรงกับต้นฉบับ `f2d18b0c293acf9f47fd8c81b7b59c221fed52032d754a62f6c1801630846523`
- พบข้อจำกัดแพ็กเกจอีกจุด: Gradle injected ABI build ทำให้ APK มี `TEST_ONLY`; `pm install -r` ถูกปฏิเสธด้วย `INSTALL_FAILED_TEST_ONLY` รุ่นนี้จึงไม่ควรแนะนำให้แตะติดตั้งผ่านตัวจัดการไฟล์ตามปกติ
- ติดตั้งผ่าน `pm install -r -t` สำเร็จ โดยคงข้อมูลแอปเดิม เปิด `com.mixpractice.studio/.MainActivity` แล้วกระบวนการยังทำงาน และไม่พบ crash log ของกระบวนการที่เปิดในการตรวจครั้งนี้
- ลบไฟล์ APK/parts ชั่วคราวบนโทรศัพท์ และสลับ adbd กลับ USB (TCP port = 0) แล้ว
- ยังไม่ได้ยืนยันผลการฟังเสียง WAV สองแทร็กบนอุปกรณ์ ต้องให้ผู้ใช้ลองไฟล์เดิมหลังติดตั้ง
