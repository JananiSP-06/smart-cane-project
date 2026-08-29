import React, { useState } from "react";
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Alert,
  KeyboardAvoidingView,
  Platform
} from "react-native";
import { Ionicons } from "@expo/vector-icons";
import { doc, setDoc } from "firebase/firestore";
import { auth, db } from "../firebaseConfig";

export default function ProfileSetupScreen({ onComplete }) {
  const [patientName, setPatientName] = useState("");
  const [patientAge, setPatientAge] = useState("");
  const [medicalNotes, setMedicalNotes] = useState("");
  const [emergencyContact, setEmergencyContact] = useState("");
  const [caneId, setCaneId] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSave = async () => {
    if (!patientName) {
      Alert.alert("Missing info", "Enter their name.");
      return;
    }
    setLoading(true);
    try {
      const uid = auth.currentUser.uid;
      await setDoc(doc(db, "patients", uid), {
        caregiverId: uid,
        name: patientName,
        age: patientAge,
        medicalNotes,
        emergencyContact,
        caneId,
        createdAt: new Date().toISOString()
      });
      onComplete();
    } catch (err) {
      Alert.alert("Save failed", err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <KeyboardAvoidingView
      style={{ flex: 1, backgroundColor: "#fff" }}
      behavior={Platform.OS === "ios" ? "padding" : undefined}
    >
      <ScrollView contentContainerStyle={styles.container} keyboardShouldPersistTaps="handled">
        <View style={styles.iconWrap}>
          <Ionicons name="body-outline" size={36} color="#D32F2F" />
        </View>
        <Text style={styles.title}>Set up their profile</Text>
        <Text style={styles.subtitle}>Details about the visually impaired person using the cane</Text>

        <View style={styles.inputWrap}>
          <Ionicons name="person-outline" size={20} color="#999" style={styles.inputIcon} />
          <TextInput
            style={styles.input}
            placeholder="Their name"
            value={patientName}
            onChangeText={setPatientName}
            accessibilityLabel="Their name"
          />
        </View>

        <View style={styles.inputWrap}>
          <Ionicons name="calendar-outline" size={20} color="#999" style={styles.inputIcon} />
          <TextInput
            style={styles.input}
            placeholder="Age"
            keyboardType="number-pad"
            value={patientAge}
            onChangeText={setPatientAge}
            accessibilityLabel="Their age"
          />
        </View>

        <View style={[styles.inputWrap, styles.multilineWrap]}>
          <Ionicons name="document-text-outline" size={20} color="#999" style={styles.inputIcon} />
          <TextInput
            style={[styles.input, styles.multiline]}
            placeholder="Medical notes (allergies, conditions, medications)"
            multiline
            value={medicalNotes}
            onChangeText={setMedicalNotes}
            accessibilityLabel="Medical notes"
          />
        </View>

        <View style={styles.inputWrap}>
          <Ionicons name="call-outline" size={20} color="#999" style={styles.inputIcon} />
          <TextInput
            style={styles.input}
            placeholder="Emergency contact (name + phone)"
            value={emergencyContact}
            onChangeText={setEmergencyContact}
            accessibilityLabel="Emergency contact"
          />
        </View>

        <View style={styles.inputWrap}>
          <Ionicons name="hardware-chip-outline" size={20} color="#999" style={styles.inputIcon} />
          <TextInput
            style={styles.input}
            placeholder="Cane / device ID (optional)"
            value={caneId}
            onChangeText={setCaneId}
            accessibilityLabel="Cane or device ID"
          />
        </View>

        <TouchableOpacity
          style={[styles.button, loading && styles.buttonDisabled]}
          onPress={handleSave}
          disabled={loading}
          accessibilityRole="button"
          accessibilityLabel="Save and continue"
        >
          <Text style={styles.buttonText}>{loading ? "Saving..." : "Save and continue"}</Text>
        </TouchableOpacity>
      </ScrollView>
    </KeyboardAvoidingView>
  );
}

const styles = StyleSheet.create({
  container: { padding: 24, flexGrow: 1, justifyContent: "center" },
  iconWrap: {
    alignSelf: "center",
    width: 72,
    height: 72,
    borderRadius: 36,
    backgroundColor: "#FDEDED",
    justifyContent: "center",
    alignItems: "center",
    marginBottom: 20
  },
  title: { fontSize: 24, fontWeight: "700", textAlign: "center" },
  subtitle: { fontSize: 14, color: "#777", textAlign: "center", marginBottom: 28, marginTop: 6 },
  inputWrap: {
    flexDirection: "row",
    alignItems: "center",
    borderWidth: 1,
    borderColor: "#ddd",
    borderRadius: 10,
    marginBottom: 14,
    paddingHorizontal: 12
  },
  multilineWrap: { alignItems: "flex-start", paddingTop: 12 },
  inputIcon: { marginRight: 8, marginTop: 2 },
  input: { flex: 1, paddingVertical: 14, fontSize: 16 },
  multiline: { height: 80, textAlignVertical: "top", paddingTop: 0 },
  button: {
    backgroundColor: "#D32F2F",
    borderRadius: 10,
    padding: 16,
    alignItems: "center",
    marginTop: 8
  },
  buttonDisabled: { opacity: 0.6 },
  buttonText: { color: "#fff", fontSize: 16, fontWeight: "700" }
});