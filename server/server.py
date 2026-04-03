from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import sqlite3

app = Flask(__name__)
CORS(app)

RESULT_LOCK = True

def db():
    conn = sqlite3.connect('voting.db')
    conn.row_factory = sqlite3.Row
    return conn

# 🔐 VERIFY FINGERPRINT
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

    if voter is None:
        return jsonify({"status": "invalid"})

    if voter["voted"] == 1:
        return jsonify({"status": "already_voted"})

    return jsonify({
        "status": "valid",
        "voter_id": voter["id"],
        "name": voter["name"]
    })

# 📋 GET CANDIDATES
@app.route('/get_candidates', methods=['POST'])
def get_candidates():
    data = request.json
    constituency = data['constituency']

    conn = db()
    c = conn.cursor()
    c.execute("SELECT * FROM candidates WHERE constituency=?", (constituency,))
    rows = c.fetchall()

    return jsonify([dict(r) for r in rows])

# 🗳️ VOTE
@app.route('/vote', methods=['POST'])
def vote():
    data = request.json
    voter_id = data['voter_id']
    candidate_id = data['candidate_id']

    conn = db()
    c = conn.cursor()

    c.execute("INSERT INTO votes (voter_id, candidate_id) VALUES (?,?)",
              (voter_id, candidate_id))

    c.execute("UPDATE voters SET voted=1 WHERE id=?", (voter_id,))
    c.execute("UPDATE candidates SET votes = votes + 1 WHERE id=?", (candidate_id,))

    conn.commit()
    return jsonify({"status": "success"})

# 📊 LIVE STATS
@app.route('/live_stats')
def stats():
    conn = db()
    c = conn.cursor()

    total_voters = c.execute("SELECT COUNT(*) FROM voters").fetchone()[0]
    total_votes = c.execute("SELECT COUNT(*) FROM votes").fetchone()[0]

    return jsonify({
        "total_voters": total_voters,
        "total_votes": total_votes
    })

# 🔒 PUBLISH RESULTS
@app.route('/publish_results', methods=['POST'])
def publish():
    global RESULT_LOCK
    password = request.json.get("password")

    if password == "admin123":
        RESULT_LOCK = False
        return jsonify({"status": "unlocked"})
    return jsonify({"status": "wrong_password"})
@app.route('/reset_all', methods=['POST'])
def reset_all():
    password = request.json.get("password")

    if password != "admin123":
        return jsonify({"status": "wrong_password"})

    conn = db()
    c = conn.cursor()

    c.execute("DELETE FROM votes")
    c.execute("UPDATE candidates SET votes = 0")
    c.execute("UPDATE voters SET voted = 0")

    conn.commit()
    conn.close()

    return jsonify({"status": "reset_done"})
# 📊 RESULTS DATA
@app.route('/results_data')
def results_data():
    if RESULT_LOCK:
        return jsonify({"status": "locked"})

    conn = db()
    c = conn.cursor()
    rows = c.execute("SELECT name, party, votes FROM candidates").fetchall()

    return jsonify([dict(r) for r in rows])

# 🌐 WEB PAGES
@app.route('/')
def home():
    return send_from_directory('../web', 'index.html')

@app.route('/admin')
def admin():
    return send_from_directory('../web', 'admin.html')
@app.route('/add_voter', methods=['POST'])
def add_voter():
    data = request.json
    name = data['name']
    fingerprint_id = data['fingerprint_id']

    conn = sqlite3.connect("voting.db")
    c = conn.cursor()
    c.execute("INSERT INTO voters (name, fingerprint_id, voted) VALUES (?, ?, 0)", (name, fingerprint_id))
    conn.commit()
    conn.close()

    return {"status": "voter added"}


@app.route('/delete_voter', methods=['POST'])
def delete_voter():
    data = request.json
    fingerprint_id = data['fingerprint_id']

    conn = sqlite3.connect("voting.db")
    c = conn.cursor()
    c.execute("DELETE FROM voters WHERE fingerprint_id=?", (fingerprint_id,))
    conn.commit()
    conn.close()

    return {"status": "voter deleted"}


@app.route('/add_candidate', methods=['POST'])
def add_candidate():
    data = request.json
    name = data['name']

    conn = sqlite3.connect("voting.db")
    c = conn.cursor()
    c.execute("INSERT INTO candidates (name, votes) VALUES (?, 0)", (name,))
    conn.commit()
    conn.close()

    return {"status": "candidate added"}


@app.route('/delete_candidate', methods=['POST'])
def delete_candidate():
    data = request.json
    name = data['name']

    conn = sqlite3.connect("voting.db")
    c = conn.cursor()
    c.execute("DELETE FROM candidates WHERE name=?", (name,))
    conn.commit()
    conn.close()

    return {"status": "candidate deleted"}

# 👥 VIEW ALL VOTERS
@app.route('/view_voters')
def view_voters():
    conn = sqlite3.connect("voting.db")
    conn.row_factory = sqlite3.Row
    c = conn.cursor()

    rows = c.execute("SELECT * FROM voters").fetchall()
    conn.close()

    return jsonify([dict(r) for r in rows])


# 🧑‍💼 VIEW ALL CANDIDATES
@app.route('/view_candidates')
def view_candidates():
    conn = sqlite3.connect("voting.db")
    conn.row_factory = sqlite3.Row
    c = conn.cursor()

    rows = c.execute("SELECT * FROM candidates").fetchall()
    conn.close()

    return jsonify([dict(r) for r in rows])
@app.route('/reset_votes', methods=['POST'])
def reset_votes():
    conn = sqlite3.connect("voting.db")
    c = conn.cursor()
    c.execute("UPDATE voters SET voted=0")
    c.execute("UPDATE candidates SET votes=0")
    conn.commit()
    conn.close()

    return {"status": "votes reset"}
@app.route('/result')
def result():
    return send_from_directory('../web', 'result.html')
# 🔒 LOCK RESULTS AGAIN
@app.route('/lock_results', methods=['POST'])
def lock():
    global RESULT_LOCK
    password = request.json.get("password")

    if password == "admin123":
        RESULT_LOCK = True
        return jsonify({"status": "locked"})
    return jsonify({"status": "wrong_password"})
app.run(host='0.0.0.0', port=5000)
