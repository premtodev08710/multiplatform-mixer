# Privacy Policy & Play Console Data Safety Guide
**Application**: Mix Practice Studio (com.mixpractice.studio)  
**Last Updated**: October 2026

---

## 1. นโยบายความเป็นส่วนตัว (Privacy Policy)

### ภาพรวม (Overview)
Mix Practice Studio เคารพความเป็นส่วนตัวของผู้ใช้งาน นโยบายนี้อธิบายเกี่ยวกับการประมวลผลข้อมูลเมื่อใช้งานแอปพลิเคชัน

### ข้อมูลที่แอปพลิเคชันเข้าถึงและประมวลผล (Data Collection & Usage)
1. **ไฟล์เสียงและการจัดเก็บ (Audio Files & Storage)**
   - สิทธิ์: `READ_MEDIA_AUDIO` / `READ_EXTERNAL_STORAGE`
   - วัตถุประสงค์: เพื่อเปิด นำเข้า และเล่นไฟล์เสียง Multitrack (WAV, AIFF, MP3, FLAC) ที่ผู้ใช้เลือกเข้ามาฝึกมิกซ์เสียง
   - การส่งออก: แอปประมวลผลไฟล์เสียงบนตัวเครื่อง (On-device Processing) เท่านั้น **ไม่มีการอัปโหลดไฟล์เสียงของผู้ใช้ขึ้นเซิร์ฟเวอร์ใดๆ ทั้งสิ้น**

2. **การวิเคราะห์การติดตั้งและการใช้งานพื้นฐาน (App Telemetry & Analytics)**
   - บริการ: Google Firebase Firestore
   - ข้อมูลที่บันทึก: Install ID (UUID สุ่มขึ้นมาประจำเครื่อง), ยี่ห้อ/รุ่นอุปกรณ์, สถาปัตยกรรม CPU, จำนวนครั้งที่เปิดใช้งาน, เวอร์ชันแอปพลิเคชัน
   - วัตถุประสงค์: เพื่อวิเคราะห์ความเข้ากันได้ของอุปกรณ์ ปรับปรุงเสถียรภาพ และตรวจนับจำนวนการติดตั้ง

3. **โฆษณา (Advertisements)**
   - บริการ: Google AdMob SDK
   - ข้อมูลที่ประมวลผล: Advertising ID (AD_ID) เพื่อแสดงโฆษณาคั่นหน้า (Interstitial Ad) แบบข้ามได้หลัง 5 วินาที สำหรับผู้ใช้ Free Edition
   - ผู้ใช้สามารถอัปเกรดเป็น **Pro Lifetime** เพื่อปิดการแสดงโฆษณาทั้งหมดอย่างถาวร

4. **การชำระเงินและระบบสมาชิก (In-App Purchases & Billing)**
   - บริการ: Google Play In-app Billing API (com.android.billingclient)
   - ข้อมูล: Purchase Token และ Order ID เพื่อตรวจสอบและปลดล็อกสิทธิ์การใช้งาน Pro Lifetime
   - ข้อมูลบัตรเครดิตหรือข้อมูลการเงินทั้งหมดถูกจัดการโดย Google Play อย่างปลอดภัย โดยที่ผู้พัฒนาไม่สามารถเข้าถึงข้อมูลการชำระเงินโดยตรงได้

5. **ติดต่อเรา (Contact Information)**
   - หากมีข้อสงสัยเกี่ยวกับนโยบายความเป็นส่วนตัว สามารถติดต่อทีมงานได้ที่ support หรือช่องทางทางการของ Mix Practice Studio

### ลิงก์ Privacy Policy URL สำหรับกรอกใน Play Console
สามารถเปิดใช้งาน **GitHub Pages** (Settings > Pages > Source: `Deploy from a branch` > Branch: `main` > Folder: `/docs`) แล้วใช้ URL นี้ได้ทันที:
- **`https://premtodev08710.github.io/multiplatform_mixer/privacy-policy.html`**
(หรือ `https://premtodev08710.github.io/multiplatform_mixer/`)

---

## 2. คู่มือการกรอก Google Play Console (Data Safety Form)

เมื่อส่งแอปเข้า Google Play Console ให้กรอกในส่วน **App Content > Data Safety** ดังนี้:

### Does your app collect or share any of the required user data types?
- **Yes**

### Is all of the user data collected by your app encrypted in transit?
- **Yes** (ใช้ HTTPS / SSL ในการเชื่อมต่อทั้งหมด)

### Data Types:

| Data Type | Collected? | Shared? | Purpose | Ephemeral? |
| :--- | :--- | :--- | :--- | :--- |
| **Device or other IDs** (Advertising ID, Install UUID) | Yes | Yes (กับ AdMob) | App functionality, Analytics, Advertising | No |
| **Financial Info** (Purchase history) | Yes | No | Account management, App functionality (In-app purchase) | No |
| **Audio files** | **No** (ประมวลผลบนเครื่องเท่านั้น Not transmitted) | No | N/A | N/A |

---

## 3. การตั้งค่าสินค้าใน Google Play Console (In-App Product)

1. ไปที่ **Google Play Console** > เลือกแอป `Mix Practice Studio`
2. เมนูด้านซ้าย: **Monetize with Play** > **In-app products**
3. คลิกปุ่ม **Create product**:
   - **Product ID**: `mps_pro_lifetime`
   - **Name**: `Mix Practice Studio Pro (Lifetime)`
   - **Description**: `ปลดล็อกฟังก์ชัน Pro 32 แทร็ก, 8 FX Rack และปิดโฆษณาถาวร ซื้อครั้งเดียวใช้งานได้ตลอดชีพ`
   - **Status**: Active
   - **Default Price**: 299.00 THB (หรือกำหนดราคาตามค่าเงินแต่ละประเทศ)
4. กดบันทึก (**Save**) และเปิดใช้งาน (**Activate**)
