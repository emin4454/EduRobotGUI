# EduRobotGUI

This repository contains a voice-interactive robot workflow. The project mainly:

- Listens to the user with STT (speech-to-text)
- Produces text responses with Rasa + LLM
- Plays audio with TTS
- Displays state through screen/animation components
- Retrieves content from a database

Its educational focus is to help Turkish-speaking children learn English through voice-based interaction and guided activities.

## 1) Requirements

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

Note:
In this project, Rasa is used as a private intent classification service.
It basically receives text data and is sent with a JSON body.
Intents can be single-step, or used in multi-step flows similar to a state machine.
The code currently parses only these fields:
- Intent request: `text`
- Intent response: `intent.name`, `intent.confidence`
- Webhook request: `sender`, `message`
- Webhook response: first available string `text` in response items
- `recipient_id` is not parsed in the code.

Important:
- `/model/parse` response shape can vary by Rasa version/pipeline.
- In this codebase, only `intent.name` and `intent.confidence` are required.
- Root-level `text` in parse response is optional for this project.

Flow note:
1. First, a request is sent to `/model/parse` with `{ "text": "..." }`.
2. If the returned intent is not `nlu_fallback` and confidence is `>= 0.50`, a second request is sent to `/webhooks/rest/webhook` to check whether there is an extra direct answer.
3. If webhook response contains a string `text` in any item, that value is used as Rasa's direct response.

Example intent parse request/response:

Request:

```json
{
  "text": "How is the weather today?"
}
```

Response:

```json
{
  "intent": {
    "name": "ask_weather",
    "confidence": 0.98
  },
  "entities": []
}
```

Example webhook request/response:

Request:

```json
{
  "sender": "user",
  "message": "How is the weather today?"
}
```

Response:

```json
[
  {
    "text": "Today's weather is mild and partly cloudy."
  }
]
```

Additional request/response examples:

Request:

```json
{
  "text": "Open a game about animals"
}
```

Response:

```json
{
  "intent": {
    "name": "start_animal_game",
    "confidence": 0.96
  },
  "entities": []
}
```

Request:

```json
{
  "sender": "user",
  "message": "Can you teach me colors in English?"
}
```

Response:

```json
[
  {
    "text": "Of course. Let's start with basic colors: red, blue, and green."
  }
]
```

Multi-step intent flows compatible with the state machine in the code:

1. Visual card game flow (`visual_card_game_start` -> `VISUAL_GAME_START`)

```text
User: "Let's play the what's on screen game"
Rasa intent: visual_card_game_start
State transition: LISTENING -> VISUAL_GAME_START
Robot: Shows 5 random visuals and waits for a spoken answer each round
User (during the game): "stop"
Rasa intent: stop
State transition: VISUAL_GAME_START -> SLEEPING
```

2. Word translation game flow (`start_translation_game` -> `TRANSLATION_GAME_START`)

```text
User: "Let's start the translation game"
Rasa intent: start_translation_game
State transition: LISTENING -> TRANSLATION_GAME_START
Robot: Asks 5 words and receives an answer each round
User (during the game): "stop"
Rasa intent: stop
State transition: TRANSLATION_GAME_START -> LISTENING
```

3. Previous answer explanation flow (`dont_understand_word` / `i_dont_know_any_word`)

```text
Step 1: The robot generates an answer and stores it as `last_response`
Step 2: The user says an expression similar to "I don't understand"
Rasa intent: dont_understand_word (or i_dont_know_any_word)
Step 3: The robot translates `last_response` to Turkish and speaks it again
```

## 2) Environment Variables (.env)

Fill the following fields in `.env`:

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

- If an API key is empty, the related service will not run

## 3) Build and Run

```bash
mkdir -p build
cd build
cmake ..
make -j
# alternative:
# cmake --build . -j
./EduRobot
```

## 4) Architecture Summary

- `src/audio/stt/`: audio capture and websocket STT flow
- `src/websocket/`: Soniox websocket client
- `src/rasa/`: intent + webhook calls
- `src/chatbot/`: LLM responses via Gemini
- `src/audio/tts/`: ElevenLabs TTS
- `src/database/`: MySQL/MariaDB access
- `src/gui/`: screen and animation management
- `src/helpers/`: translation, env helpers, text helpers

## 5) Troubleshooting

- If the database cannot connect:
  - Check `DB_HOST`, `DB_PORT`, `DB_USER`, `DB_PASS`, `DB_NAME` values.
- If Rasa does not respond:
  - Check `RASA_BASE_URL` and that the Rasa service is running.
- If STT does not start:
  - Check the `SONIOX_API_KEY` value.
- If TTS does not work:
  - Check `ELEVENLABS_API_KEY` and voice/model fields.
- If there is a build error:
  - Run the system dependency scripts again.

## 6) How the Project Works

The workflow is summarized as follows:

1. On startup, the `.env` file is loaded and service settings are read.
2. The robot enters listening mode, and the microphone stream is sent to STT.
3. STT output is received as text, and the user's utterance is obtained.
4. The text is first processed by Rasa for intent analysis.
5. If the Rasa result is insufficient/not suitable, a response is generated with the LLM (Gemini).
6. The generated or found response is converted to speech with TTS (ElevenLabs).
7. While audio is playing, GUI states/animations are updated.
8. When needed, question-answer or visual content is read from the database.
