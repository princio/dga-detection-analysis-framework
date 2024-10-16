SELECT
	SUM(
		(
			W.ID IS NOT NULL
			AND DACRANK < 3
		)::INT
	)
FROM
	MESSAGE2_IT2016_0_COMPACT M2
	JOIN DN ON M2.DN_ID = DN.ID
	LEFT JOIN WHITELIST1 W ON DN.ID = W.DN_ID