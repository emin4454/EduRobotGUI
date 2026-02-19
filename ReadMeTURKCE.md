# EduRobotGUI

Bu repo, sesli etkileşimli bir robot akışını içerir. Proje temel olarak:

- STT (speech-to-text) ile kullanıcıyı dinler
- Rasa + LLM ile metin yanıtı üretir
- TTS ile sesi oynatır
- Ekran/animasyon bileşenleriyle durum gösterir
- Veritabanından içerik alır

Eğitsel odağı, Türkçe bilen çocuklara sesli etkileşim ve yönlendirilmiş aktiviteler üzerinden İngilizce öğretmektir.

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
Temel olarak metin tipinde veri alir ve JSON body ile gönderilir.
Intentler tek adimli olabilecegi gibi state machine benzeri cok adimli akislarda da kullanilabilir.
Kod tarafinda su alanlar parse edilir:
- Intent istegi: `text`
- Intent cevabi: `intent.name`, `intent.confidence`
- Webhook istegi: `sender`, `message`
- Webhook cevabi: dizinin ilk elemanindaki `text` (`[0].text`)
- `recipient_id` kodda parse edilmiyor.

Flow notu:
1. Ilk olarak `/model/parse` endpoint'ine `{ "text": "..." }` ile istek atilir.
2. Donen intent `nlu_fallback` degilse ve confidence `>= 0.50` ise, ekstra dogrudan cevap var mi diye `/webhooks/rest/webhook` endpoint'ine ikinci istek atilir.
3. Webhook cevabi bir dizi doner ve ilk elemanda `text` varsa, bu deger Rasa'nin dogrudan cevabi olarak kullanilir.

Ornek intent parse istek/cevap:

Istek:

```json
{
  "text": "Bugun hava nasil?"
}
```

Cevap:

```json
{
  "text": "Bugun hava nasil?",
  "intent": {
    "name": "ask_weather",
    "confidence": 0.98
  },
  "entities": []
}
```

Ornek webhook istek/cevap:

Istek:

```json
{
  "sender": "user",
  "message": "Bugun hava nasil?"
}
```

Cevap:

```json
[
  {
    "text": "Bugun hava ilik ve parcali bulutlu."
  }
]
```

Ek istek/cevap ornekleri:

Istek:

```json
{
  "text": "Bana hayvanlar hakkinda bir oyun ac"
}
```

Cevap:

```json
{
  "text": "Bana hayvanlar hakkinda bir oyun ac",
  "intent": {
    "name": "start_animal_game",
    "confidence": 0.96
  },
  "entities": []
}
```

Istek:

```json
{
  "sender": "user",
  "message": "Ingilizce renkleri ogretir misin?"
}
```

Cevap:

```json
[
  {
    "text": "Tabii. Temel renklerle baslayalim: red, blue ve green."
  }
]
```

Koddaki state machine ile uyumlu cok adimli intent akislari:

1. Gorsel kart oyunu akisi (`visual_card_game_start` -> `VISUAL_GAME_START`)

```text
Kullanici: "Ekranda ne var oyunu oynayalim"
Rasa intent: visual_card_game_start
State gecisi: LISTENING -> VISUAL_GAME_START
Robot: Rastgele 5 gorsel gosterir ve her tur sesli cevap bekler
Kullanici (oyun icinde): "dur"
Rasa intent: stop
State gecisi: VISUAL_GAME_START -> SLEEPING
```

2. Kelime ceviri oyunu akisi (`start_translation_game` -> `TRANSLATION_GAME_START`)

```text
Kullanici: "Ceviri oyunu baslatalim"
Rasa intent: start_translation_game
State gecisi: LISTENING -> TRANSLATION_GAME_START
Robot: 5 kelime sorar, her tur cevap alir
Kullanici (oyun icinde): "stop"
Rasa intent: stop
State gecisi: TRANSLATION_GAME_START -> LISTENING
```

3. Onceki cevabi aciklatma akisi (`dont_understand_word` / `i_dont_know_any_word`)

```text
Adim 1: Robot bir cevap uretir ve bunu `last_response` olarak saklar
Adim 2: Kullanici "anlamadim" benzeri bir ifade soyler
Rasa intent: dont_understand_word (veya i_dont_know_any_word)
Adim 3: Robot `last_response` metnini Turkceye cevirip tekrar seslendirir
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
