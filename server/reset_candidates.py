import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

# Reset all candidate votes
c.execute("UPDATE candidates SET votes = 0")

conn.commit()
conn.close()

print("All candidate votes reset to 0")