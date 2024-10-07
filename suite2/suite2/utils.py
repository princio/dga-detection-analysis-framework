
from pathlib import Path
import subprocess

class Subprocess:
    INPUT_FLAG = f'%input'
    OUTPUT_FLAG = f'%output'
    def __init__(self, name, binary: Path, outdir: Path):
        self.name = name
        self.binary = binary
        self.outdir = outdir.joinpath(self.name)
        if not self.outdir.exists():
            self.outdir.mkdir(parents=False, exist_ok=True)
            pass
        self.output: Path = self.outdir.joinpath(f'{self.name}.output')
        self.logfile: Path = self.outdir.joinpath(f'{self.name}.log')
        pass

    def already_done(self):
        if self.output is not None and self.output.exists():
            return True
        return False

    def rename_file_if_error(self, is_test_failed: bool):
        if is_test_failed and self.output is not None and self.output.exists():
            self.output.rename(f"{self.output}.error")
        pass
    
    def launch(self, input: Path, args):
        if self.already_done():
            return
        
        args = [ input if arg == Subprocess.INPUT_FLAG else arg for arg in args ]
        args = [ self.output if arg == Subprocess.OUTPUT_FLAG else arg for arg in args ]
        args = [ arg.absolute() if isinstance(arg, Path) else arg for arg in args ]
        args = [self.binary] + args

        print(f"[info]: {self.name} step...")
        with open(self.logfile, "w") as fplog:
            try:
                p = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
            except Exception as e:
                raise Exception(e, "\n\n".join([ str(arg) for arg in args ]))

            output, errors = p.communicate(timeout=100000)

            fplog.writelines(output)
            fplog.writelines(errors)

            self.rename_file_if_error(p.returncode != 0)

            if p.returncode != 0:
                print(output)
                print(errors)
                raise Exception(f"{self.name} execution failed")
            pass
        pass # method launch
    pass # class Subprocess