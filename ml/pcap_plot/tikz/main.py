import numpy as np



def ciao():
    bars = 8
    H = np.random.normal(4, 0.8, bars).round(2)
    M = np.random.normal(3, 0.5, bars).round(2)

    maxmax = max(H.max(), M.max())

    _txt = ''
    def write(x):
        nonlocal _txt
        _txt += x + '\n'
        pass


    write(r"""
    \documentclass{standalone}
    \usepackage{tikz}

    \begin{document}

    \begin{tikzpicture}

    """)

    cc = { 'm': 'red!20', 'h': 'blue!20'}
    vv = { 'm': M, 'h': H}


    margin = 0.25
    alpha = 5
    xorigin = 0
    yorigin = 0
    Wa = 0.8
    for i, l in enumerate(['h', 'm']):
        c = cc[l]
        v = vv[l]

        for x, y in enumerate(v):
            write(rf'\draw[fill={c}] ({xorigin + x * Wa + margin}, {yorigin}) rectangle ++({Wa - margin*2}, {y});')
            pass

        for x, y in enumerate(v):
            write(rf'\node at ({xorigin + x * Wa + Wa / 2}, {yorigin - 0.35}) {{${l}_{x}$}};')
            pass

        for s in range(len(v) - alpha + 1):
            x = xorigin + s * Wa
            y = v.max() / 4 + s * .6 + yorigin
            arrow_pad = 0.08
            write(rf'\draw[|-|,thick] ({x + margin - arrow_pad}, {y}) -- ({x + alpha * Wa - margin + arrow_pad}, {y}) node[inner sep=2pt, fill=white, draw, thin, midway, centered] {{${l.upper()}_{s}$}};')
            pass

        write(rf'\draw[thin] ({xorigin}, 0) rectangle ++({Wa * bars}, {maxmax * 1.1});')

        write(rf'\node[fill=white,draw,thin,anchor=center] at ({xorigin + (Wa * bars)/2}, {maxmax*1.1}) {{$T^{l}$}};')

        xorigin = Wa * bars + 2

        pass



    c = { 'm': 'red!20', 'h': 'blue!20'}[l]

    iota = 2
    kappa = 3
    Wb = 0.3
    margin = 0.8
    pad = 0.5
    WW = (Wb + margin) * 2 + pad
    xorigin = ((2 + (Wa * bars) * 2) - (WW * alpha))/2
    yorigin -= 7
    for x in range(alpha):

        h = vv['h'][x + iota]
        m = vv['h'][x + kappa]
        x0 = xorigin + x * WW + margin
        write(rf'\draw[fill={cc['h']}] ({x0}, {yorigin}) rectangle ++({Wb}, {h});')
        write(rf'\draw[fill={cc['m']}] ({x0 + pad + Wb}, {yorigin}) rectangle ++({Wb}, {m});')

        write(rf'\node[anchor=west] at ({x0 + Wb/2 - Wb}, {yorigin - 0.35}) {{$h_{x + iota}$}};')
        write(rf'\node[anchor=west] at ({x0 + pad + Wb/2}, {yorigin - 0.35}) {{$m_{x + kappa}$}};')

        write(rf'\node[anchor=west] at ({x0 + Wb/2 -Wb}, {yorigin - 0.75}) {{$h_{{{iota}_{x}}}$}};')
        write(rf'\node[anchor=west] at ({x0 + pad + Wb/2}, {yorigin - 0.75}) {{$m_{{{kappa}_{x}}}$}};')
        pass

    write(rf'\draw[thin] ({xorigin}, 0) rectangle ++({Wb * bars}, {maxmax * 1.1});')

    write(rf'\node[fill=white,draw,thin,anchor=center] at ({xorigin + (Wb * bars)/2}, {maxmax*1.1}) {{$O_{{{iota},{kappa}$}}$}};')

    # for s in range(len(v) - alpha + 1):
    #     x = xorigin + s * W
    #     y = v.max() / 4 + s * .6 + yorigin
    #     arrow_pad = 0.08
    #     txt(rf'\draw[|-|,thick] ({x + margin - arrow_pad}, {y}) -- ({x + alpha * W - margin + arrow_pad}, {y}) node[inner sep=2pt, fill=white, draw, thin, midway, centered] {{${l.upper()}_{s}$}};')
    #     txt'\n'

    write(r"""

            \node at (5,10) {ciao};
    \end{tikzpicture}

    \end{document}

    """)

    with open('main.tex', 'w') as fp:
        fp.write(_txt)
    pass

ciao()