# Smart Cane + Caregiver App — Getting Started Guide

This guide walks through everything needed the first time you set up the cane and app — from creating accounts to how the system works day to day.

There are three one-time setup parts, done in order, followed by how the cane actually behaves once everything is running.

---

## Part 1: Set up the caregiver app

1. Install the app on the caregiver's phone (APK provided separately).
2. Open the app and tap **Sign up**. Enter your name, email, phone number, and a password, then submit.
3. You'll be asked to fill in details about the visually impaired person: their name, age, medical notes, and an emergency contact. Save and continue.
4. You'll land on the **Alerts** screen — empty for now, that's expected.
5. Go to the **Settings** tab (bottom right). Under "Cane setup," tap **Patient ID**, then tap **Copy to clipboard**.
6. Keep this copied — you'll paste it into the cane's setup page in Part 3. If you lose it, you can always come back to this same screen to copy it again.

---

## Part 2: Set up Telegram on the visually impaired person's phone

This part happens on their own phone, since Telegram messages and watch vibrations go to them, not the caregiver.

1. Open Telegram on their phone (install it first if needed).
2. Search for **BotFather** (official bot, has a blue checkmark) and open the chat.
3. Send the message `/newbot`. It'll ask for a name (anything you like, e.g. "ObstacleAlert") and then a username ending in "bot" (e.g. `ObstacleAlert_bot`).
4. BotFather replies with a long code that looks like `123456789:AAExampleTokenHere`. This is the **Bot Token** — copy it somewhere safe. *(This step is much easier with a sighted helper, since a single mistyped character in the token breaks everything downstream.)*
5. Search for the exact bot username you just created and send it any message, like "hi". This is required — Telegram won't let a bot message someone who hasn't messaged it first.
6. Search for **IDBot** (or **userinfobot**) and send `/start`. It instantly replies with a number — this is the **Chat ID**. Copy it.
7. In the bot, enable share location until stopped for sharing the user's location.

You now have two pieces of information ready: the **Bot Token** and the **Chat ID**.

---

## Part 3: Configure the cane

1. Have the home WiFi network name and password ready (check the router, or your phone's WiFi settings).
2. Power on the cane.
3. On a phone, open WiFi settings and connect to the network named **Cane-Setup**.
4. A setup page should open automatically. If it doesn't, open a browser and go to `192.168.4.1`.
5. Fill in all four fields exactly:
   - **WiFi name and password** (from step 1)
   - **Telegram Bot Token** (from Part 2, step 4)
   - **Telegram Chat ID** (from Part 2, step 6)
   - **Patient ID** (from Part 1, step 5)
6. Tap **Save / Connect**. The cane restarts and connects to the real WiFi network.
7. To force reset the WiFi or any other details, switch off the cane, switch on the cane while holding the emergency button and the setup will open.

Setup is now complete. The cane remembers all of this in its own memory — you won't need to repeat this unless the cane moves to a new WiFi network (in which case, hold the emergency button for 5 seconds while powering on to reopen this setup page).

---

## Part 4: How obstacle detection works, day to day

This happens automatically and constantly while walking — no action needed from anyone.

1. The cane's ultrasonic sensor detects something within 60cm.
2. The cane buzzes — faster and more urgently the closer the obstacle is.
3. At the same time, it sends a message to the Telegram bot, including the current GPS location.
4. That Telegram message triggers a **vibration on the visually impaired person's smartwatch**, alerting them even if their phone is in a pocket or bag.

**Important:** this path only goes to Telegram and the watch. The caregiver is **not** notified for routine obstacles — those are frequent and usually minor.

---

## Part 5: How the emergency button works

This is the one action that reaches the caregiver directly.

1. The visually impaired person holds the emergency button on the cane for about 1 second.
2. The cane plays 3 short confirmation beeps, so they know the signal was sent.
3. At the same moment, two things happen at once:
   - A message goes to Telegram (same as the obstacle alerts).
   - An emergency alert is sent directly to the caregiver's app.
4. On the caregiver's phone, the **Alerts** tab updates instantly — no refresh needed — showing "EMERGENCY - patient pressed the SOS button on the cane," along with the time and a link to view the exact location on the map.

---

## Quick reference

| Situation | Who gets notified | How |
|---|---|---|
| Obstacle detected (any distance) | Visually impaired person only | Telegram message → watch vibration |
| Emergency button held | Both the visually impaired person and the caregiver | Telegram message + live caregiver app alert |

| Data to gather during setup | Where it comes from |
|---|---|
| Patient ID | Caregiver app → Settings → Patient ID |
| Telegram Bot Token | BotFather, on the visually impaired person's phone |
| Telegram Chat ID | IDBot, on the visually impaired person's phone |
| WiFi name and password | Home router |