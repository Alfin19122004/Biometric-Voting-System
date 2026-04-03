import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

voter_id = int(input("Enter Voter ID to delete: "))

c.execute("DELETE FROM voters WHERE id=?", (voter_id,))

conn.commit()
conn.close()

print("Voter Deleted")