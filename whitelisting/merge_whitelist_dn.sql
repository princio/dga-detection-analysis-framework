INSERT INTO
  PUBLIC.WHITELIST_DN (DN_ID, WHITELIST_ID, RANK_DN, RANK_BDN)
  (
    SELECT
      J_DN.DN_ID,
      1,
      J_DN.DN_RANK,
      J_BDN.BDN_RANK
    FROM
      (
        SELECT
          DN.ID AS DN_ID,
          WL_DN.RANK AS DN_RANK
        FROM
          DN
          LEFT JOIN WHITELIST_LIST_1 WL_DN ON DN.DN = WL_DN.DN
      ) AS J_DN
      JOIN (
        SELECT
          DN.ID AS DN_ID,
          WL_DN.RANK AS BDN_RANK
        FROM
          DN
          LEFT JOIN WHITELIST_LIST_1 WL_DN ON DN.BDN = WL_DN.DN
      ) AS J_BDN ON J_DN.DN_ID = J_BDN.DN_ID
  );