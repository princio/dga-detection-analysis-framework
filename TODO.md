# Analyze with Startosphere chosing the treshold t_w with train/test method

# Analyze the performance impact of the Suffix in a deeper way

# Perform new trainings with the FlashStart domains.

# The domain names are about 160 000. Use some reinforcement learning technique.

# Investigate some solution which consider the number of occurence of a domain

For example, if we are in a centralized DNS server of a small/medium/big company, if the same domain comes from two different terminal then the probability that it will be malicious is low becuase of the DGA nature.

## Notes

Like flashstart, there are a lot of services that could provide a check over Domains integrity.

We need to focus to approaches that doesn't rely on blacklisting except for assured domain names (like Google, ...).

## Can we generate a dataset from the available domain names

Is it so important if a domain name appeare in a stratosphere terminal or in another?

We only need to check the capability of the neural network to see some anomaly in a centralized way.


# Test 1

Using a splitting test/train of the stratosphere dataset we obtain the following results FPR/TPR:

| split  |                      10  |                      50  |                     100  |                    500  |                  2500  |
|--------|--------------------------|--------------------------|--------------------------|-------------------------|------------------------|
|  0.01  |    298   0.3216/98.0812  |     59   1.7128/99.5403  |     29   3.2725/99.6182  |     6  14.6321/99.7440  |    1  40.7733/99.8684  |
|  0.05  |   1492   0.0714/95.1978  |    298   0.3475/99.3461  |    149   0.6094/99.3420  |    30   2.6990/99.1902  |    6  13.5411/99.7772  |
|  0.10  |   2985   0.0439/92.6923  |    597   0.1854/99.2309  |    299   0.2894/99.1037  |    60   1.5283/99.1821  |   12   8.4830/99.6165  |
|  0.20  |   5971   0.0184/87.4538  |   1195   0.0669/99.0564  |    598   0.2574/99.1724  |   121   0.6881/98.7831  |   25   3.9947/99.4441  |
|  0.25  |   7464   0.0131/83.1893  |   1494   0.0676/98.9860  |    748   0.1624/99.0393  |   151   0.6544/98.7325  |   31   2.9095/99.2868  |
|  0.30  |   8957   0.0121/80.9741  |   1793   0.0549/99.0281  |    898   0.1537/98.9807  |   182   0.3896/98.6100  |   37   3.0202/99.3556  |
|  0.40  |  11943   0.0052/68.1234  |   2391   0.0412/98.9382  |   1197   0.0690/98.7729  |   242   0.2466/98.4936  |   50   1.5737/99.2251  |
|  0.50  |  14929   0.0056/72.1391  |   2989   0.0354/98.9204  |   1497   0.0750/98.7498  |   303   0.3776/98.5225  |   63   1.3228/99.1322  |
|  0.60  |  17915   0.0040/65.4844  |   3587   0.0259/98.8653  |   1796   0.1072/98.7959  |   364   0.0642/98.4693  |   75   1.7333/99.1164  |
|  0.70  |  20901   0.0057/66.3426  |   4185   0.0141/98.8304  |   2095   0.0249/98.5538  |   424   0.2055/98.3999  |   88   1.0737/99.1410  |


Where we with _split_ we mean the percentage destinated to the train data set of the whole one.



In TABLE_METRICS.md there are the values for each metric on average based on KFOLDs (10).

The metrics are the following:


