# Go-Back-N Protocol Simulation in OMNeT++

## 📌 Overview
This project simulates two devices communicating using the **Go-Back-N (GBN) protocol** in **OMNeT++**. The simulation demonstrates how GBN handles packet loss, acknowledgments, and retransmissions to ensure reliable data transfer.

## ⚙ How It Works
The Go-Back-N protocol is an **ARQ (Automatic Repeat reQuest) protocol** that ensures reliable data transfer by:
- Sending multiple frames up to a **window size (N)** before requiring an acknowledgment.
- Retransmitting all unacknowledged frames when a packet is lost or an error occurs.
- Maintaining a sliding window to efficiently handle data flow.

## 📊 Simulation Features
✔ **Configurable window size (N), transmission and timeout delays, and error frequency**  
✔ **Packet loss simulation**  
✔ **Timeout & retransmission handling**  
✔ **Performance metrics (throughput, retransmissions, etc.)**  
✔ **Log visualization for debugging**  

## 🚀 Usage
You can take this code and tweak it to test different error loads, different data framing algorithms and such.
code is written in OOP format so you can add or remove from it more easily independant of GBN functionality.

## 🤝 Contributions
Lots of thanks to @OmarElshereef for helping in code writing, testing, and simulation optimization.

