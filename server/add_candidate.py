import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

cid = int(input("Enter Candidate ID: "))
name = input("Candidate Name: ")
party = input("Party Name: ")
constituency = input("Constituency: ")

c.execute("INSERT INTO candidates VALUES (?,?,?,?,?)",
          (cid, name, party, constituency, 0))

conn.commit()
conn.close()

print("Candidate Added")