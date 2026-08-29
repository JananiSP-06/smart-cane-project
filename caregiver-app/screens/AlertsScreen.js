import React, { useEffect, useState } from "react";
import { View, Text, FlatList, StyleSheet, ActivityIndicator, TouchableOpacity, Modal } from "react-native";
import { Ionicons } from "@expo/vector-icons";
import * as Clipboard from "expo-clipboard";
import { collection, query, where, orderBy, onSnapshot } from "firebase/firestore";
import { auth, db } from "../firebaseConfig";

export default function AlertsScreen({ navigation }) {
  const [alerts, setAlerts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [idModalVisible, setIdModalVisible] = useState(false);
  const [copied, setCopied] = useState(false);

  const patientId = auth.currentUser.uid;

  useEffect(() => {
    const uid = auth.currentUser.uid;
    const q = query(
      collection(db, "alerts"),
      where("patientId", "==", uid),
      orderBy("timestamp", "desc")
    );

    // Realtime listener: updates instantly when the cane/phone writes a new alert
    const unsubscribe = onSnapshot(q, (snapshot) => {
      const list = snapshot.docs.map((d) => ({ id: d.id, ...d.data() }));
      setAlerts(list);
      setLoading(false);
    });

    return unsubscribe;
  }, []);

  const handleCopy = async () => {
    await Clipboard.setStringAsync(patientId);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const IdModal = () => (
    <Modal visible={idModalVisible} animationType="slide" transparent onRequestClose={() => setIdModalVisible(false)}>
      <View style={styles.modalOverlay}>
        <View style={styles.modalCard}>
          <View style={styles.modalIconWrap}>
            <Ionicons name="key-outline" size={32} color="#D32F2F" />
          </View>
          <Text style={styles.modalTitle}>Your Patient ID</Text>
          <Text style={styles.modalSubtitle}>
            Enter this exact code as the "Patient ID" field when setting up the cane's WiFi.
          </Text>

          <View style={styles.idBox}>
            <Text selectable style={styles.idText}>{patientId}</Text>
          </View>

          <TouchableOpacity style={styles.copyButton} onPress={handleCopy} accessibilityRole="button">
            <Ionicons name={copied ? "checkmark" : "copy-outline"} size={18} color="#fff" style={{ marginRight: 8 }} />
            <Text style={styles.copyButtonText}>{copied ? "Copied!" : "Copy to clipboard"}</Text>
          </TouchableOpacity>

          <TouchableOpacity onPress={() => setIdModalVisible(false)} accessibilityRole="button">
            <Text style={styles.closeLink}>Close</Text>
          </TouchableOpacity>
        </View>
      </View>
    </Modal>
  );

  if (loading) {
    return (
      <View style={styles.centered}>
        <ActivityIndicator size="large" color="#D32F2F" />
      </View>
    );
  }

  return (
    <View style={{ flex: 1 }}>
      <TouchableOpacity
        style={styles.idBanner}
        onPress={() => setIdModalVisible(true)}
        accessibilityRole="button"
        accessibilityLabel="View your Patient ID for cane setup"
      >
        <Ionicons name="key-outline" size={16} color="#1565C0" />
        <Text style={styles.idBannerText}>View Patient ID for cane setup</Text>
        <Ionicons name="chevron-forward" size={16} color="#1565C0" />
      </TouchableOpacity>

      {alerts.length === 0 ? (
        <View style={styles.centered}>
          <Ionicons name="notifications-off-outline" size={48} color="#ccc" />
          <Text style={styles.emptyText}>No alerts yet</Text>
          <Text style={styles.emptySubtext}>You'll see cane alerts here as they come in</Text>
        </View>
      ) : (
        <FlatList
          data={alerts}
          keyExtractor={(item) => item.id}
          contentContainerStyle={{ padding: 16 }}
          renderItem={({ item }) => (
            <View style={styles.card}>
              <View style={styles.cardHeader}>
                <Ionicons name="warning" size={18} color="#D32F2F" style={{ marginRight: 6 }} />
                <Text style={styles.cardTitle}>{item.message || "Emergency alert"}</Text>
              </View>
              <Text style={styles.cardTime}>
                {item.timestamp ? new Date(item.timestamp).toLocaleString() : ""}
              </Text>
              {item.location ? (
                <Text
                  style={styles.mapLink}
                  onPress={() =>
                    navigation.navigate("Map", {
                      latitude: item.location.latitude,
                      longitude: item.location.longitude
                    })
                  }
                  accessibilityRole="button"
                  accessibilityLabel="View this alert on the map"
                >
                  View on map <Ionicons name="arrow-forward" size={13} />
                </Text>
              ) : null}
            </View>
          )}
        />
      )}

      <IdModal />
    </View>
  );
}

const styles = StyleSheet.create({
  centered: { flex: 1, justifyContent: "center", alignItems: "center", padding: 24 },
  emptyText: { color: "#888", fontSize: 16, fontWeight: "600", marginTop: 12 },
  emptySubtext: { color: "#aaa", fontSize: 13, marginTop: 4, textAlign: "center" },
  idBanner: {
    flexDirection: "row",
    alignItems: "center",
    backgroundColor: "#E8F1FC",
    paddingVertical: 10,
    paddingHorizontal: 16,
    gap: 6
  },
  idBannerText: { flex: 1, color: "#1565C0", fontSize: 13, fontWeight: "600" },
  card: {
    backgroundColor: "#FFF3F3",
    borderRadius: 10,
    padding: 16,
    marginBottom: 12,
    borderLeftWidth: 4,
    borderLeftColor: "#D32F2F"
  },
  cardHeader: { flexDirection: "row", alignItems: "center" },
  cardTitle: { fontSize: 16, fontWeight: "700", color: "#B71C1C" },
  cardTime: { fontSize: 13, color: "#777", marginTop: 4, marginLeft: 24 },
  mapLink: { fontSize: 14, color: "#1565C0", marginTop: 10, marginLeft: 24, fontWeight: "600" },
  modalOverlay: { flex: 1, backgroundColor: "rgba(0,0,0,0.5)", justifyContent: "center", padding: 24 },
  modalCard: { backgroundColor: "#fff", borderRadius: 16, padding: 24, alignItems: "center" },
  modalIconWrap: {
    width: 60,
    height: 60,
    borderRadius: 30,
    backgroundColor: "#FDEDED",
    justifyContent: "center",
    alignItems: "center",
    marginBottom: 14
  },
  modalTitle: { fontSize: 20, fontWeight: "700" },
  modalSubtitle: { fontSize: 13, color: "#777", textAlign: "center", marginTop: 8, marginBottom: 18 },
  idBox: {
    backgroundColor: "#f5f5f5",
    borderRadius: 8,
    paddingVertical: 14,
    paddingHorizontal: 12,
    width: "100%",
    marginBottom: 16
  },
  idText: { fontSize: 13, fontFamily: "monospace", textAlign: "center", color: "#333" },
  copyButton: {
    flexDirection: "row",
    backgroundColor: "#D32F2F",
    borderRadius: 10,
    paddingVertical: 12,
    paddingHorizontal: 24,
    alignItems: "center",
    marginBottom: 14
  },
  copyButtonText: { color: "#fff", fontWeight: "700", fontSize: 15 },
  closeLink: { color: "#888", fontSize: 14 }
});