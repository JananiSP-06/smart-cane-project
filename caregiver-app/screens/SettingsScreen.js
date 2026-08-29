import React, { useEffect, useState } from "react";
import {
  View,
  Text,
  ScrollView,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ActivityIndicator,
  Alert,
  Modal,
  KeyboardAvoidingView,
  Platform
} from "react-native";
import { Ionicons } from "@expo/vector-icons";
import * as Clipboard from "expo-clipboard";
import { signOut } from "firebase/auth";
import { doc, getDoc, setDoc } from "firebase/firestore";
import { auth, db } from "../firebaseConfig";

export default function SettingsScreen() {
  const [loading, setLoading] = useState(true);
  const [caregiver, setCaregiver] = useState(null);
  const [personName, setPersonName] = useState("");
  const [personAge, setPersonAge] = useState("");
  const [medicalNotes, setMedicalNotes] = useState("");
  const [emergencyContact, setEmergencyContact] = useState("");
  const [caneId, setCaneId] = useState("");
  const [saving, setSaving] = useState(false);
  const [idModalVisible, setIdModalVisible] = useState(false);
  const [copied, setCopied] = useState(false);

  const uid = auth.currentUser.uid;

  useEffect(() => {
    const load = async () => {
      const caregiverDoc = await getDoc(doc(db, "caregivers", uid));
      if (caregiverDoc.exists()) setCaregiver(caregiverDoc.data());

      const personDoc = await getDoc(doc(db, "patients", uid));
      if (personDoc.exists()) {
        const data = personDoc.data();
        setPersonName(data.name || "");
        setPersonAge(data.age || "");
        setMedicalNotes(data.medicalNotes || "");
        setEmergencyContact(data.emergencyContact || "");
        setCaneId(data.caneId || "");
      }
      setLoading(false);
    };
    load();
  }, []);

  const handleSave = async () => {
    setSaving(true);
    try {
      await setDoc(doc(db, "patients", uid), {
        caregiverId: uid,
        name: personName,
        age: personAge,
        medicalNotes,
        emergencyContact,
        caneId,
        updatedAt: new Date().toISOString()
      });
      Alert.alert("Saved", "Details updated successfully.");
    } catch (err) {
      Alert.alert("Save failed", err.message);
    } finally {
      setSaving(false);
    }
  };

  const handleCopy = async () => {
    await Clipboard.setStringAsync(uid);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleLogout = () => {
    Alert.alert("Log out", "Are you sure you want to log out?", [
      { text: "Cancel", style: "cancel" },
      { text: "Log out", style: "destructive", onPress: () => signOut(auth) }
    ]);
  };

  if (loading) {
    return (
      <View style={styles.centered}>
        <ActivityIndicator size="large" color="#D32F2F" />
      </View>
    );
  }

  return (
    <KeyboardAvoidingView style={{ flex: 1 }} behavior={Platform.OS === "ios" ? "padding" : undefined}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.sectionTitle}>Your details</Text>
        <View style={styles.card}>
          <View style={styles.readRow}>
            <Ionicons name="person-outline" size={18} color="#999" style={styles.readIcon} />
            <View>
              <Text style={styles.readLabel}>Name</Text>
              <Text style={styles.readValue}>{caregiver?.name || "—"}</Text>
            </View>
          </View>
          <View style={styles.readRow}>
            <Ionicons name="mail-outline" size={18} color="#999" style={styles.readIcon} />
            <View>
              <Text style={styles.readLabel}>Email</Text>
              <Text style={styles.readValue}>{caregiver?.email || auth.currentUser.email}</Text>
            </View>
          </View>
          <View style={[styles.readRow, { marginBottom: 0 }]}>
            <Ionicons name="call-outline" size={18} color="#999" style={styles.readIcon} />
            <View>
              <Text style={styles.readLabel}>Phone</Text>
              <Text style={styles.readValue}>{caregiver?.phone || "—"}</Text>
            </View>
          </View>
        </View>

        <Text style={styles.sectionTitle}>Visually impaired person's details</Text>
        <View style={styles.card}>
          <Text style={styles.inputLabel}>Name</Text>
          <TextInput style={styles.input} value={personName} onChangeText={setPersonName} accessibilityLabel="Name" />

          <Text style={styles.inputLabel}>Age</Text>
          <TextInput
            style={styles.input}
            value={personAge}
            onChangeText={setPersonAge}
            keyboardType="number-pad"
            accessibilityLabel="Age"
          />

          <Text style={styles.inputLabel}>Medical notes</Text>
          <TextInput
            style={[styles.input, styles.multiline]}
            value={medicalNotes}
            onChangeText={setMedicalNotes}
            multiline
            accessibilityLabel="Medical notes"
          />

          <Text style={styles.inputLabel}>Emergency contact</Text>
          <TextInput
            style={styles.input}
            value={emergencyContact}
            onChangeText={setEmergencyContact}
            accessibilityLabel="Emergency contact"
          />

          <Text style={styles.inputLabel}>Cane / device ID</Text>
          <TextInput style={styles.input} value={caneId} onChangeText={setCaneId} accessibilityLabel="Cane or device ID" />

          <TouchableOpacity
            style={[styles.saveButton, saving && styles.saveButtonDisabled]}
            onPress={handleSave}
            disabled={saving}
            accessibilityRole="button"
          >
            <Text style={styles.saveButtonText}>{saving ? "Saving..." : "Save changes"}</Text>
          </TouchableOpacity>
        </View>

        <Text style={styles.sectionTitle}>Cane setup</Text>
        <TouchableOpacity style={styles.card} onPress={() => setIdModalVisible(true)} accessibilityRole="button">
          <View style={styles.readRow}>
            <Ionicons name="key-outline" size={18} color="#1565C0" style={styles.readIcon} />
            <View style={{ flex: 1 }}>
              <Text style={styles.readLabel}>Patient ID</Text>
              <Text style={[styles.readValue, { color: "#1565C0" }]}>Tap to view and copy</Text>
            </View>
            <Ionicons name="chevron-forward" size={18} color="#1565C0" />
          </View>
        </TouchableOpacity>

        <TouchableOpacity style={styles.logoutButton} onPress={handleLogout} accessibilityRole="button">
          <Ionicons name="log-out-outline" size={18} color="#D32F2F" style={{ marginRight: 8 }} />
          <Text style={styles.logoutText}>Log out</Text>
        </TouchableOpacity>
      </ScrollView>

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
              <Text selectable style={styles.idText}>{uid}</Text>
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
    </KeyboardAvoidingView>
  );
}

