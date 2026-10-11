# Mix Practice Studio - Multiplatform Build Guide (.exe & .apk)

คู่มือนี้แนะนำวิธีการ Build โครงการ **Mix Practice Studio** ออกมาเป็นไฟล์สำหรับระบบปฏิบัติการต่างๆ:
- **Windows:** `Mix Practice Studio.exe`
- **Android:** `MixPracticeStudio.apk`
- **macOS:** `Mix Practice Studio.app`

---

## วิธีที่ 1: Build อัตโนมัติบน Cloud ผ่าน GitHub Actions (แนะนำและง่ายที่สุด)

โครงการนี้มีไฟล์ Workflow อยู่ที่ `.github/workflows/build-all.yml` เรียบร้อยแล้ว

### ขั้นตอน:
1. สร้าง Repository ใหม่บน GitHub (เช่น ชื่อ `MixPracticeStudio`)
2. อัปโหลดโค้ดโครงการนี้ขึ้น GitHub:
   ```bash
   git init
   git add .
   git commit -m "Initial commit for multiplatform mixer"
   git branch -M main
   git remote add origin https://github.com/<YOUR_USERNAME>/<YOUR_REPO>.git
   git push -u origin main
   ```
3. ไปที่แท็บ **Actions** บน GitHub
4. ระบบจะทำการหมุน Server บนคลาวด์เพื่อ Build ทั้ง 3 ระบบปฏิบัติการพร้อมกัน:
   - **Windows:** ใช้เครื่อง Windows Server + Visual Studio 2022 เพื่อคอมไพล์ `.exe`
   - **macOS:** ใช้เครื่อง macOS + Clang เพื่อคอมไพล์ `.app`
   - **Android:** ใช้เครื่อง Ubuntu + Android NDK / Gradle เพื่อคอมไพล์ `.apk`
5. เมื่อ Build เสร็จ จะมีไฟล์ให้ดาวน์โหลดในส่วน **Artifacts** ทันที

---

## วิธีที่ 2: Build บนเครื่องคอมพิวเตอร์ของคุณเอง (Local Machine)

### 1. การสร้างไฟล์ `.exe` (บนคอมพิวเตอร์ Windows)
1. ติดตั้ง **Visual Studio 2022 Community** (เลือกติ๊ก Workload: `Desktop development with C++`)
2. เปิด PowerShell หรือ Command Prompt เข้าไปที่โฟลเดอร์โปรเจกต์
3. รันคำสั่ง CMake:
   ```bash
   cmake -B build-win -G "Visual Studio 17 2022" -A x64
   cmake --build build-win --config Release
   ```
4. จะได้ไฟล์ `.exe` อยู่ที่:
   `build-win/MixPracticeStudio_artefacts/Release/Mix Practice Studio.exe`

---

### 2. การสร้างไฟล์ `.apk` (สำหรับ Android)
1. ติดตั้ง **Android Studio** พร้อม Android SDK และ Android NDK (r26 ขึ้นไป)
2. รันคำสั่ง CMake เพื่อสร้างไฟล์สำหรับสถาปัตยกรรม ARM64:
   ```bash
   cmake -B build-android \
     -DCMAKE_SYSTEM_NAME=Android \
     -DCMAKE_SYSTEM_VERSION=24 \
     -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
     -DCMAKE_ANDROID_NDK=$ANDROID_NDK_HOME \
     -DCMAKE_ANDROID_STL_TYPE=c++_static \
     -DCMAKE_BUILD_TYPE=Release
   cmake --build build-android --config Release
   ```
3. จะได้ไฟล์ `.apk` ออกมาพร้อมติดตั้งบนมือถือหรือแท็บเล็ต Android ครับ
