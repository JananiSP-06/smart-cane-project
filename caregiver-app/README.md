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

## 2. Google Maps API key

1. In Google Cloud Console (same project Firebase created, or a new one),
   enable the "Maps SDK for Android" (and "Maps SDK for iOS" if needed).
2. Create an API key, restrict it to those APIs.
3. Paste it into `app.json` in both the `android.config.googleMaps.apiKey`
   and `ios.config.googleMapsApiKey` fields.

## 3. Install and run

```
npm install
npx expo start
```

Scan the QR code with Expo Go on your Android phone, or run `npm run android`
with an emulator/device connected.

## 4. Firestore data model

```
caregivers/{uid}        - name, email, phone, pushToken
patients/{uid}           - keyed by the caregiver's uid (1 caregiver : 1 patient for MVP)
                            name, age, medicalNotes, emergencyContact, caneId
alerts/{autoId}          - patientId (= caregiver uid), message, latitude, longitude,
                            timestamp (ms since epoch), status
```

## 5. Making the cane/phone write alerts here instead of (or alongside) Telegram

Instead of calling the Telegram API, the ESP32 or the phone SOS button can write
directly to Firestore's REST endpoint:

```
POST https://firestore.googleapis.com/v1/projects/YOUR_PROJECT_ID/databases/(default)/documents/alerts
Content-Type: application/json

{
  "fields": {
    "patientId": { "stringValue": "THE_CAREGIVER_UID" },
    "message": { "stringValue": "DANGER! Obstacle very close!" },
    "latitude": { "doubleValue": 12.9716 },
    "longitude": { "doubleValue": 77.5946 },
    "timestamp": { "integerValue": "1234567890000" },
    "status": { "stringValue": "new" }
  }
}
```

This needs either an API key with Firestore REST access enabled, or (more
securely) routing the write through a small Cloud Function/HTTP endpoint so
the cane never holds a credential with broad database access. Happy to wire
this into your existing ESP32 sketch if you want — just say the word.

## 6. Push notifications when the app is closed

`functions/index.js` has a Cloud Function that fires whenever a new alert
document is created and sends a push notification via Firebase Cloud
Messaging. To use it:

1. `npm install -g firebase-tools && firebase login`
2. `firebase init functions` in this folder (link to your project)
3. `firebase deploy --only functions`
4. On the client, capture an Expo/FCM push token with `expo-notifications`
   and save it to `caregivers/{uid}.pushToken` after login.

## 7. Before going live: security rules

Test-mode Firestore rules allow anyone to read/write everything. Before
sharing this with real users, lock it down, e.g.:

```
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /caregivers/{uid} {
      allow read, write: if request.auth.uid == uid;
    }
    match /patients/{uid} {
      allow read, write: if request.auth.uid == uid;
    }
    match /alerts/{alertId} {
      allow read: if request.auth.uid == resource.data.patientId;
      allow create: if true; // tighten once the cane writes through a trusted endpoint
    }
  }
}
```
