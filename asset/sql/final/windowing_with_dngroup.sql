WITH
	W AS (
		SELECT
			*
		FROM
			MV3_0
		WHERE
			FLOOR(SECONDS / 360) = 0
	),
	W_DN AS (
		SELECT
			COUNT(*) AS QR,
			SUM((RCODE is NULL)::INT) AS Q,
			SUM((RCODE = 3)::INT) AS NX
		FROM
			W
		GROUP BY
			DN_ID
	),
	W_DN_DESCRIBE AS (
		SELECT
			AVG(QR), AVG(Q), AVG(NX),
			STDDEV(QR), STDDEV(Q), STDDEV(NX),
			MAX(QR), MAX(Q), MAX(NX) FROM W_DN
	),
	METRICS AS (
		SELECT
			COUNT(*) FILTER (
				WHERE
					DAC_RANK = 1
			) AS "label_1",
			COUNT(*) FILTER (
				WHERE
					DAC_RANK <= 2
			) AS "label_2",
			COUNT(*) AS QR,
			COUNT(*) FILTER (
				WHERE
					RCODE IS NULL
			) AS Q,
			COUNT(*) FILTER (
				WHERE
					RCODE = 3
			) AS NX,
			-- max()
			COUNT(*) FILTER (
				WHERE
					EPS1 >= 0.5
			) AS P1
		FROM
			W
	)
SELECT
	*
FROM
	METRICS
	JOIN W_DN_DESCRIBE ON TRUE