import pandas as pd

from config import Config, ConfigWhitelist
from utils import pandas_batching


def _batch_whitelist_insert(__df_batch: pd.DataFrame, whitelisting):
    df_batch = __df_batch.copy()
    whitelistconfig: ConfigWhitelist = whitelisting.whitelistconfig

    with whitelisting.config.psyconn.cursor() as cursor:
        if not whitelistconfig.rank_col == "#index":
            df_batch.index.set_names("rank")
            df_batch.reset_index(inplace=True)
            pass
        elif not whitelistconfig.rank_col in df_batch:
            df_batch["rank"] = 1
        else:
            df_batch["rank"] = df_batch[whitelistconfig.rank_col]

        df_batch["wl_id"] = whitelisting.id
        values = df_batch[["wl_id", whitelistconfig.dn_col, "rank"]].values.tolist()
        args_str = b",".join([cursor.mogrify("(%s, %s, %s)", x) for x in values])
        cursor.execute(b"""
            INSERT INTO public.whitelist_list(whitelist_id, dn, rank)
                VALUES """ +
            args_str +
            b" ON CONFLICT DO NOTHING RETURNING id")
        
        cursor.connection.commit()

        pass
    pass

class Whitelisting:
    def __init__(self, config: Config, whitelist: ConfigWhitelist) -> None:
        self.config = config
        self.whitelistconfig = whitelist
        self.id = 1
        self.rows_inserted = 0
        pass

    def run_fetch(self):
        with self.config.psyconn.cursor() as cursor:

            cursor.execute("SELECT id FROM WHITELIST WHERE name=%s and year=%s", (self.whitelistconfig.name, self.whitelistconfig.year,))
            tmp = cursor.fetchone()
            if tmp:
                self.id = tmp[0]
                pass
            else:
                raise Exception("Whitelist (%s,%s) not exists" % (self.whitelistconfig.name, self.whitelistconfig.year))
            pass

            cursor.execute("SELECT COUNT(*) FROM WHITELIST_LIST WHERE whitelist_id=%s", (self.id,))
            tmp = cursor.fetchone()
            if tmp:
                self.rows_inserted = tmp[0]
                pass
            pass
        pass

    def run_insert(self, df):
        with self.config.psyconn.cursor() as cursor:
            cursor.execute("""
                INSERT INTO whitelist
                    (name, year, "count")
                VALUES (%s, %s, %s)
                    ON CONFLICT DO NOTHING RETURNING id""",
                (self.whitelistconfig.name, self.whitelistconfig.year, df.shape[0])
            )

            tmp = cursor.fetchone()
            if tmp is None:
                self.run_fetch()
            else:
                self.id = tmp[0]
                self.rows_inserted = 0
                pass

            cursor.execute("""CREATE TABLE IF NOT EXISTS whitelist_list_%s PARTITION OF whitelist_list FOR VALUES IN (%s);""", (self.id, self.id, ))
            cursor.connection.commit()
            pass
        return True

    def run_add(self):
        df = pd.read_csv(self.whitelistconfig.path, index_col=False)
        
        self.run_insert(df)

        diff = df.shape[0] - self.rows_inserted

        print("Whitelist#%s " % (self.id), end="")
        if diff < 0:
            raise Exception("something is wrong: %s records over %s." % (self.rows_inserted, df.shape[0]))
        
        if diff > 0:
            print("requires to insert %s records in the database." % (diff))
            pandas_batching(df.iloc[self.rows_inserted:], 100_000, _batch_whitelist_insert, self)
            pass
        else: # diff == 0
            print("already filled to the database.")
            pass

        pass
    pass

    def run_merge_dn(self):
        with self.config.psyconn.cursor() as cursor:
            cursor.execute("""
                INSERT INTO
                    PUBLIC.WHITELIST_DN (DN_ID, WHITELIST_ID, RANK_DN, RANK_BDN)
                    (
                        SELECT
                        J_DN.DN_ID,
                        1,
                        J_DN.DN_RANK,
                        J_BDN.BDN_RANK
                        FROM
                        (
                            SELECT
                            DN.ID AS DN_ID,
                            WL_DN.RANK AS DN_RANK
                            FROM
                            DN
                            LEFT JOIN WHITELIST_LIST_%s WL_DN ON DN.DN = WL_DN.DN
                        ) AS J_DN
                        JOIN (
                            SELECT
                            DN.ID AS DN_ID,
                            WL_DN.RANK AS BDN_RANK
                            FROM
                            DN
                            LEFT JOIN WHITELIST_LIST_%s WL_DN ON DN.BDN = WL_DN.DN
                        ) AS J_BDN ON J_DN.DN_ID = J_BDN.DN_ID
                    )
                ON CONFLICT DO NOTHING
                RETURNING ID;""",
                (self.id, self.id,)
            )
            cursor.connection.commit()
            pass
        pass
    pass
