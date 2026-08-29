# Smart Cane + Caregiver App

An obstacle-detection smart cane for visually impaired users, paired with a
mobile app that lets a caregiver keep an eye on them remotely. The cane
handles everyday obstacle alerts on its own (buzzer + smartwatch vibration
via Telegram) and escalates straight to the caregiver's phone only when the
user presses an emergency button.

For full setup instructions, see **[docs/getting-started.md](docs/getting-started.md)**.

---

## How it all fits together

The system has three parts that talk to each other:

1. **The cane (ESP32 + ultrasonic sensor)** — worn/carried by the visually
   impaired person. Constantly measures distance to nearby obstacles and
   has a physical emergency button.
2. **Telegram** — acts as the messaging relay between the cane and the
   visually impaired person's own smartwatch/phone. The cane talks to a
   Telegram bot; Telegram pushes that to the paired device, which triggers
   a vibration.
3. **The caregiver app (React Native / Expo + Firebase)** — installed on
   the caregiver's phone. Signs up, stores the patient's profile, and shows
   live emergency alerts + last-known location on a map. It does **not**
   see routine obstacle pings — only emergencies.

### Data flow

**Routine obstacle detected:**
```
Ultrasonic sensor → ESP32 → buzzer (local)
                          → Telegram Bot API → visually impaired person's phone
                                              → smartwatch vibration
```
Caregiver is not involved in this path — obstacle alerts are frequent and
usually minor, so they stay between the cane and the person carrying it.

**Emergency button pressed:**
```
Emergency button held (~1s) → ESP32 → 3 confirmation beeps (local)
                                    → Telegram Bot API → visually impaired person's phone
                                    → Firestore "alerts" collection → caregiver app (real-time)
```
This is the one path that reaches the caregiver directly and instantly,
via a live Firestore listener in the app's Alerts screen.

### Why two different notification paths?

Obstacle detection happens constantly while walking — sending every one of
those to a caregiver would be noisy and would drown out the alerts that
actually matter. The emergency button is the deliberate, low-frequency
signal reserved for situations that need a caregiver's attention right away.

---

## Repo structure

```
smart-cane-project/
├── README.md                      ← you are here
├── caregiver-app/                 ← Expo React Native app
│   ├── App.js
│   ├── screens/
│   │   ├── LoginScreen.js
│   │   ├── SignupScreen.js
│   │   ├── ProfileSetupScreen.js
│   │   ├── AlertsScreen.js
│   │   └── MapScreen.js
│   ├── functions/                 ← Firebase Cloud Function (push notifications)
│   ├── package.json
│   ├── app.json
│   ├── firebaseConfig.example.js  ← copy to firebaseConfig.js and fill in
│   └── .gitignore
├── cane-firmware/                 ← ESP32 code
│   ├── cane.ino
│   ├── secrets.example.h          ← copy to secrets.h and fill in
│   └── README.md                  ← wiring + firmware-specific notes
└── docs/
    └── getting-started.md         ← end-to-end setup guide for a new user
    └── Working model.jpeg        ← a image of my working model
```

---

## Tech stack

| Layer | Tech |
|---|---|
| Cane firmware | ESP32, ultrasonic (HC-SR04 or similar), Arduino framework |
| Messaging relay | Telegram Bot API |
| Caregiver app | React Native (Expo), React Navigation |
| Backend | Firebase Authentication, Firestore, Cloud Functions (FCM push) |
| Maps | react-native-maps + Google Maps SDK |

---

## Firestore data model

```
caregivers/{uid}     - name, email, phone, pushToken
patients/{uid}        - keyed by the caregiver's uid (1 caregiver : 1 patient for MVP)
                         name, age, medicalNotes, emergencyContact, caneId
alerts/{autoId}       - patientId (= caregiver uid), message, latitude, longitude,
                         timestamp (ms since epoch), status
```

---

## Quick start for developers

Full step-by-step is in [docs/getting-started.md](docs/getting-started.md)
(written for end users setting up their own cane) — the summary for
developers working on the code itself:

```bash
# Caregiver app
cd caregiver-app
cp firebaseConfig.example.js firebaseConfig.js   # fill in your Firebase project values
npm install
npx expo start
```



---

## Security notes before going live

- **Never commit real credentials.** `firebaseConfig.js`, `secrets.h`, and
  any `.env` files are gitignored — only the `.example` versions with
  placeholder values are checked in.
- **Firestore starts in test mode**, which allows open read/write. Lock it
  down before real users touch it — see the sample security rules in
  `caregiver-app/README.md`.
- The cane's Telegram Bot Token and Chat ID are effectively credentials —
  anyone with them can message that bot/chat. Treat them like a password.

---
