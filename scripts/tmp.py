import csv
from pathlib import Path
import sys
from tempfile import NamedTemporaryFile
from h11 import Data
import pandas as pd

sys.path.append(str(Path(__file__).resolve().parent.parent.joinpath('suite2').absolute()))

from suite2.db import Database

db = Database('debug', 'localhost', 'postgres', 'postgre', 'dns_mac')


with db.psycopg2().cursor() as cursor:
    cursor.execute('select * from dn where id=%s', (20732290,))

    a = cursor.fetchone()

    print(a)