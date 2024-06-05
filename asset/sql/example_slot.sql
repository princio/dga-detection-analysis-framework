WITH WHITELIST AS (SELECT * FROM WHITELIST_DN WHERE WHITELIST_ID = 1),
DN_NN1 AS (SELECT * FROM DN_NN WHERE NN_ID = 1),
DN_NN2 AS (SELECT * FROM DN_NN WHERE NN_ID = 2),
DN_NN3 AS (SELECT * FROM DN_NN WHERE NN_ID = 3),
DN_NN4 AS (SELECT * FROM DN_NN WHERE NN_ID = 4),
PCAPMW AS (SELECT PCAP.ID, DGA FROM PCAP JOIN MALWARE ON PCAP.MALWARE_ID = MALWARE.ID),
MESSAGES AS (
  SELECT 
    __M.DN_ID,
    __M.PCAP_ID, 
    FLOOR(__M.TIME_S_TRANSLATED / (2 * 60 * 60)) AS SLOTNUM, 
    COUNT(*) AS QR,
    SUM(CASE WHEN __M.IS_R IS FALSE THEN 1 ELSE 0 END) AS Q,
    SUM(CASE WHEN __M.IS_R IS TRUE THEN 1 ELSE 0 END) AS R,
    SUM(CASE WHEN __M.IS_R IS TRUE AND RCODE = 3 THEN 1 ELSE 0 END ) AS NX
  FROM
      MESSAGE __M
  GROUP BY
    __M.DN_ID,
    __M.PCAP_ID,
    SLOTNUM
)
SELECT
  M.PCAP_ID,
  PCAPMW.DGA,
  M.SLOTNUM,
  SUM(CASE WHEN WHITELIST.RANK_BDN > 1000000 THEN 1 ELSE 0 END) AS RANK_BDN,
  COUNT(DISTINCT M.DN_ID) AS U,
  SUM(M.QR) AS QR,
  SUM(M.Q) AS Q,
  SUM(M.R) AS R,
  SUM(M.NX) AS NX,
  SUM(CASE WHEN DN_NN1.VALUE <= 0.5 THEN 1 ELSE 0 END) AS NEG_NN1,
  SUM(CASE WHEN DN_NN2.VALUE <= 0.5 THEN 1 ELSE 0 END) AS NEG_NN2,
  SUM(CASE WHEN DN_NN3.VALUE <= 0.5 THEN 1 ELSE 0 END) AS NEG_NN3,
  SUM(CASE WHEN DN_NN4.VALUE <= 0.5 THEN 1 ELSE 0 END) AS NEG_NN4,
  SUM(CASE WHEN DN_NN1.VALUE > 0.5 THEN 1 ELSE 0 END) AS POS_NN1,
  SUM(CASE WHEN DN_NN2.VALUE > 0.5 THEN 1 ELSE 0 END) AS POS_NN2,
  SUM(CASE WHEN DN_NN3.VALUE > 0.5 THEN 1 ELSE 0 END) AS POS_NN3,
  SUM(CASE WHEN DN_NN4.VALUE > 0.5 THEN 1 ELSE 0 END) AS POS_NN4
FROM
  MESSAGES M
  JOIN PCAPMW ON M.PCAP_ID = PCAPMW.ID
  LEFT JOIN WHITELIST ON M.DN_ID = WHITELIST.DN_ID
  JOIN DN_NN1 ON M.DN_ID = DN_NN1.DN_ID
  JOIN DN_NN2 ON M.DN_ID = DN_NN2.DN_ID
  JOIN DN_NN3 ON M.DN_ID = DN_NN3.DN_ID
  JOIN DN_NN4 ON M.DN_ID = DN_NN4.DN_ID
GROUP BY
  M.PCAP_ID,
  PCAPMW.DGA,
  M.SLOTNUM