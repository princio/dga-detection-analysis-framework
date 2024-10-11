import numpy as np



def ciao():
    bars = 8

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

    cc = { 'm': 'red', 'h': 'blue'}
    bars = { 'h': 3, 'm': 11
             }


    alpha = 3
    xorigin = 0
    yorigin = 0
    Wa = 1
    pad = 0
    H=Wa
    W = (Wa + pad)
    kappas = [6,0]
    for kappa in kappas:


        # write(rf'\node [align=center, font=\small] at ({xorigin + (kappa) * W + alpha/2*W}, {yorigin + H*2.5}) {{window group\\with slide step {kappa}}};')

        write(rf'\node [align=center, font=\small] at ({xorigin + (kappa) * W + alpha/2*W}, {yorigin - H*1.75}) {{window group\\with slide step {kappa}}};')
        for i, l in enumerate(['h', 'm'][::-1]):
            if l == 'h':
                write(rf'\node[anchor=center,align=center] at (-1, {yorigin + H/2}) {{healthy\\windows}};')
            else:
                write(rf'\node[anchor=center,align=center] at (-1, {yorigin + H/2}) {{malicious\\windows}} ;')
                write(rf'\node[anchor=center,align=center] at (-1, {yorigin - H/2 - H/4}) {{window\\pairs}} ;')
            xx = kappa if l == 'h' else 0
            group_width = W * bars[l]



            for x in range(bars[l]):
                c = cc[l]
                opacity = 20
                overlap = any([ x >= (kappa2) and x < (kappa2 + alpha) for kappa2 in kappas])

                if l == 'm' and not overlap:
                    c = 'gray'
                    opacity=10
                    pass
                x0 = xorigin + (x + xx) * W + pad
                write(rf'\draw[fill={c}!{opacity}] ({x0}, {yorigin}) rectangle ++({Wa}, {H});')

                nodetext = f'${l}_{{{x}}}$'

                if l == 'm' and (x + 2) == bars[l]:
                    nodetext = r'\dots'
                if l == 'm' and (x + 1) == bars[l]:
                    nodetext = r'$m_{\alpha^m}$'
                
                write(rf'\node[] at ({x0 + Wa/2}, {yorigin + H/2}) {{{nodetext}}};')

                if l == 'm' and overlap:
                    nodetext = f'$h_{{{x}}}$' #$h_{%s,{%s}}$' % (0, x-kappa)
                    write(rf'\node[minimum width={Wa}cm,minimum height={H/2}cm,inner ysep=0,font=\small,fill=blue!20] at ({x0 + Wa/2}, {yorigin - H/2}) {{{nodetext}}};')
                    nodetext = f'$m_{{{x}}}$' #'$m_{%s,{%s}}$' % (kappa, x-kappa)
                    write(rf'\node[minimum width={Wa}cm,minimum height={H/2}cm,inner ysep=0,font=\small,fill=red!20] at ({x0 + Wa/2}, {yorigin - H}) {{{nodetext}}};')

                    write(rf'\draw ({x0}, {yorigin - H * 1.25}) rectangle ++ ({Wa}, {H});')
                pass

            # write(rf'\draw[|-|] ({xorigin}, {yorigin + H + 0.01}) ++ ({W * alpha}, 0);')

            # write(rf'\node[] at ({xorigin + xx * group_width + group_width / 2}, {yorigin + H + 0.01}) {{$H_{{0}}$}};')

            yorigin += H

            pass
        yorigin -= 2*H
        # yorigin += H * 2


    # write(r"\node at (5,15) {ciao};")


    write(r"""

    \end{tikzpicture}

    \end{document}

    """)

    with open('main.tex', 'w') as fp:
        fp.write(_txt)
    pass

ciao()