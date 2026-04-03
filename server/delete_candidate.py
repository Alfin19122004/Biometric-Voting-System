import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

cid = int(input("Enter Candidate ID to delete: "))

c.execute("DELETE FROM candidates WHERE id=?", (cid,))

conn.commit()
conn.close()

print("Candidate Deleted")