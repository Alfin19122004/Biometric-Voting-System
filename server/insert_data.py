import sqlite3

conn = sqlite3.connect('voting.db')
c = conn.cursor()

# Insert Voter
c.execute("INSERT INTO voters (id, name, state, district, constituency, fingerprint_id, voted) VALUES (1,'Alfin','Tamil Nadu','Chennai','ABC',1,0)")

# Insert Candidates
c.execute("INSERT INTO candidates (id, name, party, constituency, votes) VALUES (1,'Candidate A','Party A','ABC',0)")
c.execute("INSERT INTO candidates (id, name, party, constituency, votes) VALUES (2,'Candidate B','Party B','ABC',0)")
c.execute("INSERT INTO candidates (id, name, party, constituency, votes) VALUES (3,'Candidate C','Party C','ABC',0)")

conn.commit()
conn.close()

print("Sample Data Inserted")