
from logging import Logger
import logging
from pathlib import Path
import subprocess
import traceback

class Subprocess:
    INPUT_FLAG = f'%input'
    OUTPUT_FLAG = f'%output'
    def __init__(self, name, binary: Path, outdir: Path):
        self.name = name
        self.binary = binary
        self.outdir = outdir
        if not self.outdir.exists():
            self.outdir.mkdir(parents=False, exist_ok=True)
            pass
        self.output: Path = self.outdir.joinpath(f'{self.name}.output')
        self.logfile: Path = self.outdir.joinpath(f'{self.name}.log')
        pass

    def already_done(self):
        return self.output is not None and self.output.exists()

    def rename_file_if_error(self, is_test_failed: bool):
        if is_test_failed and self.output is not None and self.output.exists():
            logging.debug(f"Renaming output file {self.output} with suffix «.error».")
            self.output.rename(f"{self.output}.error")
        pass
    
    def launch(self, input: Path, args):
        if self.already_done():
            logging.debug(f"{self.name} already done for input: «{input}»")
            return
        
        logging.debug(f"Proceeding with {self.name} with input: «{input}»")
        
        args = [ input if arg == Subprocess.INPUT_FLAG else arg for arg in args ]
        args = [ self.output if arg == Subprocess.OUTPUT_FLAG else arg for arg in args ]
        args = [ arg.absolute() if isinstance(arg, Path) else arg for arg in args ]
        args = [self.binary] + args

        with open(self.logfile, "w") as fplog:
            try:
                p = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True
                )
            except Exception as e:
                logging.error("Error with arguments:\n> %s" % "\n> ".join([ str(arg) for arg in args ]))
                raise Exception('Subprocess failure.') from e

            output, errors = p.communicate(timeout=100000)

            fplog.writelines(output)
            fplog.writelines(errors)

            self.rename_file_if_error(p.returncode != 0)

            if p.returncode != 0:
                logoutput = '\n> '.join(output.split('\n'))
                logerror = '\n~ '.join(output.split('\n'))
                raise Exception(f"{self.name} execution failed:\n{logoutput}\n{logerror}")
            pass
        pass # method launch
    pass # class Subprocess