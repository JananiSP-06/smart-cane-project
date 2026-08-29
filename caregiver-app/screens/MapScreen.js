import React, { useEffect, useState } from "react";
import { View, StyleSheet, Text } from "react-native";
import { WebView } from "react-native-webview";
import { collection, query, where, orderBy, limit, onSnapshot } from "firebase/firestore";
import { auth, db } from "../firebaseConfig";

function buildMapHtml(latitude, longitude) {
  return `
    <!DOCTYPE html>
    <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
        <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
        <style>
          html, body, #map { height: 100%; margin: 0; padding: 0; }
        </style>
      </head>
      <body>
        <div id="map"></div>
        <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
        <script>
          var map = L.map('map').setView([${latitude}, ${longitude}], 16);
          L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            maxZoom: 19,
            attribution: '&copy; OpenStreetMap contributors'
          }).addTo(map);
          L.marker([${latitude}, ${longitude}])
            .addTo(map)
            .bindPopup('Last known location')
            .openPopup();
        </script>
      </body>
    </html>
  `;
}

export default function MapScreen({ route }) {
  const [location, setLocation] = useState(
    route?.params?.latitude
      ? { latitude: route.params.latitude, longitude: route.params.longitude }
      : null
  );

  // The Map tab stays mounted across navigations (it's a persistent tab screen,
  // not a fresh screen each time), so the useState initial value above only
  // fires once. This effect re-syncs `location` whenever new params arrive,
  // e.g. tapping "View on map" on a different alert.
  useEffect(() => {
    if (route?.params?.latitude) {
      setLocation({ latitude: route.params.latitude, longitude: route.params.longitude });
    }
  }, [route?.params?.latitude, route?.params?.longitude]);

  useEffect(() => {
    // If opened via a specific alert's coordinates, skip the live "latest alert" listener
    if (route?.params?.latitude) return;

    const uid = auth.currentUser.uid;
    const q = query(
      collection(db, "alerts"),
      where("patientId", "==", uid),
      orderBy("timestamp", "desc"),
      limit(1)
    );

    const unsubscribe = onSnapshot(q, (snapshot) => {
      if (!snapshot.empty) {
        const latest = snapshot.docs[0].data();
        if (latest.location) {
          setLocation({ latitude: latest.location.latitude, longitude: latest.location.longitude });
        }
      }
    });

    return unsubscribe;
  }, []);

  if (!location) {
    return (
      <View style={styles.centered}>
        <Text style={styles.emptyText}>No location received yet</Text>
      </View>
    );
  }

  return (
    <WebView
      style={styles.map}
      originWhitelist={["*"]}
      source={{ html: buildMapHtml(location.latitude, location.longitude) }}
    />
  );
}

const styles = StyleSheet.create({
  map: { flex: 1 },
  centered: { flex: 1, justifyContent: "center", alignItems: "center" },
  emptyText: { color: "#888", fontSize: 16 }
});