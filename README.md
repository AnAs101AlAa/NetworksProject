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
You can tweak the simulation settings to test different scenarios and analyze the performance of Go-Back-N in various network conditions.

## 🤝 Contributions
Feel free to submit **issues** or **pull requests** to improve the simulation.

## 📄 License
This project is licensed under the **MIT License**.

## 📩 Contact
For questions or collaboration, reach out via [your.email@example.com](mailto:your.email@example.com).

