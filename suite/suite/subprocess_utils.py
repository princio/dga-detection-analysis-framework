
from pathlib import Path
import subprocess

from config import Config


def subprocess_rename_file_if_error(test: bool, path_currentfile: Path):
    if test:
        path_errorfile = f"{path_currentfile}.error"
        if path_currentfile.exists():
            path_currentfile.rename(path_errorfile)
            pass
        pass
    pass

def subprocess_launch(config: Config, file_output: Path, name, args):
    if not file_output.exists():
        print(f"[info]: {name} step...")
        with open(config.workdir.log.joinpath(f"{name}.log"), "w") as fplog:
            p = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )

            output, errors = p.communicate(timeout=100)

            fplog.writelines(output)
            fplog.writelines(errors)

            subprocess_rename_file_if_error(p.returncode != 0, file_output)

            if p.returncode != 0:
                print(output)
                print(errors)
                raise Exception(f"{name} execution failed")

            pass
        pass
    else:
        print(f"[info]: skipping {name}, file already exists.")
        pass
    pass

