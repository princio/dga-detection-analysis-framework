"""utils.py"""


def mkdir27(newdir, exist_ok=True, parents=False):
    if not newdir.exists():
        newdir.mkdir(exist_ok=exist_ok, parents=parents)
        pass
    return newdir
