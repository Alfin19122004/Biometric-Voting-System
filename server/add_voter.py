import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

voter_id = int(input("Enter Voter ID: "))
name = input("Enter Name: ")
state = input("Enter State: ")
district = input("Enter District: ")
constituency = input("Enter Constituency: ")
fingerprint_id = int(input("Enter Fingerprint ID: "))

c.execute("INSERT INTO voters VALUES (?,?,?,?,?,?,?)",
          (voter_id, name, state, district, constituency, fingerprint_id, 0))

conn.commit()
conn.close()

print("Voter Added Successfully")