# README

## Requirements

- python da 3.8 in su ma non 3.12, ad oggi (Ottobre 1, 2024).
- Tensorflow 2.13.0, ad oggi (Ottobre 1, 2024).

## predict_file.py

```
usage: Predict file [-h] [-d] [-b BATCH] outdir filename

The input file should be a list of [new-line] separated domain. It will perform the prediction based on 4 nn, each of them will take a different portion of the domain.

positional arguments:
  outdir
  filename

optional arguments:
  -h, --help            show this help message and exit
  -d, --debug
  -b BATCH, --batch BATCH
                        The batch size, zero means no batching.

Text at the bottom of help
```

