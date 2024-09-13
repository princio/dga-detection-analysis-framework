import argparse
import json
import pandas as pd
from sqlalchemy import create_engine
from sqlalchemy.engine import URL
import psycopg2 as psy

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='DNSMalwareDetection',
                    description='Malware detection')


    parser.add_argument('path_whitelist')

    args = parser.parse_args()

    with open("conf.json", 'r') as fp_conf:
        conf = json.load(fp_conf)

    psyconn = psy.connect("host=%s dbname=%s user=%s password=%s" % (conf['postgre']['host'], conf['postgre']['dbname'], conf['postgre']['user'], conf['postgre']['password']))

    url = URL.create(
        drivername="postgresql",
        host=conf['postgre']['host'],
        username=conf['postgre']['user'],
        database=conf['postgre']['dbname'],
        password=conf['postgre']['password']
    )

    engine = create_engine(url)
    connection = engine.connect()

    cursor = psyconn.cursor()

    cursor.execute(b"INSERT INTO whitelist(name, year, \"number\") VALUES (%s, %s, %s) RETURNING id", ["top10m", 2020, 7254902])

    df = pd.read_csv(args.path_whitelist)

    w_id = cursor.lastrowid

    values = [ [ w_id, x["bdn"], x["bdn"], x["rank"] ] for _, x in df.iterrows() ]
    bo = [cursor.mogrify("(%s,%s)", list(x)) for _, x in df.iterrows()]
    args_str = b",".join(bo)
    cursor.execute(b"INSERT INTO whitelists_dn VALUES " + args_str + b" ON CONFLICT DO NOTHING")

    

    pass
