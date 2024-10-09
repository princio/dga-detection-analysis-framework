#!/bin/sh

# python run.py "./outdirs"
# "/media/princio/ssd512/dns_detection_datasets/ctu-sme-11/CTU-SME-11_Experiment-VM-Microsoft-Windows7AD-1_v1.0.0/CTU-SME-11/Experiment-VM-Microsoft-Windows7AD-1/2023-02-20/raw/2023-02-20-00-00-03-192.168.1.108.pcap"

PY=/Users/princio/Repo/princio/malware-detection-predict-file/web/mwdb/pybackend/venv/bin/python3

"$PY" run.py "./outdir/" "/Users/princio/Downloads/IT2016/Day0/20160423_235403.pcap"
            