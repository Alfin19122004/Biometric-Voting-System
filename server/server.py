from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import sqlite3
import os

app = Flask(__name__, static_folder='../web')
CORS(app)

# Database path - Ensure this directory exists on your server
DB_PATH = "voting.db" 

# Result lock status
RESULT_LOCK = True
ADMIN_PASS = "admin123"

# ---------------- DATABASE HELPER ----------------
def db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# ---------------- VOTER VERIFICATION ----------------
@app.route('/verify_fingerprint', methods=['POST'])
def verify():
    data = request.json
    fid = data['fingerprint_id']
    state = data['state']
    district = data['district']
    constituency = data['constituency']

    conn = db()
    c = conn.cursor()
    c.execute("SELECT * FROM voters WHERE fingerprint_id=? AND state=? AND district=? AND constituency=?",
              (fid, state, district, constituency))
    voter = c.fetchone()
    conn.close()

    if voter is None:
        return jsonify({"status": "invalid"})

    if voter["voted"] == 1:
        return jsonify({"status": "already_voted"})

    return jsonify({
        "status": "valid",
        "voter_id": voter["id"],
        "name": voter["name"]
    })

# ---------------- BULK REGISTRATION ----------------
@app.route('/add_bulk_voters', methods=['POST'])
def add_bulk_voters():
    data = request.json
    voters = data.get('voters', [])
    password = data.get('password')

    if password != ADMIN_PASS:
        return jsonify({"status": "wrong_password"}), 403

    conn = db()
    c = conn.cursor()
    added = 0
    skipped = 0

    for v in voters:
        try:
            c.execute("""
                INSERT INTO voters (name, state, district, constituency, fingerprint_id, voted) 
                VALUES (?, ?, ?, ?, ?, 0)
            """, (
                v.get('name'), 
                v.get('state', 'Tamil Nadu'), 
                v.get('district', 'Chennai'),
                v.get('constituency', 'ABC'),
                v.get('fingerprint_id')
            ))
            added += 1
        except sqlite3.IntegrityError:
            skipped += 1
        except Exception as e:
            print(f"Bulk Error: {e}")
            skipped += 1

    conn.commit()
    conn.close()
    return jsonify({"status": f"Success: {added} added, {skipped} skipped"})

# ---------------- VOTING SYSTEM ----------------
@app.route('/vote', methods=['POST'])
def vote():
    data = request.json
    voter_id = data['voter_id']
    candidate_id = data['candidate_id']

    conn = db()
    c = conn.cursor()
    try:
        c.execute("INSERT INTO votes (voter_id, candidate_id) VALUES (?,?)", (voter_id, candidate_id))
        c.execute("UPDATE voters SET voted=1 WHERE id=?", (voter_id,))
        c.execute("UPDATE candidates SET votes = votes + 1 WHERE id=?", (candidate_id,))
        conn.commit()
        status = "success"
    except Exception as e:
        print(f"Vote Error: {e}")
        status = "error"
    finally:
        conn.close()
    return jsonify({"status": status})

# ---------------- ADMIN CONTROLS ----------------
@app.route('/live_stats')
def stats():
    conn = db()
    c = conn.cursor()
    total_voters = c.execute("SELECT COUNT(*) FROM voters").fetchone()[0]
    total_votes = c.execute("SELECT COUNT(*) FROM votes").fetchone()[0]
    conn.close()
    return jsonify({"total_voters": total_voters, "total_votes": total_votes})

@app.route('/publish_results', methods=['POST'])
def publish():
    global RESULT_LOCK
    if request.json.get("password") == ADMIN_PASS:
        RESULT_LOCK = False
        return jsonify({"status": "unlocked"})
    return jsonify({"status": "wrong_password"})

@app.route('/lock_results', methods=['POST'])
def lock():
    global RESULT_LOCK
    if request.json.get("password") == ADMIN_PASS:
        RESULT_LOCK = True
        return jsonify({"status": "locked"})
    return jsonify({"status": "wrong_password"})

@app.route('/reset_all', methods=['POST'])
def reset_all():
    if request.json.get("password") != ADMIN_PASS:
        return jsonify({"status": "wrong_password"})

    conn = db()
    c = conn.cursor()
    c.execute("DELETE FROM votes")
    c.execute("UPDATE candidates SET votes = 0")
    c.execute("UPDATE voters SET voted = 0")
    conn.commit()
    conn.close()
    return jsonify({"status": "reset_done"})

# ---------------- VIEW DATA ----------------
@app.route('/view_voters')
def view_voters():
    conn = db()
    rows = conn.execute("SELECT * FROM voters").fetchall()
    conn.close()
    return jsonify([dict(r) for r in rows])

@app.route('/view_candidates')
def view_candidates():
    conn = db()
    rows = conn.execute("SELECT * FROM candidates").fetchall()
    conn.close()
    return jsonify([dict(r) for r in rows])

@app.route('/results_data')
def results_data():
    if RESULT_LOCK:
        return jsonify({"status": "locked"})
    conn = db()
    rows = conn.execute("SELECT name, party, votes FROM candidates").fetchall()
    conn.close()
    return jsonify([dict(r) for r in rows])

# ---------------- ROUTING ----------------
@app.route('/')
def home():
    return send_from_directory(app.static_folder, 'index.html')

@app.route('/admin')
def admin():
    return send_from_directory(app.static_folder, 'admin.html')

@app.route('/result')
def result():
    return send_from_directory(app.static_folder, 'result.html')

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)