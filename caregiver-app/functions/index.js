// Deploy with: firebase deploy --only functions
// Sends a push notification to the caregiver the instant a new alert document
// is created in Firestore (works even when the caregiver app is closed).
//
// Requires: caregiver's Expo/FCM push token saved on their profile doc
// (caregivers/{uid}.pushToken) - captured client-side with expo-notifications.

const functions = require("firebase-functions");
const admin = require("firebase-admin");
admin.initializeApp();

exports.onNewAlert = functions.firestore
  .document("alerts/{alertId}")
  .onCreate(async (snap) => {
    const alert = snap.data();
    const patientId = alert.patientId;

    const caregiverDoc = await admin.firestore().collection("caregivers").doc(patientId).get();
    if (!caregiverDoc.exists) return null;

    const pushToken = caregiverDoc.data().pushToken;
    if (!pushToken) return null;

    const message = {
      token: pushToken,
      notification: {
        title: "Emergency alert",
        body: alert.message || "New alert received"
      },
      data: {
        latitude: String(alert.latitude || ""),
        longitude: String(alert.longitude || "")
      }
    };

    return admin.messaging().send(message);
  });
