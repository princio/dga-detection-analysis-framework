
import argparse
from pathlib import Path

import pandas as pd


def process(model_dir: Path, s_dn: pd.Series):
    from lstm_univpm import lstm_univpm_run
    s_dn_x, s_dn_y = lstm_univpm_run(s_dn, model_dir)
    return s_dn_x, s_dn_y

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
                    prog='LSTM Processer',
                    description='Author: Lorenzo Principi, 2024')
    

    parser.add_argument('input', help='The csv file path of the domain list.')
    parser.add_argument('output', help='Where to save the output files.')
    parser.add_argument('--col_pos', type=int, default=0, help='The column position where the domains are placed (starting from 0). Default is 0.')
    parser.add_argument('--header', type=int, default=1, help='Number of rows reserved to the header. Default is one.')
    parser.add_argument('--aux', action='store_true', help='Save all the auxiliary files.')

    group = parser.add_mutually_exclusive_group(required=True)

    group.add_argument('--model-path', required=False,help='The model path. The format should be compatible with TF 2.12/13 and in json.')
    group.add_argument(
        '--model', 
        type=str.lower,
        choices=['none', 'icann', 'tld', 'private'],
        help="Scegli tra 'none', 'icann', 'tld', 'private' (ignora maiuscole/minuscole)"
    )
    args = parser.parse_args()

    input = Path(args.input)
    output = Path(args.output)

    df = pd.read_csv(input, header=args.header-1).head(1000)
    s_dn = df[df.columns[args.col_pos]]

    if args.model:
        model_path = Path(__file__).parent.joinpath('models_tf2.13').joinpath(args.model)
    else:
        model_path = args.model_path
    
    print(f'model_path', model_path)
    print(f'columns', args.col_pos, df.columns[args.col_pos] if args.header else None)

    s_dn_x, s_dn_y = process(model_path, s_dn)

    if args.aux:
        pd.concat({'dn': s_dn, 'x': s_dn_x, 'y': s_dn_y }, axis=1, ignore_index=True).to_csv(output.with_suffix('.aux.csv'))
        pass

    pass
