import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

print("----- VOTERS LIST -----")

for row in c.execute("SELECT * FROM voters"):
    print("ID:", row[0])
    print("Name:", row[1])
    print("State:", row[2])
    print("District:", row[3])
    print("Constituency:", row[4])
    print("Fingerprint ID:", row[5])
    print("Voted:", row[6])
    print("----------------------")

conn.close()