const styles = StyleSheet.create({
  centered: { flex: 1, justifyContent: "center", alignItems: "center" },
  container: { padding: 16, paddingBottom: 40 },
  sectionTitle: { fontSize: 13, fontWeight: "700", color: "#888", marginTop: 20, marginBottom: 8, marginLeft: 4, textTransform: "uppercase" },
  card: { backgroundColor: "#fff", borderRadius: 12, padding: 16, borderWidth: 1, borderColor: "#eee" },
  readRow: { flexDirection: "row", alignItems: "center", marginBottom: 16 },
  readIcon: { marginRight: 12 },
  readLabel: { fontSize: 12, color: "#999" },
  readValue: { fontSize: 15, color: "#222", marginTop: 2 },
  inputLabel: { fontSize: 13, color: "#777", marginBottom: 6, marginTop: 12 },
  input: { borderWidth: 1, borderColor: "#ddd", borderRadius: 8, paddingHorizontal: 12, paddingVertical: 10, fontSize: 15 },
  multiline: { height: 70, textAlignVertical: "top" },
  saveButton: { backgroundColor: "#D32F2F", borderRadius: 10, padding: 14, alignItems: "center", marginTop: 20 },
  saveButtonDisabled: { opacity: 0.6 },
  saveButtonText: { color: "#fff", fontWeight: "700", fontSize: 15 },
  logoutButton: {
    flexDirection: "row",
    justifyContent: "center",
    alignItems: "center",
    marginTop: 28,
    paddingVertical: 14,
    borderWidth: 1,
    borderColor: "#D32F2F",
    borderRadius: 10
  },
  logoutText: { color: "#D32F2F", fontWeight: "700", fontSize: 15 },
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
  idBox: { backgroundColor: "#f5f5f5", borderRadius: 8, paddingVertical: 14, paddingHorizontal: 12, width: "100%", marginBottom: 16 },
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