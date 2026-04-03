import sqlite3

conn = sqlite3.connect("voting.db")
c = conn.cursor()

c.execute("DELETE FROM votes")
c.execute("UPDATE candidates SET votes = 0")
c.execute("UPDATE voters SET voted = 0")
conn.commit()

conn.close()

print("All votes reset")