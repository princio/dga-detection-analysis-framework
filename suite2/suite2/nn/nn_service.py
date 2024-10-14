
from tempfile import NamedTemporaryFile, TemporaryDirectory
import tempfile
import zipfile

from ..defs import NN
from ..db import Database


class NNService:
    def __init__(self, db: Database):
        self.db = db
        pass

    def get_model(self, nn_id: int):

        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT name, extractor, model_json, model_hf5, model_portable FROM nn WHERE id=%s", (nn_id,))
            tmp = cursor.fetchone()
            if tmp is None:
                raise Exception('Query failed.')
        
        tempfile = NamedTemporaryFile('wb')#, delete=False)
        tempfile.write(tmp[3])
        tempfile.flush()
        return NN(nn_id, tmp[0], tmp[1], tmp[2], tmp[3], tmp[4])

    def get_all(self):
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("""SELECT id FROM NN;""")
            ids = cursor.fetchall()
            if ids is None:
                raise Exception('Query failed.')
            pass
        return [ self.get_model(id[0]) for id in ids ]