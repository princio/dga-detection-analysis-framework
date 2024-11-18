WITH
	W AS (
		SELECT * FROM
			MV3_0
		WHERE
			FLOOR(SECONDS / 360) = 0
			AND DAC_FAMILY='modpack' OR DAC_FAMILY IS NULL
	),
	W_DN AS (
		SELECT
			DN_ID,
			MAC,
			COUNT(*) AS QR,
			SUM((RCODE is NULL)::INT) AS Q,
			SUM((RCODE = 3)::INT) AS NX
		FROM
			W
		GROUP BY
			DN_ID, MAC
	),
	W_DN_DESCRIBE AS (
		SELECT
			MAC,
			COUNT(*) AS "num DN",
			AVG(QR)::real AS "avg DN/QR",
			AVG(Q)::real AS "avg DN/Q",
			AVG(NX)::real AS "avg DN/NX",
			STDDEV(QR)::real AS "std DN/QR",
			STDDEV(Q)::real AS  "std DN/Q",
			STDDEV(NX)::real AS  "std DN/NX",
			MAX(QR) AS "max DN/QR", 
			MAX(Q) AS "max DN/Q", 
			MAX(NX) AS "max DN/NX" 
		FROM W_DN
		GROUP BY
			MAC
	),
	METRICS AS (
		SELECT
			MAC,
			COUNT(*) FILTER (
				WHERE
					DAC_RANK = 1
			) AS "num DAC_RANK=1",
			COUNT(*) FILTER (
				WHERE
					DAC_RANK <= 2
			) AS "num DAC_RANK>=2",
			COUNT(*) AS QR,
			COUNT(*) FILTER (
				WHERE
					RCODE IS NULL
			) AS Q,
			COUNT(*) FILTER (
				WHERE
					RCODE = 3 AND EPS1 >= 0.5
			) AS NX,
			COUNT(*) FILTER (
				WHERE
					EPS1 >= 0.5
			) AS QR_P1,
			COUNT(*) FILTER (
				WHERE
					RCODE IS NULL AND EPS1 >= 0.5
			) AS Q_P1,
			COUNT(*) FILTER (
				WHERE
					RCODE = 3 AND EPS1 >= 0.5
			) AS NX_P1
			-- max()
		FROM
			W
		GROUP BY
			MAC
	)
SELECT
	*
FROM
	METRICS M
	JOIN W_DN_DESCRIBE W ON M.MAC=W.MAC