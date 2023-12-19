import pandas as pd
from sqlalchemy import create_engine

eng = create_engine("postgresql://princio:postgres@localhost/dns",)

pd.options.display.float_format = '{:.2f}'.format

pcaps = pd.read_sql(
    f"""SELECT PCAP.ID, MALWARE.DGA FROM PCAP JOIN MALWARE ON PCAP.MALWARE_ID=MALWARE.ID ORDER BY qr""",
    eng,
)

pcaps_good = pcaps[pcaps.dga == 0]

for nn in [7, 8, 9, 10]:
    for th in [0.5, 0.9, 0.99, 0.999]:
        fp_tot = 0
        fp_tot_uniques = 0
        tot = 0
        tot_uniques = 0
        for _, pcap in pcaps_good.iterrows():

            df = pd.read_sql(
                f"""SELECT * FROM MESSAGES_{pcap['id']} AS M JOIN DN_NN ON M.DN_ID=DN_NN.DN_ID WHERE DN_NN.NN_ID={nn}""",
                eng,
            )

            fp = df['value'] >= th


            fp_tot += fp.sum()

            fp_tot_uniques += (df.drop_duplicates(subset=['dn_id'])['value'] >= th).sum()

            tot += df.shape[0]
            tot_uniques += df.drop_duplicates(subset=['dn_id']).shape[0]

            # print(fp.sum(), df.shape[0], df.drop_duplicates(subset=['dn_id']).shape[0])

            pass

        dfnn = pd.read_sql(
            f"""SELECT * FROM nns where id={nn}""",
            eng,
        )
        print(f"{dfnn['name'].iloc[0]}\t{th}\t{1 - fp_tot_uniques/tot_uniques}")
        # print(th)
        # print(f"{fp_tot:10d}\t{tot:10d}\t{1 - fp_tot/tot}")
        # print(f"{fp_tot_uniques:10d}\t{tot_uniques:10d}\t{1 - fp_tot_uniques/tot_uniques}")



