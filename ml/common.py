import pandas as pd

csv = None

label_col = ('-',      'source',             'mc')

features_cols = [
    (  '-',           '-',      'n_message'),
    
    (  '-',           '-',     'fn_req_min'),
    (  '-',           '-',     'fn_req_max'),
    
    (  '-',           '-',   'time_s_start'),
    (  '-',           '-',     'time_s_end'),
    (  '-',           '-',       'duration'),
    
    (  '-',           '-',             'qr'),
    (  '-',           '-',              'u'),
    (  '-',           '-',              'q'),
    (  '-',           '-',              'r'),
    (  '-',           '-',             'nx'),
    
    (  '-', 'whitelisted',              '0'),
    (  '-', 'whitelisted',             '100'),
    (  '-', 'whitelisted',             '1000'),
    (  '-', 'whitelisted',             '1000000'),
    
    ('eps',        'NONE', '[0.000, 0.100)'),
    ('eps',        'NONE', '[0.100, 0.250)'),
    ('eps',        'NONE', '[0.250, 0.500)'),
    ('eps',        'NONE', '[0.500, 0.900)'),
    ('eps',        'NONE', '[0.900, 1.100)'),
    
    ('eps',         'TLD', '[0.000, 0.100)'),
    ('eps',         'TLD', '[0.100, 0.250)'),
    ('eps',         'TLD', '[0.250, 0.500)'),
    ('eps',         'TLD', '[0.500, 0.900)'),
    ('eps',         'TLD', '[0.900, 1.100)'),
    
    ('eps',       'ICANN', '[0.000, 0.100)'),
    ('eps',       'ICANN', '[0.100, 0.250)'),
    ('eps',       'ICANN', '[0.250, 0.500)'),
    ('eps',       'ICANN', '[0.500, 0.900)'),
    ('eps',       'ICANN', '[0.900, 1.100)'),
    
    ('eps',     'PRIVATE', '[0.000, 0.100)'),
    ('eps',     'PRIVATE', '[0.100, 0.250)'),
    ('eps',     'PRIVATE', '[0.250, 0.500)'),
    ('eps',     'PRIVATE', '[0.500, 0.900)'),
    ('eps',     'PRIVATE', '[0.900, 1.100)'),
    
    ('llr',        'ninf',           'NONE'),
    ('llr',        'ninf',            'TLD'),
    ('llr',        'ninf',          'ICANN'),
    ('llr',        'ninf',        'PRIVATE'),
    
    ('llr',        'pinf',           'NONE'),
    ('llr',        'pinf',            'TLD'),
    ('llr',        'pinf',          'ICANN'),
    ('llr',        'pinf',        'PRIVATE'),
    
    ('llr',        'NONE',              '0'),
    ('llr',        'NONE',             '100'),
    ('llr',        'NONE',             '1000'),
    ('llr',        'NONE',             '1000000'),
    
    ('llr',         'TLD',              '0'),
    ('llr',         'TLD',             '100'),
    ('llr',         'TLD',             '1000'),
    ('llr',         'TLD',             '1000000'),
    
    ('llr',       'ICANN',              '0'),
    ('llr',       'ICANN',             '100'),
    ('llr',       'ICANN',             '1000'),
    ('llr',       'ICANN',             '1000000'),
    
    ('llr',     'PRIVATE',              '0'),
    ('llr',     'PRIVATE',             '100'),
    ('llr',     'PRIVATE',             '1000'),
    ('llr',     'PRIVATE',             '1000000')
]

def load_dataset():
    global csv
    if csv is None:
        csv = pd.read_csv("./windows.csv", header=[0,1,2])
        csv[('-', '-', 'duration')] = csv[('-', '-', 'time_s_end')] - csv[('-', '-', 'time_s_start')]
        pass
    return csv[features_cols].copy(), csv[label_col].copy()
