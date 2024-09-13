


from rules import freq_p, pcaps


FPR = 0.00182

TPR = 0.99997


fanci2 = 27162088 / (30*24)

freq_n_s = [
    500,
    5000,
    fanci2,
    200000
]

freq_p_s = [
    1,
    5,
    10,
    100
]

df_pcap = pcaps(FPR=FPR, TPR=TPR)
df_pcap.to_csv('DF_pcaps_max.csv')

df = freq_p(freq_n_s, freq_p_s, FPR=FPR, TPR=TPR)
df.to_csv('DF.csv')
