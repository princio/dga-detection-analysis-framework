SELECT
  M.WNUM,
  M.DN_ID,
  CASE
    WHEN WL_NN.RANK_BDN IS NULL THEN -1
    ELSE 0
  END AS RANK_BDN,
  M.QR AS QR,
  M.Q AS Q,
  M.R AS R,
  M.NX AS NX,
  DN_NN1.VALUE AS VALUE1,
  DN_NN1.LOGIT AS LOGIT1,
  DN_NN2.VALUE AS VALUE2,
  DN_NN2.LOGIT AS LOGIT2,
  DN_NN3.VALUE AS VALUE3,
  DN_NN3.LOGIT AS LOGIT3,
  DN_NN4.VALUE AS VALUE4,
  DN_NN4.LOGIT AS LOGIT4
FROM
  (
    SELECT
      __M.DN_ID,
      __M.FN_REQ / 50 AS WNUM,
      COUNT(*) AS QR,
      SUM(
        CASE
          WHEN __M.IS_R IS FALSE THEN 1
          ELSE 0
        END
      ) AS Q,
      SUM(
        CASE
          WHEN __M.IS_R IS TRUE THEN 1
          ELSE 0
        END
      ) AS R,
      SUM(
        CASE
          WHEN __M.IS_R IS TRUE
          AND RCODE = 3 THEN 1
          ELSE 0
        END
      ) AS NX
    FROM
      MESSAGE_9 __M
    GROUP BY
      __M.DN_ID,
      WNUM
  ) AS M
  LEFT JOIN (
    SELECT
      *
    FROM
      WHITELIST_DN
    WHERE
      WHITELIST_ID = 1
  ) AS WL_NN ON M.DN_ID = WL_NN.DN_ID
  JOIN (
    SELECT
      *
    FROM
      DN_NN
    WHERE
      NN_ID = 1
  ) AS DN_NN1 ON M.DN_ID = DN_NN1.DN_ID
  JOIN (
    SELECT
      *
    FROM
      DN_NN
    WHERE
      NN_ID = 2
  ) AS DN_NN2 ON M.DN_ID = DN_NN2.DN_ID
  JOIN (
    SELECT
      *
    FROM
      DN_NN
    WHERE
      NN_ID = 3
  ) AS DN_NN3 ON M.DN_ID = DN_NN3.DN_ID
  JOIN (
    SELECT
      *
    FROM
      DN_NN
    WHERE
      NN_ID = 4
  ) AS DN_NN4 ON M.DN_ID = DN_NN4.DN_ID
GROUP BY
  M.WNUM,
  M.DN_ID,
  RANK_BDN,
  QR,
  Q,
  R,
  NX,
  VALUE1,
  LOGIT1,
  VALUE2,
  LOGIT2,
  VALUE3,
  LOGIT3,
  VALUE4,
  LOGIT4