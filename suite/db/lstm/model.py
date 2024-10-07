from ..common import Database
from ..defs import NNType

class DBModel:
    def __init__(self, db: Database, id: int):
        self.db = db
        self.id = id
        self.name = ''
        self.extractor = DBModelType.NONE
        self.model_json = ''
        self.tempfile = tempfile.NamedTemporaryFile('wb', delete=False)
        self.fetched = False
    pass

    def fetch(self):
        if self.fetched:
            return
        with self.db.psycopg2().cursor() as cursor:
            cursor.execute("SELECT name, extractor, model_json, model_hf5 FROM nn WHERE id=%s", (self.id,))
            tmp = cursor.fetchone()
            if tmp is None:
                raise Exception('Query failed.')

            self.name = tmp[0]
            self.extractor = tmp[1]
            self.model_json = tmp[2]
            self.tempfile.write(tmp[3])
            self.fetched = True
            pass
        pass
    pass