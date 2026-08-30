# Caregiver App

Companion app for the obstacle-detection cane project. Caregiver signs up, fills
in their own details plus the blind person's profile, then sees live alerts and
last-known location on a map.

## 1. Firebase project setup

1. Go to https://console.firebase.google.com, create a new project.
2. Add an app (Web app is fine even for Expo — "</>" icon) and copy the config
   values into `firebaseConfig.js`.
3. Enable **Authentication > Sign-in method > Email/Password**.
4. Enable **Firestore Database** (start in test mode for development, then
   lock down with security rules before going live — see below).


## 2. Install and run

```
npm install
npx expo start
```

Scan the QR code with Expo Go on your Android phone, or run `npm run android`
with an emulator/device connected.

## 3. Firestore data model

```
caregivers/{uid}        - name, email, phone, pushToken
patients/{uid}           - keyed by the caregiver's uid (1 caregiver : 1 patient for MVP)
                            name, age, medicalNotes, emergencyContact, caneId
alerts/{autoId}          - patientId (= caregiver uid), message, location,
                            timestamp (ms since epoch)
```

## 4. Making the cane/phone write alerts here instead of (or alongside) Telegram

Instead of calling the Telegram API, the ESP32 or the phone SOS button can write
directly to Firestore's REST endpoint:

```
POST https://firestore.googleapis.com/v1/projects/YOUR_PROJECT_ID/databases/(default)/documents/alerts
Content-Type: application/json

{
  "fields": {
    "patientId": { "stringValue": "THE_CAREGIVER_UID" },
    "message": { "stringValue": "DANGER! Obstacle very close!" },
    "location": { ["doubleValue": 12.9716 , "doubleValue": 77.5946] },
    "timestamp": { "integerValue": "1234567890000" }
  }
}
```

This needs either an API key with Firestore REST access enabled, or (more
securely) routing the write through a small Cloud Function/HTTP endpoint so
the cane never holds a credential with broad database access. Happy to wire
this into your existing ESP32 sketch if you want — just say the word.

