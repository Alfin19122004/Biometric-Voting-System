import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

print("----- CANDIDATES LIST -----")

for row in c.execute("SELECT * FROM candidates"):
    print("Candidate ID:", row[0])
    print("Name:", row[1])
    print("Party:", row[2])
    print("Constituency:", row[3])
    print("Votes:", row[4])
    print("--------------------------")

conn.close()