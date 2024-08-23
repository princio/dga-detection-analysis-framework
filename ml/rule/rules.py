




from matplotlib import pyplot as plt
import numpy as np
import pandas as pd

from db import Database, get_df
from PDF import PDF, PDFRNG



def pcaps(FPR, TPR):

    pcaps = [('caphaw', 54), ('zbot', 46), ('simda', 58), ('unknown', 57)]

    fanci2 = 27162088 / (30*24)

    freq_n_s = [
        # 5,
        # 50,
        500,
        5000,
        fanci2,
        200000
    ]

    freq_p_s = [
        # 5,
        # 50,
        10,
        100,
        1000
    ]

    db = Database()

    _print = False
    __print = lambda *args, **kwargs, : print(*args, **kwargs) if _print else None

    plot=False
    stats_n = []
    for mwname, pcap_id in pcaps:
        for balance in [0.5, 0.9]:
            for how in ['max', 'mean']:
                for masked in [False, True]:
                    for g in [False, True]:
                        for pt in ['nx']:
                            df = get_df(db, mwname, pcap_id, g, pt)
                            if plot:
                                figure, axs = plt.subplots(len(freq_n_s), 2, figsize=(20,8), sharey=False)
                            for i, freq_n in enumerate(freq_n_s):
                                n = 1
                                pdf = PDF(
                                    int(freq_n * FPR * n),
                                    freq_n / n,
                                    df[df > 0 if masked else df >= 0]['p'].iloc[:30*24].agg(how).item(),
                                    None,
                                    np.sqrt(df.iloc[:30*24].var().item()),
                                    FPR,
                                    TPR
                                )

                                pdf.set_sigma_n(0.6, prob_discost=0.2)

                                if plot:
                                    th, fpr, tpr, roc_auc = pdf.roc(axs[i,:], balance, PDFRNG.POISSON) # type: ignore
                                else:
                                    th, fpr, tpr, roc_auc = pdf.roc(None, balance, PDFRNG.POISSON)

                                stats_n.append([
                                    mwname,
                                    how,
                                    df[df > 0 if masked else df >= 0]['p'].iloc[:30*24].argmax(),
                                    balance,
                                    masked,
                                    g,
                                    pt,
                                    pdf.mu_n,
                                    pdf.mu_p,
                                    pdf.mu_n * pdf.FPR,
                                    pdf.mu_p * pdf.TPR,
                                    th,
                                    fpr,
                                    tpr,
                                    roc_auc
                                ])

                                pass
                            pass
                        pass
                    pass
                pass
            pass
        pass

    DF = pd.DataFrame(stats_n, columns=['mwname', 'how', 'max-index', 'balance', 'masked', 'g', 'pt', 'mu_n', 'mu_p', 'mu_{fp}', 'mu_{tp}', 'th^{best}', 'FPR^{best}', 'TPR^{best}', 'ROC_AUC'])

    return DF



def freq_p(freq_n_s, freq_p_s, FPR, TPR):
    plot=False
    stats_n = []
    for freq_p in freq_p_s:
        for balance in [0.5, 0.9]:
            for masked in [False, True]:
                for g in [False, True]:
                    for pt in ['nx']:
                        for i, freq_n in enumerate(freq_n_s):
                            n = 1
                            pdf = PDF(
                                int(freq_n * FPR * n),
                                freq_n / n,
                                freq_p,
                                None,
                                0,
                                FPR,
                                TPR
                            )

                            pdf.set_sigma_n(0.6, prob_discost=0.2)

                            if plot:
                                th, fpr, tpr, roc_auc = pdf.roc(axs[i,:], balance, PDFRNG.POISSON) # type: ignore
                            else:
                                th, fpr, tpr, roc_auc = pdf.roc(None, balance, PDFRNG.POISSON)

                            stats_n.append([
                                '-',
                                'mean',
                                0,
                                balance,
                                masked,
                                g,
                                pt,
                                pdf.mu_n,
                                pdf.mu_p,
                                pdf.mu_n * pdf.FPR,
                                pdf.mu_p * pdf.TPR,
                                th,
                                fpr,
                                tpr,
                                roc_auc
                            ])

                            pass
                        pass
                    pass
                pass
            pass
        pass

    DF = pd.DataFrame(stats_n, columns=['mwname', 'how', 'max-index', 'balance', 'masked', 'g', 'pt', 'mu_n', 'mu_p', 'mu_{fp}', 'mu_{tp}', 'th^{best}', 'FPR^{best}', 'TPR^{best}', 'ROC_AUC'])

    return DF