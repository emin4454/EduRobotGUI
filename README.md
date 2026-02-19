# EduRobotGUI

Bu repo, sesli etkileşimli bir robot akışını içerir. Proje temel olarak:

- STT (speech-to-text) ile kullanıcıyı dinler
- Rasa + LLM ile metin yanıtı üretir
- TTS ile sesi oynatır
- Ekran/animasyon bileşenleriyle durum gösterir
- Veritabanından içerik alır

## 1) Gereksinimler

### Ubuntu

```bash
chmod +x ubuntu-ideps.sh
./ubuntu-ideps.sh
```

### Arch

```bash
chmod +x arch-ideps.sh
./arch-ideps.sh
```

Not:
Rasa bu projede private bir intent tanilama (intent classification) servisi olarak kullanilir.
Temel olarak metin tipinde veri alir ve JSON body ile beslenir.

Ornek intent parse istegi:

```json
{
  "text": "Bugun hava nasil?"
}
```

Ornek webhook istegi:

```json
{
  "sender": "user",
  "message": "Bugun hava nasil?"
}
```

Ek ornekler:

```json
{
  "text": "Bana hayvanlar hakkinda bir oyun ac"
}
```

```json
{
  "sender": "user",
  "message": "Ingilizce renkleri ogretir misin?"
}
```

```json
{
  "text": "Muzik ac ve sesi biraz azalt"
}
```

## 2) Ortam Degiskenleri (.env)

`.env` icinde su alanlari doldur:

```env
DB_HOST=
DB_PORT=3306
DB_USER=
DB_PASS=
DB_NAME=

GEMINI_API_KEY=
GEMINI_MODEL=gemini-2.0-flash

GOOGLE_TRANSLATE_API_KEY=

ELEVENLABS_API_KEY=
ELEVENLABS_VOICE_ID=ZgndNiUCwSn9oAFrs6rJ
ELEVENLABS_MODEL_ID=eleven_multilingual_v2

SONIOX_API_KEY=

RASA_BASE_URL=
```

- API key bos ise ilgili servis calismaz

## 3) Build ve Calistirma

```bash
mkdir -p build
cd build
cmake ..
make -j
# alternatif:
# cmake --build . -j
./EduRobot
```

## 4) Mimari Ozet

- `src/audio/stt/`: ses kaydi ve websocket STT akisi
- `src/websocket/`: Soniox websocket istemcisi
- `src/rasa/`: intent + webhook cagrilari
- `src/chatbot/`: Gemini uzerinden LLM yanitlari
- `src/audio/tts/`: ElevenLabs TTS
- `src/database/`: MySQL/MariaDB erisimi
- `src/gui/`: ekran ve animasyon yonetimi
- `src/helpers/`: ceviri, env yardimcilari, metin yardimcilari

## 5) Hata Ayiklama

- Veritabani baglanamiyorsa:
  - `DB_HOST`, `DB_PORT`, `DB_USER`, `DB_PASS`, `DB_NAME` degerlerini kontrol et.
- Rasa donmuyorsa:
  - `RASA_BASE_URL` ve Rasa servisinin ayakta oldugunu kontrol et.
- STT baslamiyorsa:
  - `SONIOX_API_KEY` degerini kontrol et.
- TTS calismiyorsa:
  - `ELEVENLABS_API_KEY` ve ses/model alanlarini kontrol et.
- Build hatasi varsa:
  - sistem bagimlilik scriptlerini tekrar calistir.

## 6) Proje Nasil Calisir

Calisma akisi ozetle su sekilde:

1. Program baslarken `.env` dosyasi yuklenir ve servis ayarlari okunur.
2. Robot ses dinleme moduna gecer, mikrofon akisi STT tarafina gonderilir.
3. STT sonucu metin olarak alinir ve kullanicinin ifadesi elde edilir.
4. Metin once Rasa tarafinda intent analizi icin islenir.
5. Rasa sonucu yetersizse/uygun degilse LLM (Gemini) ile yanit uretilir.
6. Uretilen veya bulunan yanit TTS (ElevenLabs) ile sese cevrilir.
7. Ses oynatilirken GUI durumu/animasyonlar guncellenir.
8. Gerekli durumlarda DB'den soru-cevap veya gorsel icerik okunur.
