import sqlite3

conn = sqlite3.connect('voting.db')
c = conn.cursor()

# Voters
c.execute('''
CREATE TABLE IF NOT EXISTS voters (
id INTEGER PRIMARY KEY,
name TEXT,
state TEXT,
district TEXT,
constituency TEXT,
fingerprint_id INTEGER,
voted INTEGER DEFAULT 0
)
''')

# Candidates
c.execute('''
CREATE TABLE IF NOT EXISTS candidates (
id INTEGER PRIMARY KEY,
name TEXT,
party TEXT,
constituency TEXT,
votes INTEGER DEFAULT 0
)
''')

# Votes
c.execute('''
CREATE TABLE IF NOT EXISTS votes (
id INTEGER PRIMARY KEY AUTOINCREMENT,
voter_id INTEGER,
candidate_id INTEGER,
time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
''')

conn.commit()
conn.close()

print("Database Created Successfully")
