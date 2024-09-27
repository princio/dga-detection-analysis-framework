import pandas as pd
from db import Database
from ratelimit import limits, sleep_and_retry
import requests

db = Database()

df = pd.read_sql('SELECT * FROM DN ORDER BY ID', db.engine)

url = 'https://dgarchive.caad.fkie.fraunhofer.de/reverse'
auth = ('lorenzo_principi', 'galleydrownquintupleoverwritepromenadedonor')

ONE_MINUTE = 60

bdn = df.drop_duplicates(subset='bdn')


print(bdn)