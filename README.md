# Biometric Voting System using ESP32

## Project Description
This project is a biometric voting system that uses an ESP32 microcontroller and R307 fingerprint sensor for voter authentication. The system ensures that only authorized voters can vote and prevents duplicate voting.

The system includes a web-based dashboard built using Python Flask and SQLite database to store voter and candidate data. The admin can publish or lock election results and reset the election for future use.

## Features
- Fingerprint Authentication
- One Person One Vote
- NOTA Option
- Live Voting Dashboard
- Admin Control Panel
- Result Publish/Lock System
- Reset Election Option
- SQLite Database Storage

## Technologies Used
- ESP32
- R307 Fingerprint Sensor
- Python (Flask)
- SQLite
- HTML, CSS, JavaScript
- GitHub
- VS Code

## How to Run
1. Go to server folder:
   cd server
2. Start server:
   python server.py
3. Open browser:
   http://localhost:5000
4. Admin page:
   http://localhost:5000/admin
5. Results page:
   http://localhost:5000/result

## Author
Alfin
