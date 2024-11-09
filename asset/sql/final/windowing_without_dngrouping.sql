
		SELECT
			COUNT(*) FILTER (
				WHERE
					DAC_RANK = 1
			) AS "#dac_rank_1",
			COUNT(*) FILTER (
				WHERE
					DAC_RANK <= 2
			) AS "#dac_rank_2",
			COUNT(*) AS QR,
			COUNT(*) FILTER (
				WHERE
					RCODE IS NULL
			) AS Q,
			COUNT(*) FILTER (
				WHERE
					RCODE = 3
			) AS NX,
			COUNT(*) FILTER (
				WHERE
					EPS1 >= 0.5
			) AS P1,
			COUNT(*) FILTER (
				WHERE
					EPS2 >= 0.5
			) AS P2,
			COUNT(*) FILTER (
				WHERE
					EPS3 >= 0.5
			) AS P3,
			COUNT(*) FILTER (
				WHERE
					EPS4 >= 0.5
			) AS P4,
			ARRAY[
				SUM(CASE WHEN LAMBDA1 = 'infinity' OR LAMBDA1 = '-infinity' THEN 0 ELSE LAMBDA1 END)::int,
				COUNT(*) FILTER (WHERE LAMBDA1 = '-infinity')::int,
				COUNT(*) FILTER (WHERE LAMBDA1 = 'infinity')::int
			] AS LLR1,
			ARRAY[
				SUM(CASE WHEN LAMBDA2 = 'infinity' OR LAMBDA2 = '-infinity' THEN 0 ELSE LAMBDA2 END)::int,
				COUNT(*) FILTER (WHERE LAMBDA2 = '-infinity')::int,
				COUNT(*) FILTER (WHERE LAMBDA2 = 'infinity')::int
			] AS LLR2,
			ARRAY[
				SUM(CASE WHEN LAMBDA3 = 'infinity' OR LAMBDA3 = '-infinity' THEN 0 ELSE LAMBDA3 END)::int,
				COUNT(*) FILTER (WHERE LAMBDA3 = '-infinity')::int,
				COUNT(*) FILTER (WHERE LAMBDA3 = 'infinity')::int
			] AS LLR3,
			ARRAY[
				SUM(CASE WHEN LAMBDA4 = 'infinity' OR LAMBDA4 = '-infinity' THEN 0 ELSE LAMBDA4 END)::int,
				COUNT(*) FILTER (WHERE LAMBDA4 = '-infinity')::int,
				COUNT(*) FILTER (WHERE LAMBDA4 = 'infinity')::int
			] AS LLR4
		FROM
			MV3_0
		WHERE
			FLOOR(SECONDS / 360) = 0