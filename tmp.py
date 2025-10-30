s = """SELECT %d AS DAY, * FROM MV3_%d"""

q = ""
for i in range(10):
    q += s % (i, i) + '\nUNION ALL\n'

print(q)