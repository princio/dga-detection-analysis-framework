
import enum
import io
from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory
import zipfile


class NNType(enum.Enum):
    NONE = 0
    TLD = 1
    ICANN = 2
    PRIVATE = 3
    pass

class NN:
    def __init__(self, id: int, name: str, nntype: NNType, model_json: str, hf5, zip):
        self.id = id
        self.name = name
        if isinstance(nntype, str):
            nntype = NNType[nntype.upper()]
        self.nntype = nntype
        self.model_json = model_json
        self.hf5_data = hf5
        self.hf5_file = NamedTemporaryFile('wb', delete=True)
        self.hf5_file.write(hf5)
        self.hf5_file.flush()
        
        self.zipdir = TemporaryDirectory()#delete=True)
        with zipfile.ZipFile(io.BytesIO(zip.tobytes()), 'r') as zip_ref:
            zip_ref.extractall(self.zipdir.name)
            pass
        pass
    pass